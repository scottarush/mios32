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
#define NUM_LED_INDICATORS 8

static u8 stompSwitchState[NUM_STOMP_SWITCHES];
static s32 stompSwitchTimestamp[NUM_STOMP_SWITCHES];

static u8 toeSwitchState[NUM_TOE_SWITCHES];
static s32 toeSwitchTimestamp[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10B inputs
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

   // Set LED outputs to push-pull on J10B, upper 8 outputs of J10
   for(i=8;i < NUM_LED_INDICATORS+8;i++){
      MIOS32_BOARD_J10_PinInit(i,MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
   }
   MIOS32_BOARD_J10_Set(0);
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

   for(int i=0;i < NUM_TOE_SWITCHES;i++){
      HMI_SetLEDIndicator(i+1,toeSwitchState[i]);
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


///////////////////////////////////////////////////////////////////////////
// function to set/clear the LED indicators.
// indicatorNum:  Number of led starting from left with indicator 1.
// state = 1 for on, 0 for off
//
/////////////////////////////////////////////////////////////////////////////
void HMI_SetLEDIndicator(u8 indicatorNum,u8 state){
   if (indicatorNum > NUM_LED_INDICATORS){
      DEBUG_MSG("Invalid indicator number: %d",indicatorNum);
      return;
   }
   // Compute pinNum from indicator num on J10B
   u8 pinNum = 0;
   switch(indicatorNum){
      case 1:
         pinNum = 15;
      break;
      case 2:
         pinNum = 13;
      break;
      case 3:
         pinNum = 11;
      break;
      case 4:
         pinNum = 9;
      break;
      case 5:
         pinNum = 14;
      break;
      case 6:
         pinNum = 12;
      break;
      case 7:
         pinNum = 10;
      break;
      case 8:
         pinNum = 8;
      break;
   }
   MIOS32_BOARD_J10_PinSet(pinNum,state);
}

//////////////////////////////////////////////////////////////////////////
// function to get the LED indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// returns 0 for off, 1 for on
/////////////////////////////////////////////////////////////////////////////
u8 HMI_GetLEDIndicator(u8 indicatorNum){
      if (indicatorNum > NUM_LED_INDICATORS){
      DEBUG_MSG("Invalid indicator number: %d",indicatorNum);
      return;
   }
   // Compute pinNum from indicator num on J10B
   u8 pinNum = 0;
   switch(indicatorNum){
      case 1:
         pinNum = 15;
      break;
      case 2:
         pinNum = 13;
      break;
      case 3:
         pinNum = 11;
      break;
      case 4:
         pinNum = 9;
      break;
      case 5:
         pinNum = 14;
      break;
      case 6:
         pinNum = 12;
      break;
      case 7:
         pinNum = 10;
      break;
      case 8:
         pinNum = 8;
      break;
   }
   MIOS32_BOARD_J10_PinGet(pinNum);
}