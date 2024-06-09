// $Id$
/*
 * handler for switches.  All switches are dispatched from
 * application to these functions
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "switches.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


static u8 stompSwitchState[NUM_STOMP_SWITCHES];
static s32 stompSwitchTimestamp[NUM_STOMP_SWITCHES];

static u8 toeSwitchState[NUM_TOE_SWITCHES];
static s32 toeSwitchTimestamp[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10 inputs
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Init(void)
{
   int i = 0;
   for (i = 0; i < NUM_STOMP_SWITCHES;i++) {
      stompSwitchState[i] = 0;
      stompSwitchTimestamp = 0;
   }
   for (i = 0; i < NUM_TOE_SWITCHES;i++) {
      toeSwitchState[i] = 0;
      toeSwitchTimestamp = 0;
   }
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a stomp switch
// stompNum = 1 to NUM_TOE_SWITCHES
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Notify_Toe_Toggle(u8 toeNum,u8 pressed,s32 timestamp){
   if ((toeNum < 1) || (toeNum > NUM_TOE_SWITCHES){
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
void SWITCHES_Notify_Stomp_Toggle(u8 stompNum,u8 pressed,s32 timestamp){
   if ((stompNum < 1) || (stompNum > NUM_STOMP_SWITCHES){
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


