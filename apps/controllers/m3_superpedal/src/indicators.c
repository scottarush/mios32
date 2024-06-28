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

#undef DEBUG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define NUM_LED_INDICATORS 8

#define FLASH_FAST_FREQ 5
#define FLASH_SLOW_FREQ 2

#define DEFAULT_FLASH_BLIP_FREQ 2
#define FLASH_BLIP_PERCENT_DUTY_CYCLE 10

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

/**
 * Total indicator state is represented by the following struct.
*/
typedef struct indicator_fullstate_s {
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
    * current flash timer.  Set to 0 if not enabled.
    * 32 bits for intermediate precision needed in update function.
   */
   u32 flash_timer_ms;
   /**
    * flash timer freq in Hz
   */
   float flash_timer_freq;
   /*
   * flash_timer_duty_cycle_percent
   */
   u8 flash_timer_duty_cycle_percent;
} indicator_fullstate_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
u8 IND_GetLEDPin(u8 indicatorNum);
void IND_UpdateFlashTimer(indicator_fullstate_t* ptr);
void IND_SetFullIndicatorState(u8 indicatorNum, indicator_state_t state, float freq, u8 dutyCycle);

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
      ptr->flash_timer_freq = 1;  // dummy.  Just don't set to zero
      ptr->flash_timer_duty_cycle_percent = 100;

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

      IND_UpdateFlashTimer(ptr);

      // Now process the temporary state timer.
      if (ptr->timer_ms == 1) {
         // This is a temp state and the timer just expired.
         ptr->timer_ms = 0;
         // Update the state to the target state
         IND_SetIndicatorState(i + 1, ptr->target_state);
      }
      else if (ptr->timer_ms > 0) {
         // Temporary state still continues so decrement the timer
         ptr->timer_ms--;
      }
      // Update the pin in case it changed
      u8 pinNum = IND_GetLEDPin(i + 1);
      MIOS32_BOARD_J10_PinSet(pinNum, ptr->outputState);
   }
}

///////////////////////////////////////////////////////////////////////////
// Helper to set the flash timer
///////////////////////////////////////////////////////////////////////////
void IND_UpdateFlashTimer(indicator_fullstate_t* ptr) {
   if ((ptr->state == IND_ON) || (ptr->state = IND_OFF)) {
      return;
   }
   // Otherwise it's a flashing state
   if (ptr->flash_timer_ms == 1) {
      // Timer expired.  Reset to new value base on outputState
      if (ptr->outputState == 0) {
         // Transition to the on state
         ptr->outputState = 1;
         ptr->flash_timer_ms = (1000 * ptr->flash_timer_duty_cycle_percent) / ptr->flash_timer_freq / 100;

      }
      else {
         // Transition to the off state
         ptr->outputState = 0;
         ptr->flash_timer_ms = (1000 * (100 - ptr->flash_timer_duty_cycle_percent)) / ptr->flash_timer_freq / 100;;
      }
   }
   else {
      // Otherwise timer not expired so just update it
      ptr->flash_timer_ms--;
   }
}
///////////////////////////////////////////////////////////////////////////
// Sets all Indicators to Off.
///////////////////////////////////////////////////////////////////////////
void IND_ClearAll() {
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->state = IND_OFF;
      ptr->timer_ms = 0;
      ptr->target_state = IND_OFF;
      ptr->outputState = 0;
      ptr->flash_timer_ms = 0;
      ptr->flash_timer_freq = 1;
      ptr->flash_timer_duty_cycle_percent = 100;
   }
   MIOS32_BOARD_J10_Set(0);
}

