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

// Queue of notes.
// Notes set to 0 length are null
static step_note_t patternBuffer[MAX_NUM_STEPS][MAX_NUM_NOTES_PER_STEP];

// pattern step counter
static u8 stepCounter;
static u8 localPatternIndex;

static u8 updatePatternBufferCount;

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

   updatePatternBufferCount = 0;
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
   DEBUG_MSG("ARP_PAT_SetCurrentPattern:  Set pattern:%d read pattern: %d",localPatternIndex,ARP_GetARPSettings()->arpPatternIndex);
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
// Returns 0 for not consumed or too many keys, 1 for consumed
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyPressed(u8 note, u8 velocity) {
   //-- If we are in chord mode then clear the notestack and push a new chord into
   // the stack
   if (ARP_GetARPSettings()->arpMode == ARP_MODE_CHORD_ARP) {
      // Clear notestack
      NOTESTACK_Clear(&notestack);
      // set the update flag so that it gets updated after the next synch point
      updatePatternBufferCount = ARP_GetARPSettings()->synchDelay;

      // Fill the note stack with the the keys of the chord as k1...k#notes to the stack.  
      //  This is the same as pressing all the keys.  The stack will be processed at the next
      // synch point.
      persisted_arp_data_t* pArpSettings = ARP_GetARPSettings();
      const chord_type_t chord = ARP_MODES_GetModeChord(pArpSettings->modeScale,
         pArpSettings->chordExtension, pArpSettings->rootKey, note);
      // Compute octave by subtracting C-2 (note 24)
      s8 octave = ((note - 24) / 12) - 2;

      // Now put the notes of the chord into the pattern buffer at the chordNoteVelocity
      u8 numChordNotes = SEQ_CHORD_GetNumNotesByEnum(chord);

      for (u8 keyNum = 0;keyNum < numChordNotes;keyNum++) {
         s32 chordNote = SEQ_CHORD_NoteGetByEnum(keyNum, chord, octave);
         // Add offset from the root note
         chordNote += (note % 12);

         NOTESTACK_Push(&notestack, chordNote, velocity);
      }
   }
   else {
      // Not in chord mode so just push the note
      NOTESTACK_Push(&notestack, note, velocity);

      // if the push was a different note that exceeded the max number of keys, then pop it and return error
      if (NOTESTACK_CountActiveNotes(&notestack) > MAX_NUM_KEYS) {
         NOTESTACK_Pop(&notestack, note);
         return 0;
      }
      // And update the pattern buffer immediately
      ARP_PAT_UpdatePatternBuffer();
   }
   return 1; // note consumed

}
/////////////////////////////////////////////////////////////////////////////
// Called on a key release. 
// returns 0 not consumed, 1 consumed
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyReleased(u8 note, u8 velocity) {

   u8 cleared = 0;
   if (ARP_GetARPSettings()->arpMode == ARP_MODE_CHORD_ARP) {
      // Check if the release key is the root in the notestack
      if (notestack_items[0].note == note) {
         // Same note so clear notestack and fall through to update pattern buffer (which
         // will just clear it out and reset step counter.
         NOTESTACK_Clear(&notestack);
         cleared = 1;
         // set the update buffer pending flag so that it gets updated after the next
         // synch point in ARP_PAT_Tick
         updatePatternBufferCount = ARP_GetARPSettings()->synchDelay;
      }
   }
   if (!cleared) {
      // Not the root or we are in key mode so remove the note
      NOTESTACK_Pop(&notestack, note);
      // Update the pattern buffer immediately if we didn't clear
      ARP_PAT_UpdatePatternBuffer();
   }

   // Send the note off just in case to avoid a stuck on note.  During edge-case transitions between 
   // notes and into/out of ARP mode, an On can be sent, so we need to send an off
   int i;
   u16 mask = 1;
   u8 midiChannel = ARP_GetARPSettings()->midiChannel;
   for (i = 0; i < 16; ++i, mask <<= 1) {
      if (ARP_GetARPSettings()->midi_ports & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
         MIOS32_MIDI_SendNoteOff(port, midiChannel, note, velocity);
      }
   }

   return 1;   // note consumed
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
}
/////////////////////////////////////////////////////////////////////////////
// Forward from ARP_Tick when in pattern mode for each tick.  This function will output
// notes from the pattern buffer to the sequenceer
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_Tick(u32 bpm_tick)
{
   // whenever we reach a new 16th note (96 ticks @384 ppqn):
   if ((bpm_tick % (SEQ_BPM_PPQN_Get() / 4)) == 0) {
      // ensure that arp is reset on first bpm_tick
      if (bpm_tick == 0) {
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
      // Check if there is an update pattern buffer pending (count > 0) and count down
      // 1/4-note beat synch points until it expires then update the buffer
      if (updatePatternBufferCount > 0) {
         // Check if we are at the 1/4 beat synch point
         if ((bpm_tick % (SEQ_BPM_PPQN_Get() / 16)) == 0) {
            // If so, then decrement the count
            updatePatternBufferCount--;
            if (updatePatternBufferCount == 0){
              ARP_PAT_UpdatePatternBuffer();
            }
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
               midi_package.chn = ARP_GetARPSettings()->midiChannel - 1;
               midi_package.note = note;
               midi_package.velocity = velocity;

               // Play on the enabled ports.
               int i;
               u16 mask = 1;
               for (i = 0; i < 16; ++i, mask <<= 1) {
                  if (ARP_GetARPSettings()->midi_ports & mask) {
                     // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
                     mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

                     SEQ_MIDI_OUT_Send(port, midi_package, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, length);
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

      // At least one note in the notestack so refill it
      const arp_pattern_t* pPattern = &patterns[localPatternIndex];
   u32 stepPPQN = SEQ_BPM_PPQN_Get() / 4;

   for (u8 step = 0;step < pPattern->numSteps;step++) {
      const step_event_t* pEvent = &pPattern->events[step];
      switch (pEvent->stepType) {
      case RNDM:
         // TODO
         break;
      case CHORD:
         // Get the key for the root of the chord.  Make sure that the 
         // key number in the pattern doesn't exceed the notestack len
         if (notestack.len >= pEvent->keySelect) {
            u8 chordNote = notestack.note_items[pEvent->keySelect - 1].note;
            u8 chordNoteVelocity = notestack.note_items[pEvent->keySelect - 1].tag;

            // Lookup the chord notes from the root in k1 and the current modeScale setting
            // and load them into the pattern buffer.           
            persisted_arp_data_t* pArpSettings = ARP_GetARPSettings();
            const chord_type_t chord = ARP_MODES_GetModeChord(pArpSettings->modeScale,
               pArpSettings->chordExtension, pArpSettings->rootKey, chordNote);
            // Compute octave by subtracting C-2 (note 24)
            s8 octave = ((chordNote - 24) / 12) - 2;
            // adjust octave by the delta in the event
            octave += pEvent->octave;
            // Add offset from the root note
            chordNote += (chordNote % 12);
            // Now put the notes of the chord into the pattern buffer at the chordNoteVelocity
            u8 numChordNotes = SEQ_CHORD_GetNumNotesByEnum(chord);
#ifdef DEBUG
            //            DEBUG_MSG("ARP_PAT_UpdatePatternBuffer.CHORD: Adding notes @ step:%d k#:%d chord:%s, root=%d oct=%d #notes=%d",
              //             step, pEvent->keySelect, SEQ_CHORD_NameGetByEnum(chord), chordNote, octave, numChordNotes);
#endif                  
            for (u8 keyNum = 0;keyNum < numChordNotes;keyNum++) {
               s32 note = SEQ_CHORD_NoteGetByEnum(keyNum, chord, octave);
               step_note_t* pNote = &patternBuffer[step][keyNum];
               pNote->note = note;
               pNote->length = stepPPQN;
               pNote->velocity = chordNoteVelocity;
            }
         }
         else {
            DEBUG_MSG("ARP_PAT_UpdatePatternBuffer: Error: k#: %d for pattern:%s step:%d exceeds notestack size",
               pEvent->keySelect, patternNames[localPatternIndex], step);
         }
         break;
      case NORM:
         // add the note at the keySelect with a single step length adjusting for the octave and scale step
         if (notestack.len >= pEvent->keySelect) {
            step_note_t* pNote = &patternBuffer[step][pEvent->keySelect - 1];
            pNote->note = notestack.note_items[pEvent->keySelect - 1].note + 12 * pEvent->octave + pEvent->scaleStep;
            pNote->velocity = notestack.note_items[pEvent->keySelect - 1].tag;
#ifdef DEBUG
            //          DEBUG_MSG("ARP_PAT_UpdatePatternBuffer:NORM Adding note %02x @ step:%d k#:%d",
              //           pNote->note, step, pEvent->keySelect);
#endif                  
            if (pEvent->stepType == NORM) {
               pNote->length = stepPPQN;
               pNote->tickOffset = 0;
            }
            else {
               // this is a TIE so overlap the note on this step by + and - 1/4 of the stepPPQN
               s32 overlap = stepPPQN / 4;
               pNote->length = stepPPQN + overlap;
               pNote->tickOffset = -overlap;
            }
         }
         break;
      case OFF:
         // Don't add a note - do nothing here.
         break;
      case REST:
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