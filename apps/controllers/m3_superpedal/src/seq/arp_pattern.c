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

static arp_pattern_t patterns[MAX_NUM_PATTERNS];
// TODO:  Copy in some good patterns from Blue Arp

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
   if (_patternIndex >= MAX_NUM_PATTERNS) {
      return -1;
   }
   if (_patternIndex == patternIndex) {
      return 0;
   }
   // Otherwise new pattern.  Clear the notestack
   NOTESTACK_Clear(&notestack);

   // clear the pattern buffer by 
   for (int i = 0;i < MAX_NUM_STEPS;i++) {
      for (int j = 0;j < MAX_NUM_NOTES_PER_STEP;j++) {
         patternBuffer[i][j].length = 0;
         patternBuffer[i][j].note = 0;
         patternBuffer[i][j].velocity = 0;
      }
   }
   // Flush the queue to play off events
   SEQ_MIDI_OUT_FlushQueue();

   // transfer the new pattern and wait for a key.
   patternIndex = _patternIndex;
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Called on a key press.  Up to MAX_NUM_KEYS may be added.
// Returns 0 for success, -1 for too many keys
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PAT_KeyPressed(u8 note, u8 velocity) {
   NOTESTACK_Push(&notestack, note, velocity);
   // Now update the pattern buffer from the notestack.
   ARP_PAT_UpdatePatternBuffer();
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
static s32 ARP_PAT_Tick(u32 bpm_tick)
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
               midi_package.chn = ARP_GetARPSettings().midiChannel;
               midi_package.note = note;
               midi_package.velocity = velocity;

               // Play on the enabled ports.
               int i;
               u16 mask = 1;
               for (i = 0; i < 16; ++i, mask <<= 1) {
                  if (ARP_GetARPSettings().midi_ports & mask) {
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
   // TODO
}