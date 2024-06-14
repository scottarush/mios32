/*
 * HMI.c
 * 
 * Implements the HMI.  Inputs are abstracted from physical hardware in app.c, but
 * LED Indicator hardware abstraction is contained here as indicators are not 
 * accessed from any other file.
 *  
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "hmi.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define NUM_STOMP_SWITCHES 5
#define NUM_TOE_SWITCHES 8

static u8 stompSwitchState[NUM_STOMP_SWITCHES];
static s32 stompSwitchTimestamp[NUM_STOMP_SWITCHES];

static u8 toeSwitchState[NUM_TOE_SWITCHES];
static s32 toeSwitchTimestamp[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(void) {
   int i = 0;
   for (i = 0; i < NUM_STOMP_SWITCHES;i++) {
      stompSwitchState[i] = 0;
      stompSwitchTimestamp[i] = 0;
   }
   for (i = 0; i < NUM_TOE_SWITCHES;i++) {
      toeSwitchState[i] = 0;
      toeSwitchTimestamp[i] = 0;
   }
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a stomp switch
// stompNum = 1 to NUM_TOE_SWITCHES
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyToeToggle(u8 toeNum,u8 pressed,s32 timestamp){
   if ((toeNum < 1) || (toeNum > NUM_TOE_SWITCHES)){
      DEBUG_MSG("Invalid toe switch=%d",toeNum);
      return;
   }
   if (pressed){
      DEBUG_MSG("Toe switch %d pressed",toeNum);
      toeSwitchState[toeNum-1] = 1;
   }
   else{
      DEBUG_MSG("Toe switch %d released",toeNum);
      toeSwitchState[toeNum-1] = 0;
   }

}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a toe switch
// stompNum = 1 to NUM_STOMP_SWITCHES
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyStompToggle(u8 stompNum,u8 pressed,s32 timestamp){
   if ((stompNum < 1) || (stompNum > NUM_STOMP_SWITCHES)){
      DEBUG_MSG("Invalid stomp switch=%d",stompNum);
      return;
   }
   if (pressed){
      DEBUG_MSG("Stomp switch %d pressed",stompNum);
      stompSwitchState[stompNum-1] = 1;
   }
   else{
      DEBUG_MSG("Stomp switch %d released",stompNum);
      stompSwitchState[stompNum-1] = 0;
   }

}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder.
// incrementer:  +/- change since last call.  positive for clockwise, 
//                                            negative for counterclockwise.
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderChange(s32 incrementer){
   DEBUG_MSG("Rotary encoder change: %d",incrementer);
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(u8 pressed,s32 timestamp){
   if (pressed){
      DEBUG_MSG("Back button pressed");      
   }
   else{
      DEBUG_MSG("Back button released");
   }

}
