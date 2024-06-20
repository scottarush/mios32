/*
 * ==========================================================================
 *  M3 Pedal handler.
 *   
 *  Implements the MIDI pedal functionality.
 *  Abstracted pedal and make switch inputs are received from app.c.
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

// Last press velocity sent for an On note.  Using this value allows multiple pedals
// to be pressed at same time with same velocity
u8 lastPressVelocity;

// timestamp of make release used to compute release velocity
s32 makeReleaseTimestamp;

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

   pc->midi_ports = 0x0031;     // OUT1 and USB
   pc->midi_chn = 1;

   pc->num_pedals = 12;

   pc->verbose_level = 0;

   pc->delay_fastest = 6;
   pc->delay_fastest_black_pedals = 10;
   pc->delay_slowest = 70;
   pc->delay_slowest_black_pedals = 60;
   pc->delay_release_fastest = 20;
   pc->delay_release_slowest = 100;

   pc->minimum_press_velocity = 20;
   pc->minimum_release_velocity = 40;

   pc->octave = DEFAULT_OCTAVE_NUMBER;
   pc->transpose = 0;
   pc->left_pedal_note_number = 23;

   // Clear locals
   makePressed = 1;
   pendingPedalNum = 0;   // not pending
   pendingPedalTimestamp = 0;
   makeReleaseTimestamp = 0;
   lastPressVelocity = 127;
}

/////////////////////////////////////////////////////////////////////////////
// Called from application upon make switch changes
// @param pedalNum:  1=left most pedal, make switch=num_pedals+1;
// @param pressed: 1=pressed, 0=released
// @param timestamp:  event timestamp
/////////////////////////////////////////////////////////////////////////////
void PEDALS_NotifyMakeChange(u8 pressed, u32 timestamp) {
   pedal_config_t* pc = (pedal_config_t*)&pedal_config;

   //DEBUG_MSG("PEDALS_NotifyMakeChange:  %s.  time=%d",pressed > 0 ? "pressed" : "released",timestamp);

   u8 note_number = 0;
   if (!pressed) {
      // Clear makePressed
      makePressed = 0;
      // save makeReleased timestamp for use in release velocity calculation
      makeReleaseTimestamp = timestamp;
      // Default to max for robustness, but a new value should be computed on next press
      lastPressVelocity = 127;
      return;
   }
   // Otherwise, this is make pressed, so compute the velocity for any  pending pedal
   // and send not on.
   makePressed = 1;
   // send pending pedal press note on.
   if (pendingPedalNum != 0) {

      // A press was active compute velocity
      u32 delta = timestamp - pendingPedalTimestamp;
      u16 delay = (u16)delta;
      
      // compute velocity for press
      u16 delayFastest = pc->delay_fastest;
      u16 delaySlowest = pc->delay_slowest;
      if ((pendingPedalNum == 2) || (pendingPedalNum ==4) || (pendingPedalNum == 7) || (pendingPedalNum ==9) || (pendingPedalNum == 11)){
         delayFastest = pc->delay_fastest_black_pedals;
         delaySlowest = pc->delay_slowest_black_pedals;
      }
      
      lastPressVelocity = PEDALS_GetVelocity(delay, delaySlowest, delayFastest);
      if (lastPressVelocity < pc->minimum_press_velocity){
         lastPressVelocity = pc->minimum_press_velocity;
      }
      //DEBUG_MSG("press delay=%d velocity=%d",delay,lastPressVelocity);

      // Compute the midi note_number, save the computed velocity and send Note On
      note_number = PEDALS_ComputeNoteNumber(pendingPedalNum);
      PEDALS_SendNote(note_number, lastPressVelocity, 1);
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

   //DEBUG_MSG("PEDALS_NotifyChange: pedal %d %s.  time=%d",pedalNum, pressed > 0 ? "pressed" : "released",timestamp);

   u8 note_number = 0;

   if ((pedalNum == 0) || (pedalNum > pc->num_pedals)) {
      DEBUG_MSG("Invalid pedalNum");
   }
   note_number = PEDALS_ComputeNoteNumber(pedalNum);

   if (!pressed) {
      // This is a release.  compute release velocity using make release timestamp;
      u32 delta = timestamp - makeReleaseTimestamp;
      u16 delay = (u16)delta;
      
      u8 velocity = PEDALS_GetVelocity(delay, pc->delay_release_slowest, pc->delay_release_fastest);
      if (velocity < pc->minimum_release_velocity){
         velocity = pc->minimum_release_velocity;
      }
      //DEBUG_MSG("release delay=%d velocity=%d",delay,velocity);

      PEDALS_SendNote(note_number, velocity, 0);
      return;
   }
   // Otherwise, this is a regular pedal press.  If make already pressed then this is
   // a subsequent pedal so just send at the last velocity which was computed for the first press.
   if (makePressed) {
      PEDALS_SendNote(note_number, lastPressVelocity, 1);
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

         //DEBUG_MSG("midi tx:  port=0x%x",port);
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
/////////////////////////////////////////////////////////////////////////////
// API to set the current octave
// octave:  MIDI octave from 0 to 7
/////////////////////////////////////////////////////////////////////////////
void PEDALS_SetOctave(u8 octave){
   if (octave > 7){
      DEBUG_MSG("Invalid octave number=%d",octave);
      return;
   }
   pedal_config.octave = octave;
}

/////////////////////////////////////////////////////////////////////////////
// API to get the current octave
// returns octave from 1 to 8
/////////////////////////////////////////////////////////////////////////////
u8 PEDALS_GetOctave(){
   return pedal_config.octave;
}