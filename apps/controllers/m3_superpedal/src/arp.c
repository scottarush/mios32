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
#include "persist.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 ARP_PlayOffEvents(void);
static s32 ARP_Tick(u32 bpm_tick);

static s32 ARP_PersistData();
static s32 ARP_FillNoteStack();

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the arpeggiator position
static u8 arp_counter;

// current root note for ARP_ROOT_MODE or 0 if invalid/not active
static u8 rootNote;

// velocity for root note
static u8 rootNoteVelocity;

// pause mode (will be controlled from user interface)
static u8 arpPause = 0;

// Enables/disables arpeggiator.
static u8 arpEnabled = 0;

// notestack
static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];

persisted_arp_data_t arpSettings;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Init()
{
   // initialize the Notestack
  // for an arpeggiator we prefer sorted mode
  // activate hold mode as well. Stack will be cleared whenever no note is played anymore
   NOTESTACK_Init(&notestack, NOTESTACK_MODE_SORT_HOLD, &notestack_items[0], NOTESTACK_SIZE);

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = 0;
   valid = PERSIST_ReadBlock(PERSIST_ARP_BLOCK, (unsigned char*)&arpSettings, sizeof(persisted_arp_data_t));
   if (valid < 0) {
      DEBUG_MSG("ARP_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");
      arpSettings.genOrder = ARP_GEN_ORDER_ASCENDING;
      arpSettings.arpMode = ARP_MODE_KEYS;
      arpSettings.ppqn = 384;
      arpSettings.bpm = 120.0;

      valid = ARP_PersistData();
      if (valid < 0) {
         DEBUG_MSG("ARP_Init:  Error persisting settings to EEPROM");
      }

      rootNote = 0;  // invalid, inactive
      rootNoteVelocity = 0;
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
// this  handler is called periodically from the TASK_ARP in app.c
// to check for new requests from BPM generator.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Handler(void)
{
   if (arpEnabled == 0) {
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
         arpPause = 0;
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

         ARP_Tick(bpm_tick);
      }
   } while (again && num_loops < 10);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 ARP_PlayOffEvents(void)
{
   // play "off events"
   SEQ_MIDI_OUT_FlushQueue();

   // here you could send additional events, e.g. "All Notes Off" CC

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 ARP_Reset(void)
{
   // since timebase has been changed, ensure that Off-Events are played 
   // (otherwise they will be played much later...)
   ARP_PlayOffEvents();

   // release pause mode
   arpPause = 0;

   // reset BPM tick
   SEQ_BPM_TickSet(0);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 ARP_Tick(u32 bpm_tick)
{
   // whenever we reach a new 16th note (96 ticks @384 ppqn):
   if ((bpm_tick % (SEQ_BPM_PPQN_Get() / 4)) == 0) {
      // ensure that arp is reseted on first bpm_tick
      if (bpm_tick == 0) {
         arp_counter = 0;

      }
      else {
         // increment arpeggiator counter
         ++arp_counter;

         // reset once we reached length of notestack
         if (arp_counter >= notestack.len)
            arp_counter = 0;
      }

      if (notestack.len > 0) {
         // get note/velocity/length from notestack
         u8 note = notestack_items[arp_counter].note;
         u8 velocity = notestack_items[arp_counter].tag;
         u8 length = 72; // always the same, could be varied, e.g. via CC

         // put note into queue if all values are != 0
         if (note && velocity && length) {
            mios32_midi_package_t midi_package;
            midi_package.type = NoteOn; // package type must match with event!
            midi_package.event = NoteOn;
            midi_package.chn = Chn1;
            midi_package.note = note;
            midi_package.velocity = velocity;

            SEQ_MIDI_OUT_Send(DEFAULT, midi_package, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, length);
         }
      }
   }

   return 0; // no error
}
/////////////////////////////////////////////////////////////////////////////
// Called to (re)fill the note stack in Root mode.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_FillNoteStack() {
   if (arpSettings.arpMode == ARP_MODE_KEYS) {
      DEBUG_MSG("ARP_FillNoteStack:  Invalid call.  Current mode=%d", arpSettings.arpMode);
      return -1;
   }
   // Init the notestack since the mode could have changes and compute the fill array from the chord
   notestack_mode_t notestackMode = NOTESTACK_MODE_PUSH_TOP;

   if (arpSettings.genOrder == ARP_GEN_ORDER_DESCENDING){
      notestackMode = NOTESTACK_MODE_PUSH_BOTTOM;
   }
   NOTESTACK_Init(&notestack, notestackMode, &notestack_items[0], NOTESTACK_SIZE);

   // Get the keys of the chord using the rootNote, the current keyMode, and the octave of the key
   u8 key = rootNote % 12;

   const chord_type_t chord = ARP_MODES_GetChordType(arpSettings.keyMode, key);
   if (chord == CHORD_INVALID_ERROR) {
      DEBUG_MSG("ARP_FillNoteStack: Invalid keyMode=%d or key=%d combination", arpSettings.keyMode, key);
      return -1;
   }
   u8 octave = rootNote / 12;

   // Push the keys onto the note stack
   for (int keyNum = 0; keyNum < 6;keyNum++) {
      u8 note = SEQ_CHORD_NoteGetByEnum(keyNum, chord, octave);
      if (note >= 0){
         NOTESTACK_Push(&notestack,note,0);
      }
   }
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note event has been received 
// If velocity is 0 then this is a note off event.
/////////////////////////////////////////////////////////////////////////////
s32 ARP_NotifyNoteOn(u8 note, u8 velocity)
{
   u8 clear_stack = 0;
   if (arpSettings.arpMode == ARP_MODE_ROOT) {
      // If Notestack length is > 0, then ignore (because this must be another key.  Have to release the root first.)
      if (notestack.len > 0) {
         return 0;
      }
      // Otherwise, this is either the release of the root or a new root
      if (note == rootNote) {
         if (velocity == 0) {
            // Release of the root note so set clear_stack and fall through to
            // shut everything off
            clear_stack = 1;
         }
      }
      else {
         // This is a new root, so if velocity > 0 then refill the note stack
         if (velocity > 0) {
            rootNote = note;
            rootNoteVelocity = velocity;
            ARP_FillNoteStack();
         }
         // Just return even if velocity is 0 on a new root (which shouldn't happen)
         return 0;
      }
   }

   if (velocity)
      // push note into note stack
      NOTESTACK_Push(&notestack, note, velocity);
   else {
      // remove note from note stack
      // function returns 2 if no note played anymore (all keys depressed)
      if (NOTESTACK_Pop(&notestack, note) == 2)
         clear_stack = 1;
   }

   // At least one note played?
   if (!clear_stack && notestack.len > 0) {
      // start sequencer if it isn't already running
      if (!SEQ_BPM_IsRunning())
         SEQ_BPM_Start();
   }
   else {
      // clear stack
      NOTESTACK_Clear(&notestack);

      // no key is pressed anymore: stop sequencer
      SEQ_BPM_Stop();
   }


#ifdef DEBUG
   // optional debug messages
   NOTESTACK_SendDebugMessage(&notestack);
#endif

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note Off event has been received.
// forwards to ARP_NotifyNoteOn with velocity 0
/////////////////////////////////////////////////////////////////////////////
s32 ARP_NotifyNoteOff(u8 note)
{
   return ARP_NotifyNoteOn(note, 0);
}


/**
 * Returns the current Arpeggiator generation mode.
*/
arp_gen_order_t ARP_GetArpGenOrder() {
   return arpSettings.genOrder;
}

/////////////////////////////////////////////////////////////////////////////
// Sets the ArpGenMode.  On a change, the notestack order will be changed
// at the next bar.
/////////////////////////////////////////////////////////////////////////////
u8 ARP_SetArpGenOrder(arp_gen_order_t genOrder) {
   if (arpSettings.genOrder == genOrder)
      return 0;
   // Otherwise this is a change
   arpSettings.genOrder = genOrder;

   // Persist settings to E2
   ARP_PersistData();

   // And refill the note stack if arpeggiator currently running
   if (arpEnabled){
      ARP_FillNoteStack();
   }
   return 0;
}



/////////////////////////////////////////////////////////////////////////////
// Enables/Disables Arpeggiator.
/////////////////////////////////////////////////////////////////////////////
void ARP_SetEnabled(u8 enabled) {
   if (arpEnabled == 0) {
      if (enabled) {
         arpEnabled = 1;
         ARP_Reset();
      }
   }
   else {
      // Disable Arp
      arpEnabled = 0;
      // Play Off Events to release all the notes
      ARP_PlayOffEvents();
   }
}
/////////////////////////////////////////////////////////////////////////////
// Returrns enabled/disable state of Arpeggiator
/////////////////////////////////////////////////////////////////////////////
u8 ARP_GetEnabled() {
   return arpEnabled;
}

/////////////////////////////////////////////////////////////////////////////
// Returns current BPM
/////////////////////////////////////////////////////////////////////////////
float ARP_GetBPM() {
   return SEQ_BPM_Get();
}
/////////////////////////////////////////////////////////////////////////////
// Sets the BPM
/////////////////////////////////////////////////////////////////////////////
void ARP_SetBPM(u16 bpm) {
   if (bpm < 10) {
      return;
   }
   arpSettings.bpm = bpm;
   SEQ_BPM_Set(bpm);
}