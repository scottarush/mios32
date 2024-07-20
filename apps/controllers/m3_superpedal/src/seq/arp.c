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

static s32 ARP_PlayOffEvents(void);

static s32 ARP_FillChordPadNoteStack(u8 rootNote, u8 velocity);
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
s32 ARP_Init()
{
   // initialize the Notestack Stack will be cleared whenever no note is played anymore
   NOTESTACK_Init(&chordPadNotestack, NOTESTACK_MODE_PUSH_BOTTOM, &chordPadNotestackItems[0], NOTESTACK_SIZE);

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = 0;
   // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
   arpSettings.serializationID = 0x41525001;   // 'ARP1'

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
      arpSettings.midi_ports = 0x0031;     // OUT1, OUT2, and USB
      arpSettings.midiChannel = 1;
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
#ifdef DEBUG_ENABLED
   DEBUG_MSG("ARP_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(persisted_pedal_confg_t));
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
// Also used for chord pad mode to send explict NoteOffs.
/////////////////////////////////////////////////////////////////////////////
static s32 ARP_PlayOffEvents(void)
{
   if (arpSettings.arpMode == ARP_MODE_CHORD_PAD) {
      ARP_SendChordNoteOnOffs(0);
      return 0;
   }
   // Otherwise, it is a sequence mode, so flush the queue to play the "off events
   SEQ_MIDI_OUT_FlushQueue();

   // here you could send additional events, e.g. "All Notes Off" CC

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

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called to fill the note stack for PAD mode
/////////////////////////////////////////////////////////////////////////////
s32 ARP_FillChordPadNoteStack(u8 rootNote, u8 velocity) {
   if (chordPadNotestack.len > 0) {
      if (rootNote != chordPadNotestack.note_items[0].note) {
         // Change of note, so turn off any keys that were on
         ARP_PlayOffEvents();
      }
   }
   // Clear the notestack before filling
   NOTESTACK_Clear(&chordPadNotestack);

   // Get the keys of the chord
   const chord_type_t chord = ARP_MODES_GetModeChord(arpSettings.modeScale,
      arpSettings.chordExtension, arpSettings.rootKey, rootNote);
   if ((chord == CHORD_INVALID) || (chord == CHORD_ERROR)) {
      // This key is not a valid key in this cord.  Just fill the note stack with 
      // this single note instead
      NOTESTACK_Clear(&chordPadNotestack);
      NOTESTACK_Push(&chordPadNotestack, rootNote, velocity);
      return 1;
   }

   // Compute octave by subtracting C-2 (note 24)
   s8 octave = ((rootNote - 24) / 12) - 2;

   // Push the keys one by one onto the note stack from the root
   u8 numChordNotes = SEQ_CHORD_GetNumNotesByEnum(chord);
#ifdef DEBUG
  // DEBUG_MSG("ARP_FillChordPadNoteStack: Pushing chord: %s, chordPlayedNote=%d octave=%d numChordNotes=%d",
    //  SEQ_CHORD_NameGetByEnum(chord), rootNote, octave, numChordNotes);
#endif   
   for (u8 keyNum = 0;keyNum < numChordNotes;keyNum++) {
      s32 note = SEQ_CHORD_NoteGetByEnum(keyNum, chord, octave);
      if (note >= 0) {
         // add offset for the chordPlayedNote
         note += (rootNote % 12);         
         NOTESTACK_Push(&chordPadNotestack, note, velocity);
      }
   }
#ifdef DEBUG
   NOTESTACK_SendDebugMessage(&chordPadNotestack);
#endif
   return 0;
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
   if (!arpEnabled) {
      return 0;   // Note not consumed
   }

   switch (arpSettings.arpMode) {
   case ARP_MODE_CHORD_ARP:
   case ARP_MODE_KEYS:
      // Delegate to the ARP Pattern handler
      return ARP_PAT_KeyPressed(note, velocity);

   case ARP_MODE_CHORD_PAD:
      // Delegate to local fill function
      ARP_FillChordPadNoteStack(note, velocity);
      ARP_SendChordNoteOnOffs(1);
      break;
   }
   return 1; //  Event was consumed by arpeggiator
}


/////////////////////////////////////////////////////////////////////////////
// Called whenever a Note Off event has been received.  The velocity has
// already been scaled
/////////////////////////////////////////////////////////////////////////////
s32 ARP_NotifyNoteOff(u8 note, u8 velocity) {
   if (!arpEnabled) {
      return 0;  // note not consumed
   }

   // Otherwise arp or pad mode active
   if (arpSettings.arpMode == ARP_MODE_CHORD_PAD) {
      // Call function directly so release velocity gets sent
      ARP_SendChordNoteOnOffs(0);
   }
   else {
      // delegate to arp pattern handler
      ARP_PAT_KeyReleased(note, velocity);
   }
   return 1;  // note consumed
}

/////////////////////////////////////////////////////////////////////////////
// Sends NoteOns/Offs for for a chord in the notestack.
// sendOn:  if > 0 then note On.  == 0 for NoteOffs
/////////////////////////////////////////////////////////////////////////////
void ARP_SendChordNoteOnOffs(u8 sendOn) {
   //  DEBUG_MSG("ARP_SendChordNoteOnOffs: sendOn=%d notestack.len=%d",sendOn,notestack.len);
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
            if (arpSettings.midi_ports & mask) {
               // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
               mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
               if (sendOn) {
                  MIOS32_MIDI_SendNoteOn(port, arpSettings.midiChannel - 1, note, velocity);
               }
               else {
                  MIOS32_MIDI_SendNoteOff(port, arpSettings.midiChannel - 1, note, velocity);
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
   // Otherwise a mode change
   arpSettings.arpMode = mode;
   switch (arpSettings.arpMode) {
   case ARP_MODE_KEYS:
   case ARP_MODE_CHORD_ARP:
      // reset the ARP
      ARP_Reset();
      break;
   case ARP_MODE_CHORD_PAD:
      // Play off events and stop the sequencer
      ARP_PlayOffEvents();
      SEQ_BPM_Stop();
      break;
   }
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
      // Disable arp
      ARP_PlayOffEvents();
      SEQ_BPM_Stop();
   }
   else {
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
      return "CHRD";
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
// gets the midi channel
/////////////////////////////////////////////////////////////////////////////
u8 ARP_GetMIDIChannel() {
   return arpSettings.midiChannel;
}
/////////////////////////////////////////////////////////////////////////////
// Sets the midi channel
/////////////////////////////////////////////////////////////////////////////
void ARP_SetMIDIChannel(u8 channel) {
   if (arpSettings.midiChannel == channel) {
      return;
   }
   arpSettings.midiChannel = channel;

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
