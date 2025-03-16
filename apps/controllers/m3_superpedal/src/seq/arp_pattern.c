// $Id$
/*
 * Arpeggiator pattern data model
 *
 * ==========================================================================
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "pedals.h"
#include "arp_pattern.h"
#include "arp_pattern_data.h"
#include "seq_chord.h"
#include "seq_midi_out.h"
#include "seq_bpm.h"

#include "FreeRTOS.h"
#include "tasks.h"

#define DEBUG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_KEYS 5

static notestack_t notestack;
static notestack_item_t notestack_items[MAX_NUM_KEYS];


/**
 * step_note_t are the individual notes within the runtime pattern_buffer.
 * The pattern buffer can have up to MAX_NUM_NOTES_PER_STEP of these in order
 * to allow chords to be played by the arpeggiator.
 */
typedef struct step_note_s {
   u8 note;
   u8 velocity;
   // duration in bpmticks from starting tickoffset in the step
   u8 length;
   // offset += in bpmticks.  Default is 0 at start of step
   s32 tickOffset;

} step_note_t;

// Runtime buffer filled from the note stack in UpdatePatternBuffer
static step_note_t patternBuffer[MAX_NUM_STEPS][MAX_NUM_NOTES_PER_STEP];

// pattern step counter
static u8 stepCounter;
static u8 localPatternIndex;

// reset flag resets at the next ARP tick
static u8 resynchArpeggiator;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_UpdatePatternBuffer();
static void ARP_PAT_ClearPatternBuffer();

/////////////////////////////////////////////////////////////////////////////
// Initialisation
// This must be called AFTER Arp pattern init.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_Init()
{
   stepCounter = 0;
   // initialize the Notestack Stack to hold the keys
   NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_BOTTOM, &notestack_items[0], MAX_NUM_KEYS);

   localPatternIndex = ARP_GetARPSettings()->arpPatternIndex;

   resynchArpeggiator = 0;

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called to set the current pattern which will be persisted to storage.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_SetCurrentPattern(u8 _patternIndex) {
   s32 error = ARP_PAT_ActivatePattern(_patternIndex);
   if (error < 0) {
      return -1;
   }
   localPatternIndex = _patternIndex;
   // Persist the pattern change
   ARP_GetARPSettings()->arpPatternIndex = localPatternIndex;
   ARP_PersistData();
   DEBUG_MSG("ARP_PAT_SetCurrentPattern:  Set pattern:%d read pattern: %d", localPatternIndex, ARP_GetARPSettings()->arpPatternIndex);
   return 0;
}
////////////////////////////////////////////////////////////////////////////
// Called to activate a pattern.  It will not be persisted to storage.
// Returns -1 on invalid index, 0 on no pattern change, and +1 on change to
// new pattern
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_ActivatePattern(u8 _patternIndex) {
   if (_patternIndex >= NUM_PATTERNS) {
      return -1;
   }
   if (_patternIndex == localPatternIndex) {
      return 0;
   }
   // Otherwise new pattern.  Transfer locally but do not persist.
#ifdef DEBUG
   DEBUG_MSG("ARP_PAT_ActivatePattern:  activating pattern: %s", ARP_PAT_GetPatternName(_patternIndex));
#endif
   // Update local index
   localPatternIndex = _patternIndex;

   // Reset everything
   ARP_PAT_Reset();
   return 1;
}


/////////////////////////////////////////////////////////////////////////////
// Helper clears the pattern buffer 
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_ClearPatternBuffer() {
   MUTEX_PATTERN_BUFFER_TAKE
      for (int i = 0;i < patterns[localPatternIndex].numSteps;i++) {
         for (int j = 0;j < MAX_NUM_NOTES_PER_STEP;j++) {
            step_note_t* pNote = &patternBuffer[i][j];
            pNote->length = 0;
            pNote->note = 0;
            pNote->velocity = 0;
            pNote->tickOffset = 0;
         }
      }
   MUTEX_PATTERN_BUFFER_GIVE
}

