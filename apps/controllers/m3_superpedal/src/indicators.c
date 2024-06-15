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

#define FLASH_FAST_TIME_MS 100;
#define FLASH_SLOW_TIME_MS 500;


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
   /**
    * Current state of LED pin.  1=ON, 0=OFF
   */
   u8 outputState;
   /**
    * flash timer in ms.  Upon expiration, switch to opposite state and reset.
   */
   u16 flash_timer_ms;
} indicator_fullstate_t;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
indicator_fullstate_t indicator_states[NUM_LED_INDICATORS];


/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10B inputs
/////////////////////////////////////////////////////////////////////////////
void IND_Init() {

   // Set LED outputs to push-pull on J10B and initialize each indicator_state element
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      MIOS32_BOARD_J10_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->state = IND_OFF;
      ptr->timer_ms = 0;
      ptr->target_state = IND_OFF;
      ptr->outputState = 0;
      ptr->flash_timer_ms = 0;
   }
   // Set all LEDs to off in the upper 8 bits of J10.
   MIOS32_BOARD_J10_Set(0);
}

///////////////////////////////////////////////////////////////////////////
// Called every 1ms by the applicationt to update the state of indicators
///////////////////////////////////////////////////////////////////////////
void IND_1msTick() {
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      indicator_fullstate_t* ptr = &indicator_states[i];
      // For flashing states, process the flash      
      if (ptr->state == IND_FLASH_FAST) {
         if (ptr->flash_timer_ms == 1) {
            ptr->flash_timer_ms = FLASH_FAST_TIME_MS;
            ptr->outputState = (ptr->outputState == 0) ? 1 : 0;
         }
         else {
            ptr->flash_timer_ms--;
         }
      }
      // For flashing states, process the flash      
      else if (ptr->state == IND_FLASH_SLOW) {
         if (ptr->flash_timer_ms == 1) {
            ptr->flash_timer_ms = FLASH_SLOW_TIME_MS;
            ptr->outputState = (ptr->outputState == 0) ? 1 : 0;
         }
         else {
            ptr->flash_timer_ms--;
         }
      }
      // Now process the temporary timer.
      if (ptr->timer_ms == 1) {
         // This is a temp state and the timer just expired.
         ptr->timer_ms = 0;
         // Set the state to the target_state
         ptr->state = ptr->target_state;
       }
      else if (ptr->timer_ms > 0) {
         // Temporary state still continues so decrement the timer
         ptr->timer_ms--;
      }
      // Update the pin in case it changed
      u8 pinNum = IND_GetLEDPin(i+1);
      MIOS32_BOARD_J10_PinSet(pinNum, ptr->outputState);
    }
}

///////////////////////////////////////////////////////////////////////////
// Sets all Indicators to Off.
///////////////////////////////////////////////////////////////////////////
void IND_ClearAll(){
  for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->state = IND_OFF;
      ptr->timer_ms = 0;
      ptr->target_state = IND_OFF;
      ptr->outputState = 0;
      ptr->flash_timer_ms = 0;
   }
   MIOS32_BOARD_J10_Set(0);
}

///////////////////////////////////////////////////////////////////////////
// function to change the indicator state with persistence.
// indicatorNum:  Number of led starting from left with indicator 1.
// state to set the indicator to.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetIndicatorState(u8 indicatorNum, indicator_state_t state) {
   if (indicatorNum > 8) {
      DEBUG_MSG("Invalid indicator number=%d", indicatorNum);
      return;
   }
   DEBUG_MSG("IND_SetIndicatorState:  indicatorNum=%d",indicatorNum);
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];
   ptr->state = state;
   // set the output pin and init the flash timer
   switch (ptr->state) {
   case IND_FLASH_FAST:
      ptr->flash_timer_ms = FLASH_FAST_TIME_MS;
      ptr->outputState = 1;
      break;
   case IND_FLASH_SLOW:
      ptr->flash_timer_ms = FLASH_SLOW_TIME_MS;
      ptr->outputState = 1;
      break;
   case IND_ON:
      ptr->outputState = 1;
      break;
   case IND_OFF:
      ptr->outputState = 0;
      break;
   }
   u8 pinNum = IND_GetLEDPin(indicatorNum);
   MIOS32_BOARD_J10_PinSet(pinNum, ptr->outputState);
}
///////////////////////////////////////////////////////////////////////////
// function to change the indicator state temporarily
// indicatorNum:  Number of led starting from left with indicator 1.
// tempState:  state to set the indicator to.
// duration_ms:  duration of the state in milliseconds
// targetState:  state to set at the end.
/////////////////////////////////////////////////////////////////////////////
void IND_SetTempIndicatorState(u8 indicatorNum,indicator_state_t tempState,u16 duration_ms,indicator_state_t targetState){
   if (indicatorNum > 8) {
      DEBUG_MSG("Invalid indicator number=%d", indicatorNum);
      return;
   }
   DEBUG_MSG("Setting indicator: %d",indicatorNum);
   // Set the target state to the current state and initialize the timer
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];
   ptr->target_state = targetState;
   ptr->timer_ms = duration_ms;

   // Now set the state to the temporary state
   IND_SetIndicatorState(indicatorNum,tempState);
}

//////////////////////////////////////////////////////////////////////////
// function to get the LED indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// returns 0 for off, 1 for on
/////////////////////////////////////////////////////////////////////////////
indicator_state_t IND_GetIndicatorState(u8 indicatorNum) {
   if (indicatorNum > NUM_LED_INDICATORS) {
      DEBUG_MSG("Invalid indicator number: %d", indicatorNum);
      return 0;
   }
   // get pin number
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];

   return ptr->state;
}

//////////////////////////////////////////////////////////////////////////
// Helper function returns the pin for a sepcific indicator
// indicatorNum:  Number of led starting from left with indicator 1.
// returns pin number on J10.
//////////////////////////////////////////////////////////////////////////
u8 IND_GetLEDPin(u8 indicatorNum) {
   switch (indicatorNum) {
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
