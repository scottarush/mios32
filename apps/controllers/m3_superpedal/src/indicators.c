/*
 * Indicators.c
 * 
 * Implements the LED indicators and provides a functional API for the 
 * HMI to set the indicator "bank" state
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
#include "indicators.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define NUM_LED_INDICATORS 8



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
u8 IND_GetLEDPin(u8 indicatorNum);


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

/**
 * Total indicator state is represented by the following struct.
*/
typedef struct {
   indicator_state_t state;
   /*
   * state transitions to target_state after timer_ms expires
   */   
   indicator_state_t target_state;
   /*
   * If timer_ms != 0, then the current 'state' is temporary and will transition to
   * the target_state once timer_ms decrements to 0.
   */
   u16 timer_ms;
} indicator_status_t;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
indicator_status_t indicator_states[NUM_LED_INDICATORS];


/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10B inputs
/////////////////////////////////////////////////////////////////////////////
void IND_Init() {
   int i = 0;

   // Set LED outputs to push-pull on J10B and initialize each indicator_state element
   for(i=0;i < NUM_LED_INDICATORS;i++){
      MIOS32_BOARD_J10_PinInit(i,MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
      indicator_status_t * ptr = &indicator_states[i];
      ptr->state = OFF;
      ptr->timer_ms = 0;
      ptr->target_state = OFF;
   }
   // Set all LEDs to off in the upper 8 bits of J10.
   MIOS32_BOARD_J10_Set(0);
}

///////////////////////////////////////////////////////////////////////////
// function to set/clear the LED indicators.
// indicatorNum:  Number of led starting from left with indicator 1.
// state = 1 for on, 0 for off
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetIndicator(u8 indicatorNum,u8 state){
   if (indicatorNum > NUM_LED_INDICATORS){
      DEBUG_MSG("Invalid indicator number: %d",indicatorNum);
      return;
   }
   // Compute pinNum from indicator num on J10B
   u8 pinNum = 0;

   MIOS32_BOARD_J10_PinSet(pinNum,state);
}

//////////////////////////////////////////////////////////////////////////
// Helper function returns the pin for a sepcific indicator
// indicatorNum:  Number of led starting from left with indicator 1.
// returns pin number on J10.
//////////////////////////////////////////////////////////////////////////
u8 IND_GetLEDPin(u8 indicatorNum){
   switch(indicatorNum){
      case 1:
         return 15;
      case 2:
         return 13;
      case 3:
         return 11;
      case 4:
         return 9;
      case 5:
         return 14;
      case 6:
         return 12;
      case 7:
         return 10;
      case 8:
         return 8;
   }
   return 255;  // Invalid, but cannot happen
}

//////////////////////////////////////////////////////////////////////////
// function to get the LED indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// returns 0 for off, 1 for on
/////////////////////////////////////////////////////////////////////////////
u8 IND_GetIndicatorState(u8 indicatorNum){
      if (indicatorNum > NUM_LED_INDICATORS){
      DEBUG_MSG("Invalid indicator number: %d",indicatorNum);
      return 0;
   }
   // get pin number
   u8 pinNum = IND_GetLEDPin(indicatorNum);

   return MIOS32_BOARD_J10_PinGet(pinNum);
}