/////////////////////////////////////////////////////////////////////////////
// Called on a key press.  Up to MAX_NUM_KEYS may be added.
// returns 1 if note consumed, 0 if not.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyPressed(u8 rootNote, u8 velocity) {

   int handled = 0;
   int resynch = 0;
   // If notestack is zero, then we will need to immediately synch sequencer once
   // updated notes are in the stack
   if (notestack.len == 0) {
      resynch = 1;
   }
   switch (ARP_GetArpMode()) {
   case ARP_MODE_ONEKEY_CHORD_ARP:
      if (!ARP_GetEnabled()) {
         // Shouldn't have been called with ARP disabled.  return not handled
         return 0;
      }
      // Otherwise ARP running 
      if (notestack.len > 0){
         if (rootNote != notestack.note_items[0].note) {
            // Change of note, flush the queue to play the "off events
            SEQ_MIDI_OUT_FlushQueue();
            // force a sequence resynch
            resynch = 1;
         }
      }

      // Re-fill it with the notes for the new root.  If not in the chord then handled will change
      // to zero
      handled = ARP_PAT_FillChordNotestack(&notestack, rootNote, velocity);

      // update the pattern buffer 
      if (handled) {
         ARP_PAT_UpdatePatternBuffer();
      }
      // fall through to determine whether to resynhc or not
      break;
   case ARP_MODE_CHORD_PAD:
      // Should have been handled in ARP_NotifyNoeOn. This is an invalid call, return not handled
      return 0;
   case ARP_MODE_MULTI_KEY:
      if (!ARP_GetEnabled()) {
         // Shouldn't have been called with ARP disabled.  return not handled
         return 0;
      }
      // Otherwise add the key to the notestack unless already too many keys
      if (notestack.len >= MAX_NUM_KEYS) {
          return 0;         // Caller can decide what to do with it
      }
      else {
         NOTESTACK_Push(&notestack, rootNote, velocity);
      }
      break;
   }
   if (handled && resynch){
      // Was a handled event requiring a resynch so set resynch flag for next tick.
      resynchArpeggiator = 1;
   }
   return handled;
}
/////////////////////////////////////////////////////////////////////////////
// Helper function looks up the chord and fills the supplied notetstack
// Defined as a global helper also used to fill manually played cords when the
// ARP is not running.
// returns 1 if valid chord play, 0 if not a valid root for current mode/scale
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_FillChordNotestack(notestack_t* pNotestack, u8 rootNote, u8 velocity) {
   // Fill the note stack with the the keys of the chord as k1...k#notes to the stack.  
     //  This is the same as pressing all the keys.  The stack will be processed at the next
     // synch point.
   persisted_arp_data_t* pArpSettings = ARP_GetARPSettings();
   const chord_type_t chord = ARP_MODES_GetModeChord(pArpSettings->modeScale,
      pArpSettings->modeGroup, pArpSettings->rootKey, rootNote);
   // Compute octave by subtracting C-2 (note 24)
   s8 octave = ((rootNote - 24) / 12) - 2;

   // Now put the notes of the chord into the pattern buffer at the chordNoteVelocity
   u8 numChordNotes = SEQ_CHORD_GetNumNotesByEnum(chord);
#ifdef DEBUG
   DEBUG_MSG("ARP_FillChordPadNoteStack: Pushing chord: %s, chordPlayedNote=%d octave=%d numChordNotes=%d",
      SEQ_CHORD_NameGetByEnum(chord), rootNote, octave, numChordNotes);
#endif   
   for (u8 keyNum = 0;keyNum < numChordNotes;keyNum++) {
      s32 chordNote = SEQ_CHORD_NoteGetByEnum(keyNum, chord, octave);
      // Add offset from the root note
      chordNote += (rootNote % 12);

      NOTESTACK_Push(pNotestack, chordNote, velocity);
   }

   // if the push was a different note that exceeded the max number of keys, then pop it and return 
   // it was pulled from notestack in error
   if (NOTESTACK_CountActiveNotes(&notestack) > MAX_NUM_KEYS) {
      NOTESTACK_Pop(pNotestack, rootNote);
   }
#ifdef DEBUG
   DEBUG_MSG("ARP_PAT_FillChordNotestack");
   NOTESTACK_SendDebugMessage(pNotestack);
#endif
   return (pNotestack->len > 0);
}


