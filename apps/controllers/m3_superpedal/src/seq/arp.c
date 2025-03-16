/*
 * Arpeggiator functions for M3 superpedal extended from
 * Tutorial 18 - originally seq.c/h
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 *  with extensions by Scott Rush 2024.
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <seq_bpm.h>
#include <seq_midi_out.h>
#include <notestack.h>
#include "pedals.h"
#include "arp.h"
#include "arp_modes.h"
#include "arp_pattern.h"
#include "persist.h"
#include "seq_scale.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 6

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void ARP_SendChordNoteOnOffs(u8 sendOn);

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the arpeggiator position
static u8 arp_counter;

// enables/disables arp
static u8 arpEnabled = 0;

// notestack
static notestack_t chordPadNotestack;
static notestack_item_t chordPadNotestackItems[6];

persisted_arp_data_t arpSettings;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Init(u8 resetDefaults)
{
   // initialize the Notestack Stack will be cleared whenever no note is played anymore
   NOTESTACK_Init(&chordPadNotestack, NOTESTACK_MODE_PUSH_BOTTOM, &chordPadNotestackItems[0], NOTESTACK_SIZE);

   s32 valid = -1;

   if (resetDefaults == 0) {
      // Restore settings from E^2 if they exist.  If not then initialize to defaults
     // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
      arpSettings.serializationID = 0x41525001;   // 'ARP1'
   }
   valid = PERSIST_ReadBlock(PERSIST_ARP_BLOCK, (unsigned char*)&arpSettings, sizeof(persisted_arp_data_t));
   if (valid < 0) {
      DEBUG_MSG("ARP_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");
      arpSettings.arpMode = ARP_MODE_CHORD_ARP;
      arpSettings.arpPatternIndex = 0;
      arpSettings.rootKey = KEY_A;
      arpSettings.modeScale = SCALE_AEOLIAN;
      arpSettings.chordExtension = CHORD_EXT_SEVENTH;
      arpSettings.ppqn = 384;
      arpSettings.bpm = 120.0;
      ARP_PersistData();

   }

   // clear the arp counter
   arp_counter = 0;

   // reset arpeggiator
   ARP_Reset();

   // init platform BPM generator
   SEQ_BPM_Init(0);

   // Restore the ppqn and bpm
   SEQ_BPM_PPQN_Set(arpSettings.ppqn);
   SEQ_BPM_Set(arpSettings.bpm);

   // Disabled by default. HMI must turn it on.
   arpEnabled = 0;

   return 0; // no error
}
/////////////////////////////////////////////////////////////////////////////
// Global function to store persisted arpeggiator data
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PersistData() {
#ifdef DEBUG
   DEBUG_MSG("ARP_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(persisted_arp_data_t));
#endif
   s32 valid = PERSIST_StoreBlock(PERSIST_ARP_BLOCK, (unsigned char*)&arpSettings, sizeof(persisted_arp_data_t));
   if (valid < 0) {
      DEBUG_MSG("ARP_PersistData:  Error persisting setting to EEPROM");
   }
   return valid;
}


/////////////////////////////////////////////////////////////////////////////
// this  handler is called periodically from the 1ms TASK_ARP in app.c
// to check for new requests from BPM generator.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Handler(void)
{
   if (((!arpEnabled)) ||
      (arpSettings.arpMode == ARP_MODE_CHORD_PAD)) {
      return 0;
   }
   // handle requests

   u8 num_loops = 0;
   u8 again = 0;
   do {
      ++num_loops;

      // note: don't remove any request check - clocks won't be propagated
      // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
      if (SEQ_BPM_ChkReqStop()) {
         ARP_PlayOffEvents();
      }

      if (SEQ_BPM_ChkReqCont()) {
         // release pause mode
//         arpPause = 0
         arpEnabled = 0;
      }

      if (SEQ_BPM_ChkReqStart()) {
         ARP_Reset();
      }

      u16 new_song_pos;
      if (SEQ_BPM_ChkReqSongPos(&new_song_pos)) {
         ARP_PlayOffEvents();
      }

      u32 bpm_tick;
      if (SEQ_BPM_ChkReqClk(&bpm_tick) > 0) {
         again = 1; // check all requests again after execution of this part
         ARP_PAT_Tick(bpm_tick);
      }
   } while (again && num_loops < 10);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
s32 ARP_PlayOffEvents(void)
{
   // Otherwise, it is a sequence mode, so flush the queue to play the "off events
   SEQ_MIDI_OUT_FlushQueue();

   // Send an all notes off to all enable ports just in case we had a bug resulting
   // in stuck on keys.
   int i;
   u16 mask = 1;
   // Get the channel and ports from the Pedal object
   mios32_midi_port_t ports = PEDALS_GetMIDIPorts();
   u8 channel = PEDALS_GetMIDIChannel();
   for (i = 0; i < 16; ++i, mask <<= 1) {
      if (ports & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
         MIOS32_MIDI_SendCC(port, channel - 1, 123, 0);
      }
   }

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Reset(void)
{
   // timebase may have changed, ensure that Off-Events are played 
   // (otherwise they will be played much later...)
   ARP_PlayOffEvents();

   // reset BPM tick
   SEQ_BPM_TickSet(0);

   // Also reset pattern
   ARP_PAT_Reset();

   return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// Called when Note event has been received 
// If velocity is 0 then this is a note off event.
// The velocity has already been scaled
// Returns 1 if consumed by arpeggiator, 0 is arpeggiator inactive and event
// not consumed.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_NotifyNoteOn(u8 note, u8 velocity)
{
   switch (ARP_GetArpMode()) {
   case ARP_MODE_CHORD_ARP:
   case ARP_MODE_KEYS:
      if (ARP_GetEnabled()) {
         // Delegate to the ARP Pattern handler
         return ARP_PAT_KeyPressed(note, velocity);
      }
      break;
   case ARP_MODE_CHORD_PAD:
      // play a chord
      NOTESTACK_Clear(&chordPadNotestack);
      s32 filled = ARP_PAT_FillChordNotestack(&chordPadNotestack, note, velocity);
      if (filled == 0) {
         // Invalid root so return 0
         return 0;
      }
      // Otherwise, filled so send the notes on
      ARP_SendChordNoteOnOffs(1);
      return 1;
   case ARP_MODE_OFF:
      return 0;  // Not consumed
   }
   return 0;   // Not consumed
}


/////////////////////////////////////////////////////////////////////////////
// Called whenever a Note Off event has been received.  The velocity has
// already been scaled
/////////////////////////////////////////////////////////////////////////////
s32 ARP_NotifyNoteOff(u8 note, u8 velocity) {

   // Otherwise handle according to mode
   switch (ARP_GetArpMode()) {
   case ARP_MODE_CHORD_ARP:
   case ARP_MODE_KEYS:
      if (ARP_GetEnabled()) {
         // Delegate to the ARP Pattern handler
         ARP_PAT_KeyReleased(note, velocity);
         return 1;  // consumed by ARP
      }
      break;
   case ARP_MODE_CHORD_PAD:
      // Check if this note is the root of the chord stack
      int found = 0;
      if (chordPadNotestack.len > 0) {
         if (chordPadNotestack.note_items[0].note == note) {
            found = 1;
         }
      }
      if (!found) {
         // This isn't the root or the chord stack is empty so return to allow caller to send the off
         return 0;
      }
      // This is the root of the current stack so send all the offs
      ARP_SendChordNoteOnOffs(0);

      return 1;
   case ARP_MODE_OFF:
      return 0;  // Not consumed
   }
   return 0;   // Not consumed
}

/////////////////////////////////////////////////////////////////////////////
// Sends NoteOns/Offs for for a chord in the notestack.
// sendOn:  if > 0 then note On.  == 0 for NoteOffs
/////////////////////////////////////////////////////////////////////////////
void ARP_SendChordNoteOnOffs(u8 sendOn) {
   //  DEBUG_MSG("ARP_SendChordNoteOnOffs: sendOn=%d notestack.len=%d",sendOn,notestack.len);
   // Get the channel and ports from the Pedal object
   mios32_midi_port_t ports = PEDALS_GetMIDIPorts();
   u8 channel = PEDALS_GetMIDIChannel();
   for (u8 count = 0;count < chordPadNotestack.len;count++) {
      // get note/velocity/length from notestack
      u8 note = chordPadNotestackItems[count].note;
      u8 velocity = chordPadNotestackItems[count].tag;
      // put note into queue if all values are != 0
      if (note >= 0) {
         // Play note the enabled ports.
         int i;
         u16 mask = 1;
         for (i = 0; i < 16; ++i, mask <<= 1) {
            if (ports & mask) {
               // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
               mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
               if (sendOn) {
                  MIOS32_MIDI_SendNoteOn(port, channel - 1, note, velocity);
               }
               else {
                  MIOS32_MIDI_SendNoteOff(port, channel - 1, note, velocity);
               }
            }
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
// Sets the ARP operating mode.
// mode:  arp_mode_t
/////////////////////////////////////////////////////////////////////////////
void ARP_SetArpMode(arp_mode_t mode) {
   if (mode == arpSettings.arpMode) {
      return;
   }
   // Otherwise a mode change save it and reset the arp
   arpSettings.arpMode = mode;
   ARP_Reset();

   // Save the mode change to EE
   ARP_PersistData();
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables arp
/////////////////////////////////////////////////////////////////////////////
void ARP_SetEnabled(u8 enabled) {
   if (arpEnabled == enabled) {
      return;
   }
   arpEnabled = enabled;
   if (!arpEnabled) {
      // Disable arp and send all off events to avoid any stuck notes
      ARP_PlayOffEvents();
      SEQ_BPM_Stop();
   }
   else {
      // Reset 
      ARP_Reset();
      // start SEQ
      SEQ_BPM_Start();
   }
}
/////////////////////////////////////////////////////////////////////////////
// Returns arp enable status
/////////////////////////////////////////////////////////////////////////////
u8 ARP_GetEnabled() {
   return arpEnabled;
}
/////////////////////////////////////////////////////////////////////////////
// Returns current arp mode
/////////////////////////////////////////////////////////////////////////////
const arp_mode_t ARP_GetArpMode() {
   return arpSettings.arpMode;
}
/////////////////////////////////////////////////////////////////////////////
// Returns text of current arp state for display
/////////////////////////////////////////////////////////////////////////////
const char* ARP_GetArpStateText() {
   if (!arpEnabled) {
      return "STOP";
   }
   switch (arpSettings.arpMode) {
   case ARP_MODE_CHORD_ARP:
      return "ARP";
   case ARP_MODE_CHORD_PAD:
      return "PAD";
   case ARP_MODE_KEYS:
      return "KEYS";
   }
   return "ERR!";
}

/////////////////////////////////////////////////////////////////////////////
// Returns current BPM
/////////////////////////////////////////////////////////////////////////////
float ARP_GetBPM() {
   return SEQ_BPM_Get();
}
/////////////////////////////////////////////////////////////////////////////
// Sets the BPM and persists to EEPROM
/////////////////////////////////////////////////////////////////////////////
void ARP_SetBPM(u16 bpm) {
   if (bpm < 10) {
      return;
   }
   arpSettings.bpm = bpm;
   SEQ_BPM_Set(bpm);
   // And persist the change
   ARP_PersistData();
}
/////////////////////////////////////////////////////////////////////////////
// gets clock mode
/////////////////////////////////////////////////////////////////////////////
arp_clock_mode_t ARP_GetClockMode() {
   return arpSettings.clockMode;
}
/////////////////////////////////////////////////////////////////////////////
// Sets the clock mode
/////////////////////////////////////////////////////////////////////////////
void ARP_SetClockMode(arp_clock_mode_t mode) {
   if (arpSettings.clockMode == mode) {
      return;
   }
   arpSettings.clockMode = mode;

   ARP_Reset();
   // persist the change
   ARP_PersistData();
}
/////////////////////////////////////////////////////////////////////////////
// Returns arp settings 
/////////////////////////////////////////////////////////////////////////////
persisted_arp_data_t* ARP_GetARPSettings() {
   return &arpSettings;
}
/////////////////////////////////////////////////////////////////////////////
// Returns current root key from 0-11
/////////////////////////////////////////////////////////////////////////////
u8 ARP_GetRootKey() {
   return arpSettings.rootKey;
}

/////////////////////////////////////////////////////////////////////////////
// Sets the root key and persists to EEPROM
// rootKey:  normalized key from 0..11
/////////////////////////////////////////////////////////////////////////////
void ARP_SetRootKey(u8 rootKey) {
   arpSettings.rootKey = rootKey;
   ARP_PersistData();
}
/////////////////////////////////////////////////////////////////////////////
// Returns current ModeScale
/////////////////////////////////////////////////////////////////////////////
scale_t ARP_GetModeScale() {
   return arpSettings.modeScale;
}
/////////////////////////////////////////////////////////////////////////////
// Sets the mode and persists to EEPROM
/////////////////////////////////////////////////////////////////////////////
void ARP_SetModeScale(scale_t scale) {
   arpSettings.modeScale = scale;
   ARP_PersistData();
}

/////////////////////////////////////////////////////////////////////////////
// Sets chord extension and persists to EEPROM
/////////////////////////////////////////////////////////////////////////////
void ARP_SetChordExtension(chord_extension_t extension) {
   arpSettings.chordExtension = extension;
   ARP_PersistData();
}