///////////////////////////////////////////////////////////////////////////
// Sets all Indicators to Flash.
// flashFast flag:  0 = flash slow, 1 flash fast
///////////////////////////////////////////////////////////////////////////
void IND_FlashAll(u8 flashFast) {
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->flash_timer_duty_cycle_percent = 50;

      if (flashFast) {
         ptr->state = IND_FLASH_FAST;
         ptr->flash_timer_freq = FLASH_FAST_FREQ;
      }
      else {
         ptr->state = IND_FLASH_SLOW;
         ptr->flash_timer_freq = FLASH_SLOW_FREQ;
      }
      // Set output state to off and expire flash timer, then use IND_UpdateFlashTimer to
      // set the actual flash timer
      ptr->outputState = 0;
      ptr->flash_timer_ms = 1;
      IND_UpdateFlashTimer(ptr);
      // Clear the temporary timer
      ptr->timer_ms = 0;
      ptr->target_state = IND_OFF;
      ptr->outputState = 1;
   }
   MIOS32_BOARD_J10_Set(0);
}
///////////////////////////////////////////////////////////////////////////
// Public function sets a blip indicator with explicit frequency
// indicatorNum:  Number of led starting from left with indicator 1.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetBlipIndicator(u8 indicatorNum,u8 inverse,float frequency) {
   if (inverse){
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_INVERSE_BLIP,frequency,100-FLASH_BLIP_PERCENT_DUTY_CYCLE);
   }
   else{
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_BLIP,frequency,FLASH_BLIP_PERCENT_DUTY_CYCLE);
   }
}

///////////////////////////////////////////////////////////////////////////
// Public function sets a 50% duty cycle flash indicator with explicit frequency
// indicatorNum:  Number of led starting from left with indicator 1.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetFlashIndicator(u8 indicatorNum,float frequency) {
      IND_SetFullIndicatorState(indicatorNum,0,frequency,50);
}

///////////////////////////////////////////////////////////////////////////
// Public function sets the indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// state to set the indicator to.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetIndicatorState(u8 indicatorNum, indicator_state_t state) {
   // set the output pin and init the flash timer
   switch (state) {
   case IND_FLASH_FAST:
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_FAST,FLASH_FAST_FREQ,50);
      break;
   case IND_FLASH_SLOW:
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_SLOW,FLASH_SLOW_FREQ,50);
      break;
   case IND_ON:
      IND_SetFullIndicatorState(indicatorNum,IND_ON,1,100);;
      break;
   case IND_FLASH_BLIP:
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_BLIP,DEFAULT_FLASH_BLIP_FREQ,FLASH_BLIP_PERCENT_DUTY_CYCLE);
      break;
   case IND_FLASH_INVERSE_BLIP:
      IND_SetFullIndicatorState(indicatorNum,IND_FLASH_INVERSE_BLIP,DEFAULT_FLASH_BLIP_FREQ,100-FLASH_BLIP_PERCENT_DUTY_CYCLE);
      break;
   case IND_OFF:
      IND_SetFullIndicatorState(indicatorNum,IND_OFF,1,100);;
      break;
   }
}

///////////////////////////////////////////////////////////////////////////
// Internal helper function sets the indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// state to set the indicator to.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetFullIndicatorState(u8 indicatorNum, indicator_state_t state, float freq, u8 dutyCycle) {
   if (indicatorNum > 8) {
      DEBUG_MSG("Invalid indicator number=%d", indicatorNum);
      return;
   }
#ifdef DEBUG
   DEBUG_MSG("IND_SetIndicatorState:  indicatorNum=%d", indicatorNum);
#endif
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];
   ptr->state = state;

   ptr->flash_timer_freq = freq;
   ptr->flash_timer_duty_cycle_percent = dutyCycle;

   // set the output pin and init the flash timer
   switch (ptr->state) {
   case IND_FLASH_FAST:
   case IND_FLASH_SLOW:
   case IND_FLASH_BLIP:
   case IND_FLASH_INVERSE_BLIP:
      // Need to clear the outputState to force the updateFlashTimer calculation below
      ptr->outputState = 0;
      break;
   case IND_ON:
      ptr->outputState = 1;
      break;
   case IND_OFF:
      ptr->outputState = 0;
      break;
   }
   // For flash modes, the outputState is set to 0.  Now expire flash timer, then use IND_UpdateFlashTimer to
   // update the actual flash timer value.
   ptr->flash_timer_ms = 1;
   IND_UpdateFlashTimer(ptr);
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
void IND_SetTempIndicatorState(u8 indicatorNum, indicator_state_t tempState, u16 duration_ms, indicator_state_t targetState) {
   if (indicatorNum > 8) {

      DEBUG_MSG("Invalid indicator number=%d", indicatorNum);
      return;
   }
#ifdef DEBUG
   DEBUG_MSG("Setting indicator: %d", indicatorNum);
#endif
   // Set the target state to the current state and initialize the timer
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];
   ptr->target_state = targetState;
   ptr->timer_ms = duration_ms;

   // Now set the state to the temporary state
   IND_SetIndicatorState(indicatorNum, tempState);
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
