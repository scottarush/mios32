//!
//! M3 Pedal Handler
//!
//! \{
/*
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

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////
pedal_config_t pedal_config;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
u8 makePressed;

// number of pending pedal or 0 if not pending
u8 pendingPedalNum;

// If a pedal is pending, the timestamp of the press.
s32 pendingPedalTimestamp;

// Last velocity sent for an On note.  Using this value allows multiple pedals
// to be pressed at same time with same velocity
u8 lastVelocity;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
int PEDALS_GetVelocity(u16 delay, u16 delay_slowest, u16 delay_fastest);
s32 PEDALS_SendNote(u8 note_number, u8 velocity, u8 released);
u8 PEDALS_ComputeNoteNumber(u8 pedalNum);

/////////////////////////////////////////////////////////////////////////////
// Initialize the pedal handler
/////////////////////////////////////////////////////////////////////////////
void PEDALS_Init() {

   // Initialize pedal configuration

   pedal_config_t* pc = (pedal_config_t*)&pedal_config;

   pc->midi_ports = 0x1011;
   pc->midi_chn = 1;

   pc->num_pedals = 12;

   pc->verbose_level = 0;

   pc->velocity_enabled = 0;
   pc->delay_fastest = 100;
   pc->delay_fastest_black_pedals = 120;
   pc->delay_slowest = 500;

   pc->octave = 1;
   pc->transpose = 0;
   pc->left_pedal_note_number = 23;

   // Clear locals
   makePressed = 1;
   pendingPedalNum = 0;   // not pending
   pendingPedalTimestamp = 0;
   lastVelocity = 127;

}

/////////////////////////////////////////////////////////////////////////////
// Called from application upon make switch changes
// @param pedalNum:  1=left most pedal, make switch=num_pedals+1;
// @param pressed: 1=pressed, 0=released
// @param timestamp:  event timestamp
/////////////////////////////////////////////////////////////////////////////
void PEDALS_NotifyMakeChange(u8 pressed, u32 timestamp) {
   pedal_config_t* pc = (pedal_config_t*)&pedal_config;

   if (!pc->velocity_enabled){
      // Just set makePressed and return to effectively disable
      makePressed = 1;
      return;
   }

   u8 velocity = 127;
   u8 note_number = 0;
   if (!pressed) {
      // clear everything since all pedals must be released
      makePressed = 0;
      pendingPedalNum = 0;
      // Default to max for robustness, but a new value should be computed on next press
      lastVelocity = 127;
      return;
   }
   // Otherwise, this is make pressed, so compute the velocity for any  pending pedal
   // and send not on.
   makePressed = 1;
   // send pending pedal press note on.
   if (pendingPedalNum != 0) {

      // A press was active so get system time and compute velocity
      s32 time_ms = MIOS32_TIMESTAMP_Get();
      // TODO:  Handle rollover
      s32 delta_ms = time_ms - pendingPedalTimestamp;
      u16 delay = (u16)delta_ms;
      // compute velocity for press
      // TODO:  handle black key determination.  Just assume all same for now
      velocity = PEDALS_GetVelocity(delay, pc->delay_slowest, pc->delay_fastest);

      // Compute the midi note_number, save the computed velocity and send Note On
      note_number = PEDALS_ComputeNoteNumber(pendingPedalNum);
      lastVelocity = velocity;
      PEDALS_SendNote(note_number, velocity, 1);
      pendingPedalNum = 0;  // clear for next time.
   }
}


/////////////////////////////////////////////////////////////////////////////
// Called from application upon any pedal change
// @param pedalNum:  1=left most pedal
// @param pressed: 1=pressed, 0=released
// @param timestamp:  event timestamp
/////////////////////////////////////////////////////////////////////////////
void PEDALS_NotifyChange(u8 pedalNum, u8 pressed, u32 timestamp) {
   pedal_config_t* pc = (pedal_config_t*)&pedal_config;

   u8 velocity = 127;
   u8 note_number = 0;

   if ((pedalNum == 0) || (pedalNum > pc->num_pedals)) {
      DEBUG_MSG("Invalid pedalNum");
   }
   note_number = PEDALS_ComputeNoteNumber(pedalNum);

   if (!pressed) {
      // This is a release.  Just send note off event.
      PEDALS_SendNote(note_number, velocity, 0);
      return;
   }
   // Otherwise, this is a regular pedal press.  If make already pressed or velocity is
   // disable, then send the press at the last velocity
   if (makePressed || !pc->velocity_enabled) {
      PEDALS_SendNote(note_number, lastVelocity, 1);
      return;
   }
   // Otherwise, this is an initial press without make so:
   // 1. save the pending pedal number and timestamp
   // 2. Wait for the make switch to compute velocity and send note on 
   pendingPedalTimestamp = MIOS32_TIMESTAMP_Get();
   pendingPedalNum = pedalNum;
}

/////////////////////////////////////////////////////////////////////////////
// helper function to compute midi note number
/////////////////////////////////////////////////////////////////////////////
inline u8 PEDALS_ComputeNoteNumber(u8 pedalNum) {
   u8 note_number = pedal_config.octave * 12 +
      pedal_config.left_pedal_note_number +
      pedal_config.transpose +
      pedalNum;
   return note_number;
}


/////////////////////////////////////////////////////////////////////////////
//! Help function to send a MIDI note over given ports
/////////////////////////////////////////////////////////////////////////////
s32 PEDALS_SendNote(u8 note_number, u8 velocity, u8 pressed) {
   pedal_config_t* pc = (pedal_config_t*)&pedal_config;

   u8 sent_note = note_number;

   int i;
   u16 mask = 1;
   for (i = 0; i < 16; ++i, mask <<= 1) {
      if (pc->midi_ports & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

         if (!pressed)
            MIOS32_MIDI_SendNoteOff(port, pc->midi_chn - 1, sent_note, velocity);
         else
            MIOS32_MIDI_SendNoteOn(port, pc->midi_chn - 1, sent_note, velocity);
      }
   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Help function to get MIDI velocity from measured delay
/////////////////////////////////////////////////////////////////////////////
int PEDALS_GetVelocity(u16 delay, u16 delay_slowest, u16 delay_fastest) {
   int velocity = 127;

   if (delay > delay_fastest) {
      // determine velocity depending on delay
      // lineary scaling - here we could also apply a curve table
      velocity = 127 - (((delay - delay_fastest) * 127) / (delay_slowest - delay_fastest));

      // saturate to ensure that range 1..127 won't be exceeded
      if (velocity < 1)
         velocity = 1;
      if (velocity > 127)
         velocity = 127;
   }

   return velocity;
}