/////////////////////////////////////////////////////////////////////////////
// Called on a key release. 
// Returns +1 if not consumed, 0 if not consumed and caller should ahndle.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyReleased(u8 note, u8 velocity) {

   switch (ARP_GetArpMode()) {
   case ARP_MODE_ONEKEY_CHORD_ARP:
      if (!ARP_GetEnabled()) {
         // Shouldn't have been called with ARP disabled.  return not handled
         return 0;
      }
      // Check if the note is in the notestack
      int handled = 1;
      if (NOTESTACK_Pop(&notestack, note) < 0) {
         // This note isn't in the stack so clear handled flag to allow caller to turn it off
         // This happends on a release of an out-of-chord key.
         handled = 0;
      }
      // Clear the Notestack and the pattern buffer
      NOTESTACK_Clear(&notestack);
      ARP_PAT_ClearPatternBuffer();
      // And flush the SEQ queue to play the off notes in queue
      SEQ_MIDI_OUT_FlushQueue();
      return handled;
   case ARP_MODE_MULTI_KEY:
      if (!ARP_GetEnabled()) {
         // Shouldn't have been called with ARP disabled.  return not handled
         return 0;
      }
      // Remove the note
      int removed = NOTESTACK_Pop(&notestack, note);
      if (removed >= 0) {
         //  update the pattern buffer.
         ARP_PAT_UpdatePatternBuffer();
         return 1;
      }
      else {
         return 0;  // Note was not in the notestack
      }
   case ARP_MODE_OFF:
      // Arp off return 0.  This hsould not have been called
      DEBUG_MSG("Error:  ARP_PAT_KeyReleased called in ARP_MODE_OFF: note=%d", note);
      return 0;
   case ARP_MODE_CHORD_PAD:
      // Should not have been called in chord pad mode.  This is an error.  Return not handled
      DEBUG_MSG("Error:  ARP_PAT_KeyReleased called in ARP_MODE_CHORD_PAD: note=%d", note);
   }
   return 0;

   // Delete the following once we fix all the stuck notes
   /**
   // Send the note off in case it wasn't actually in the notestack or during edge-case transitions between
   // notes and into/out of ARP mode, an On can be sent, so we need to send an off
   int i;
   u16 mask = 1;
   u8 midiChannel = PEDALS_GetMIDIChannel() - 1;
   mios32_midi_port_t ports = PEDALS_GetMIDIPorts();

   for (i = 0; i < 16; ++i, mask <<= 1) {
      if (ports & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
         MIOS32_MIDI_SendNoteOff(port, midiChannel, note, velocity);
      }
   }
      **/
}

/////////////////////////////////////////////////////////////////////////////
// Helper returns name of the currently active pattern
/////////////////////////////////////////////////////////////////////////////
const char* ARP_PAT_GetCurrentPatternShortName() {
   return patternShortNames[localPatternIndex];
}

/////////////////////////////////////////////////////////////////////////////
// Helper returns index currently active pattern
/////////////////////////////////////////////////////////////////////////////
u8 ARP_PAT_GetCurrentPatternIndex() {
   return ARP_GetARPSettings()->arpPatternIndex;
}

/////////////////////////////////////////////////////////////////////////////
// Helper returns long name for a pattern
/////////////////////////////////////////////////////////////////////////////
const char* ARP_PAT_GetPatternName(u8 patternIndex) {
   if (patternIndex > NUM_PATTERNS - 1) {
      return "ERR!";
   }

   return patternNames[patternIndex];
}

