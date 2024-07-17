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

static arp_pattern_t patterns[NUM_PATTERNS] = {
   {"Ascending","Asc",4,{{NORM,1,0,0},{NORM,2,0,0},{NORM,3,0,0},{NORM,4,0,0}}}
};

// Queue of notes.
// Notes set to 0 length are null
static step_note_t patternBuffer[MAX_NUM_STEPS][MAX_NUM_NOTES_PER_STEP];

// index of current pattern
static u8 patternIndex;

// pattern step counter
static u8 stepCounter;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_UpdatePatternBuffer();


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_Init()
{
   patternIndex = 0;
   stepCounter = 0;
   // initialize the Notestack Stack to hold the keys
   NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_BOTTOM_HOLD, &notestack_items[0], MAX_NUM_KEYS);

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called to load a new pattern.  
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_LoadPattern(u8 _patternIndex) {
   if (_patternIndex >= NUM_PATTERNS) {
      return -1;
   }
   if (_patternIndex == patternIndex) {
      return 0;
   }
   // Otherwise new pattern.  Clear the notestack
   NOTESTACK_Clear(&notestack);
   // Flush the queue to play off events
   SEQ_MIDI_OUT_FlushQueue();

   // transfer the new pattern and wait for a key.
   patternIndex = _patternIndex;
   // clear the pattern buffer up to the number of steps in the new pattern

   for (int i = 0;i < ARP_PAT_GetCurrentPattern()->numSteps;i++) {
      for (int j = 0;j < MAX_NUM_NOTES_PER_STEP;j++) {
         step_note_t* pNote = &patternBuffer[i][j];
         pNote->length = 0;
         pNote->note = 0;
         pNote->velocity = 0;
         pNote->tickOffset = 0;
      }
   }



   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Called on a key press.  Up to MAX_NUM_KEYS may be added.
// Returns 0 for success, -1 for too many keys
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyPressed(u8 note, u8 velocity) {
   NOTESTACK_Push(&notestack, note, velocity);
   // if the push was a different note that exceeded the max number of keys, then pop it and return error
   if (NOTESTACK_CountActiveNotes(&notestack) > MAX_NUM_KEYS) {
      NOTESTACK_Pop(&notestack, note);
      return -1;
   }
   // update the pattern buffer from the notestack.
   ARP_PAT_UpdatePatternBuffer();
   return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Called on a key release. 
/////////////////////////////////////////////////////////////////////////////
void ARP_PAT_KeyReleased(u8 note, u8 velocity) {
   NOTESTACK_Push(&notestack, note, velocity);
   // Now update the pattern buffer from the notestack.
   ARP_PAT_UpdatePatternBuffer();
}

/////////////////////////////////////////////////////////////////////////////
// Helper returns a pointer to the currently selected pattern
/////////////////////////////////////////////////////////////////////////////
const arp_pattern_t* ARP_PAT_GetCurrentPattern() {
   return &patterns[patternIndex];
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
         if (stepCounter >= ARP_PAT_GetCurrentPattern()->numSteps) {
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
               midi_package.chn = ARP_GetARPSettings()->midiChannel;
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
   const arp_pattern_t* pPattern = ARP_PAT_GetCurrentPattern();
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

            // Now put the notes of the chord into the pattern buffer at the chordNoteVelocity
            u8 numChordNotes = SEQ_CHORD_GetNumNotesByEnum(chord);
#ifdef DEBUG
            DEBUG_MSG("ARP_PAT_UpdatePatternBuffer: Adding notes @ step:%d k#:%d chord:%s, root=%d oct=%d #notes=%d",
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
               pEvent->keySelect, pPattern->name, step);
         }
         break;
      case NORM:
      case TIE:
         // add the note at the keySelect with a single step length adjusting for the octave and scale step
         if (notestack.len >= pEvent->keySelect) {
            step_note_t* pNote = &patternBuffer[step][pEvent->keySelect];
            pNote->note = notestack.note_items[pEvent->keySelect - 1].note + 12*pEvent->octave+pEvent->scaleStep;
            pNote->velocity = notestack.note_items[pEvent->keySelect - 1].tag;
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