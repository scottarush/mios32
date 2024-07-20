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
   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called to set the current pattern which will be persisted to storage.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_SetCurrentPattern(u8 _patternIndex) {
   s32 error = ARP_PAT_ActivatePattern(_patternIndex);
   if (error <= 0){
      return -1;
   }
   ARP_PAT_ActivatePattern(_patternIndex);
   // Persist the pattern change
   ARP_GetARPSettings()->arpPatternIndex = _patternIndex;
   ARP_PersistData();
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
   DEBUG_MSG("ARP_PAT_ActivatePattern:  activating pattern: %s",ARP_PAT_GetPatternName(_patternIndex));
   #endif
   // Update local index
   localPatternIndex = _patternIndex;

   NOTESTACK_Clear(&notestack);
   // Flush the queue to play off events
   SEQ_MIDI_OUT_FlushQueue();
   // clear the pattern buffer up to the number of steps in the new patter
   ARP_PAT_ClearPatternBuffer();
   return 1;
}


/////////////////////////////////////////////////////////////////////////////
// Helper clears the pattern buffer.
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_ClearPatternBuffer() {
   for (int i = 0;i < patterns[localPatternIndex].numSteps;i++) {
      for (int j = 0;j < MAX_NUM_NOTES_PER_STEP;j++) {
         step_note_t* pNote = &patternBuffer[i][j];
         pNote->length = 0;
         pNote->note = 0;
         pNote->velocity = 0;
         pNote->tickOffset = 0;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
// Called on a key press.  Up to MAX_NUM_KEYS may be added.
// Returns 0 for not consumed or too many keys, 1 for consumed
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyPressed(u8 note, u8 velocity) {
   //-- If we are in chord mode and this is a different note, then clear the notestack
   // and repush to play a different chord
   if (ARP_GetARPSettings()->arpMode == ARP_MODE_CHORD_ARP) {
      // check if this is a different note than the first note, if so then clear the note stack and repush
      if (notestack.len > 0) {
         if (notestack.note_items[0].note != note) {
            // Clear the stack
            NOTESTACK_Clear(&notestack);
#ifdef DEBUG
            DEBUG_MSG("ARP_PAT_KeyPressed:  clearing notestack for note: %d", note);
            NOTESTACK_SendDebugMessage(&notestack);
#endif
         }
         else {
#ifdef DEBUG
            DEBUG_MSG("ARP_PAT_KeyPressed:  notestack NOT cleared for note: %d", note);
            NOTESTACK_SendDebugMessage(&notestack);
#endif           
         }
      }
   }

   //--------------------------------------------------------------------------------------
   // if arp is in CHORD_ARP mode and push all the keys into the stack
   if (ARP_GetARPSettings()->arpMode == ARP_MODE_CHORD_ARP) {

      // If the notestack is empty then this is the root of a chord so add the keys of the chord 
      // as k1...k#notes to the stack.  This is the same as pressing all the keys
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
#ifdef DEBUG
      DEBUG_MSG("ARP_PAT_KeyPressed: Added %d notes to notestack for chord:%s, root=%02X",
         numChordNotes, SEQ_CHORD_NameGetByEnum(chord), note);
      NOTESTACK_SendDebugMessage(&notestack);
#endif          
   }
   else {
      // Note in chord mode so just push the note
      NOTESTACK_Push(&notestack, note, velocity);

      // if the push was a different note that exceeded the max number of keys, then pop it and return error
      if (NOTESTACK_CountActiveNotes(&notestack) > MAX_NUM_KEYS) {
         NOTESTACK_Pop(&notestack, note);
         return 0;
      }
   }
   // update the pattern buffer from the notestack.
   ARP_PAT_UpdatePatternBuffer();
   return 1; // note consumed

}
/////////////////////////////////////////////////////////////////////////////
// Called on a key release. 
// returns 0 not consumed, 1 consumed
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyReleased(u8 note, u8 velocity) {

   u8 cleared = 0;
   if (ARP_GetARPSettings()->arpMode == ARP_MODE_CHORD_ARP) {
      // Clear out the notestack
      NOTESTACK_Clear(&notestack);
#ifdef DEBUG
      DEBUG_MSG("ARP_PAT_KeyReleased cleared notestack for note:%d", note);
      NOTESTACK_SendDebugMessage(&notestack);
#endif
   }
   if (!cleared) {
      // Not the root or we are in key mode so just remove the note
      NOTESTACK_Pop(&notestack, note);
   }

   // Now update the pattern buffer from the notestack.
   ARP_PAT_UpdatePatternBuffer();

   return 1;   // note consumed
}

/////////////////////////////////////////////////////////////////////////////
// Helper returns short nmae for a pattern
/////////////////////////////////////////////////////////////////////////////
const char * ARP_PAT_GetCurrentPatternShortName() {
   return patternShortNames[localPatternIndex];
}
/////////////////////////////////////////////////////////////////////////////
// Helper returns name for a pattern
/////////////////////////////////////////////////////////////////////////////
const char * ARP_PAT_GetPatternName(u8 patternIndex) {
   if (patternIndex > NUM_PATTERNS-1){
      return "ERR!";
   }
   
   return patternNames[patternIndex];
}

/////////////////////////////////////////////////////////////////////////////
// Forward from ARP_Tick when in pattern mode for each tick.  This function will output
// notes from the pattern buffer to the sequenceer
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_Tick(u32 bpm_tick)
{
   // whenever we reach a new 16th note (96 ticks @384 ppqn):
   if ((bpm_tick % (SEQ_BPM_PPQN_Get() / 4)) == 0) {
      // ensure that arp is reseted on first bpm_tick
      if (bpm_tick == 0) {
         stepCounter = 0;

      }
      else {
         // increment step counter
         ++stepCounter;

         // reset once we reached number of steps in pattern
         if (stepCounter >= patterns[localPatternIndex].numSteps) {
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
               midi_package.chn = ARP_GetARPSettings()->midiChannel-1;
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
   if (notestack.len == 0){
      // There are no notes in the stack so nothing to fill
      return;
   }



   // Now refill it
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
            DEBUG_MSG("ARP_PAT_UpdatePatternBuffer.CHORD: Adding notes @ step:%d k#:%d chord:%s, root=%d oct=%d #notes=%d",
               step, pEvent->keySelect, SEQ_CHORD_NameGetByEnum(chord), chordNote, octave, numChordNotes);
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
            step_note_t* pNote = &patternBuffer[step][pEvent->keySelect-1];
            pNote->note = notestack.note_items[pEvent->keySelect - 1].note + 12 * pEvent->octave + pEvent->scaleStep;
            pNote->velocity = notestack.note_items[pEvent->keySelect - 1].tag;
#ifdef DEBUG
 //           DEBUG_MSG("ARP_PAT_UpdatePatternBuffer:NORM Adding note %02x @ step:%d k#:%d",
 //              pNote->note, step, pEvent->keySelect);
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
}