/////////////////////////////////////////////////////////////////////////////
// Resets the pattern generator by clearing the notestack, pattern Buffer
// and sequencer queue.
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_Reset() {
   NOTESTACK_Clear(&notestack);
   stepCounter = 0;
   ARP_PAT_ClearPatternBuffer();
   SEQ_MIDI_OUT_FlushQueue();
   // And reset the tick so that we can quickly get to the next note
   SEQ_BPM_TickSet(0);
}
/////////////////////////////////////////////////////////////////////////////
// Forward from ARP_Handler when in pattern mode for each tick.  This function will output
// notes from the pattern buffer to the sequenceer
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_Tick(u32 bpm_tick) {
   u32 thisTick = bpm_tick;
   // Check for resynch
   if (resynchArpeggiator){
      // Clear the resynch
      resynchArpeggiator = 0;

      // Flush queue for off notes just in case not previously called
      SEQ_MIDI_OUT_FlushQueue();
      // (re)set this tick to 0 so we execute the pattern reset right now. 
      thisTick = 0;
      // and resynch seq bpm generate
      SEQ_BPM_TickSet(0);

   }

   // whenever we reach a new 16th note (96 ticks @384 ppqn):
   if ((thisTick % (SEQ_BPM_PPQN_Get() / 4)) == 0) {
      // ensure that arp is reset on first bpm_tick
      if (thisTick == 0) {
         stepCounter = 0;
      }
      else {
         // increment step counter
         ++stepCounter;

         // Check if we have reach the end of the pattern (also the synch point)
         if (stepCounter >= patterns[localPatternIndex].numSteps) {
            // Reset the step counter
            stepCounter = 0;

         }
      }

      // Now get the active notes for this step/tick and send to the sequencer for output
      for (u8 i = 0;i < MAX_NUM_NOTES_PER_STEP;i++) {
         u8 length = patternBuffer[stepCounter][i].length;
         if (length > 0) {
            // there is a note here
            u8 note = patternBuffer[stepCounter][i].note;
            u8 velocity = patternBuffer[stepCounter][i].velocity;

            // put note into queue if all values are != 0
            if (note && velocity && length) {
               mios32_midi_package_t midi_package;
               midi_package.type = NoteOn; // package type must match with event!
               midi_package.event = NoteOn;
               // For some reason SEQ_MIDI_OUT is 0 based midiChannel but ARPSettings is 1...16 
               // so subtract 1.
               midi_package.chn = PEDALS_GetMIDIChannel() - 1;
               midi_package.note = note;
               midi_package.velocity = velocity;

               mios32_midi_port_t ports = PEDALS_GetMIDIPorts();

               // Play on the enabled ports.
               int i;
               u16 mask = 1;
               for (i = 0; i < 16; ++i, mask <<= 1) {
                  if (ports & mask) {
                     // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
                     mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

                     SEQ_MIDI_OUT_Send(port, midi_package, SEQ_MIDI_OUT_OnOffEvent, thisTick, length);
                  }
               }
            }
         }
      }

   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Helper updates the pattern buffer from the current NOTESTACK
// Called whenever there is a change in the notestack content.
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_UpdatePatternBuffer() {
   // Clear the pattern buffer first
   ARP_PAT_ClearPatternBuffer();
   if (notestack.len == 0) {
      // There are no notes left in the stack so clear the step counter.  This ensures
      // that on the next note, the sequence will start from the beginning of the
      // patternBuffer and not in the middle.
      stepCounter = 0;
      // send out note offs to keep from stuck notes
      SEQ_MIDI_OUT_FlushQueue();
   }
   // Take the pattern buffer mutex while filling so that ARP_PAT_Tick thread 
   // has coherent data.
   MUTEX_PATTERN_BUFFER_TAKE

      // At least one note in the notestack so refill the buffer
      const arp_pattern_t* pPattern = &patterns[localPatternIndex];

   // Step size in PPQN for computing note on lengths below.
   u32 stepPPQN = SEQ_BPM_PPQN_Get() / 4;

   // Go through each step in the pattern buffer and create the notes according to the 
   // arpeggiator rules (i.e. same as BlueARP)
   for (u8 step = 0;step < pPattern->numSteps;step++) {
      const step_event_t* pEvent = &pPattern->events[step];
      switch (pEvent->stepType) {
      case STEP_TYPE_RNDM:
         // TODO
         break;
      case STEP_TYPE_CHORD:
         // Copy all the notes in the notestack at the current step.   
         // Note that keySelect is not valid here as key order is not relevant.         
         for (u8 noteIndex = 0;noteIndex < notestack.len;noteIndex++) {
            if (noteIndex >= MAX_NUM_NOTES_PER_STEP) {
               // Shouldn't happen but to keep from overflowing the array below
               break;
            }
            u8 note = notestack.note_items[noteIndex].note;
            step_note_t* pStepNote = &patternBuffer[step][noteIndex];
            pStepNote->note = note;
            pStepNote->length = stepPPQN;
            // Velocity is in the 'tag' item
            pStepNote->velocity = notestack.note_items[noteIndex].tag;
         }
         break;
      case STEP_TYPE_NORM:
      case STEP_TYPE_TIE:
         // add the note at the keySelect with a single step length adjusting for the octave and scale step
         if (notestack.len >= pEvent->keySelect) {
            step_note_t* pStepNote = &patternBuffer[step][pEvent->keySelect - 1];
            // Compute the octave and scale adjusted note, then limit to valid midi notes
            s32 note = notestack.note_items[pEvent->keySelect - 1].note + (12 * pEvent->octaveOffset) + pEvent->scaleStepOffset;
            if (note < 0) {
               note = 0;
            }
            else if (note > 127) {
               note = 127;
            }
            pStepNote->note = note;

            // set the velocity which is the 'tag' element.
            pStepNote->velocity = notestack.note_items[pEvent->keySelect - 1].tag;

            // Note set the length according to whether this is a NORM or a TIE
            if (pEvent->stepType == STEP_TYPE_NORM) {
               pStepNote->length = stepPPQN;
               pStepNote->tickOffset = 0;
            }
            else {
               // this is a TIE so overlap the note on this step by + and - 1/4 of the stepPPQN
               s32 overlap = stepPPQN / 4;
               pStepNote->length = stepPPQN + overlap;
               pStepNote->tickOffset = -overlap;
            }
         }
         break;
      case STEP_TYPE_OFF:
         // No notes for this step.
         break;
      case STEP_TYPE_REST:
         // Extend the length of the notes in the previous step, if any, by the length of this one.
         if (step > 0) {
            for (u8 keyNum = 0;keyNum < MAX_NUM_NOTES_PER_STEP;keyNum++) {
               step_note_t* pNote = &patternBuffer[step][keyNum];
               if (pNote->length > 0) {
                  // This is a valid key so add another step to this note
                  pNote->length += stepPPQN;
               }
            }
         }
         break;
      }
   }

   // Release the pattern buffer mutex
   MUTEX_PATTERN_BUFFER_GIVE
}