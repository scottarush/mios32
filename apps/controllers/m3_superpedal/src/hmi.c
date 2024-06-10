// $Id$
/*
 * handler for switches.  All switches are dispatched from
 * application to these functions
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
// called at Init to initialize the J10 inputs
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(void)
{
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
   }
   else{
      DEBUG_MSG("Toe switch %d released",toeNum);
   }

   // TODO: Do actual toe switch logic.
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
   }
   else{
      DEBUG_MSG("Stomp switch %d released",stompNum);
   }

   // TODO: Do actual stomp switch logic.
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


