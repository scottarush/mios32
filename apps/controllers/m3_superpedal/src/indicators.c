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

#define BRIGHTNESS_PWM_FREQUENCY 100
#define BRIGHTNESS_RAMP_PERCENT_DELTA 5

#define BRIGHTNESS_RAMP_TIME_MS 1000

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
    * current flash timer.  Set to 0 if not enabled.
    * 32 bits for intermediate precision needed in update function.
   */
   u32 flash_timer_ms;

   /**
    * flash output state.  will be AND'ed with brightness to produce the actual outputState
    *LED pin.  1=ON, 0=OFF
   */
   u8 flashOutputState;

   /**
    * current brightness timer.
    * 32 bits for intermediate precision needed in update function.
   */
   u32 brightness_timer_ms;

   /**
    * Brightness ramp mode.
   */
   indicator_ramp_t rampMode;
   /**
    * current ramp timer.  Set to 0 if not enabled.
    * 32 bits for intermediate precision needed in update function.
   */
   u32 ramp_timer_ms;

   /**
    * Current ramp direction.  0=down, 1=up
   */
   u8 rampDirection;

   /*
   * time between brightness increment changes.  This value is used
   * to reset the ramp time.
   */
   u8 brightnessChangeTime_ms;

   /**
    * flash timer freq in Hz
   */
   float flash_timer_freq;
   /*
   * flash_timer_duty_cycle_percent
   */
   u8 flash_timer_duty_cycle_percent;

   /*
   * percent brightness without ramping
   */
   u8 brightness;


   /*
   * brightness during ramping
   */
   u8 outputBrightness;

   /*
   * target Brightness during ramping
   */
   u8 targetBrightness;

   /*
   * the output state computed for the brightness.  Will be AND'ed with the flashOutputState
   */
   u8 brightnessOutputState;

} indicator_fullstate_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
u8 IND_GetLEDPin(u8 indicatorNum);
static void IND_UpdateFlashTimer(indicator_fullstate_t* ptr);
static void IND_UpdateRampTimer(indicator_fullstate_t* ptr);
void IND_UpdateBrightnessTimer(indicator_fullstate_t* ptr);

static void IND_SetFullIndicatorState(u8 indicatorNum, indicator_state_t state, u8 brightness, float flashFreq, u8 flashDutyCycle, indicator_ramp_t rampMode);

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static indicator_fullstate_t indicator_states[NUM_LED_INDICATORS];

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10B inputs
/////////////////////////////////////////////////////////////////////////////
void IND_Init() {

   // Set LED outputs to push-pull on J10B and init all states to OFF
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      MIOS32_BOARD_J10_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->state = IND_OFF;
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
      IND_UpdateBrightnessTimer(ptr);
      IND_UpdateRampTimer(ptr);

      // Now process the temporary state timer.
      if (ptr->timer_ms == 1) {
         // This is a temp state and the timer just expired.
         ptr->timer_ms = 0;
         // Update the state to the target state
         IND_SetIndicatorState(i + 1, ptr->target_state, ptr->brightness, IND_RAMP_NONE);
      }
      else if (ptr->timer_ms > 0) {
         // Temporary state still continues so decrement the timer
         ptr->timer_ms--;
      }
      // Update the pin in case it changed
      u8 pinNum = IND_GetLEDPin(i + 1);
      // And the two outputs
      u8 outputState = ptr->flashOutputState & ptr->brightnessOutputState;
      MIOS32_BOARD_J10_PinSet(pinNum, outputState);
   }
}

///////////////////////////////////////////////////////////////////////////
// Helper to set the flash timer
///////////////////////////////////////////////////////////////////////////
void IND_UpdateFlashTimer(indicator_fullstate_t* ptr) {
   if (ptr->state == IND_ON) {
      ptr->flashOutputState = 1;
      return;
   }
   if (ptr->state == IND_OFF) {
      ptr->flashOutputState = 0;
      return;
   }
   // Otherwise it's a flashing state
   if (ptr->flash_timer_ms == 1) {
      // Timer expired.  Reset to new value base on outputState
      if (ptr->flashOutputState == 0) {
         // Transition to the on state
         ptr->flashOutputState = 1;
         ptr->flash_timer_ms = (1000 * ptr->flash_timer_duty_cycle_percent) / ptr->flash_timer_freq / 100;

      }
      else {
         // Transition to the off state
         ptr->flashOutputState = 0;
         ptr->flash_timer_ms = (1000 * (100 - ptr->flash_timer_duty_cycle_percent)) / ptr->flash_timer_freq / 100;;
      }
   }
   else {
      // Otherwise timer not expired so just update it
      ptr->flash_timer_ms--;
   }
}
///////////////////////////////////////////////////////////////////////////
// Helper to update the brightness timer.
///////////////////////////////////////////////////////////////////////////
void IND_UpdateBrightnessTimer(indicator_fullstate_t* ptr) {
   if (ptr->state == IND_OFF) {
      return;
   }
   // Otherwise drive the brightness PWM onto the output
   if (ptr->brightness_timer_ms == 1) {
      // Timer expired.  Reset to new value base on outputState
      if (ptr->brightnessOutputState == 0) {
         // Transition to the on state
         ptr->brightnessOutputState = 1;
         ptr->brightness_timer_ms = (1000 * ptr->outputBrightness) / (BRIGHTNESS_PWM_FREQUENCY * 100) + 1;

      }
      else {
         // Transition to the off state
         ptr->brightnessOutputState = 0;
         ptr->brightness_timer_ms = (1000 * (100 - ptr->outputBrightness)) / (BRIGHTNESS_PWM_FREQUENCY * 100) + 1;
      }
   }
   else {
      // Otherwise timer not expired so just update it
      ptr->brightness_timer_ms--;
   }
}

///////////////////////////////////////////////////////////////////////////
// Helper to update the brightness ramp timer and brightness value
///////////////////////////////////////////////////////////////////////////
void IND_UpdateRampTimer(indicator_fullstate_t* ptr) {

   if (ptr->ramp_timer_ms <= 1) {
      // Timer expired.  increment or decrement brightness
      s32 increment = BRIGHTNESS_RAMP_PERCENT_DELTA;
      if (!ptr->rampDirection) {
         increment = -increment;
      }
      else {
         ptr->outputBrightness += increment;
      }
      // Limit to 0..100
      if (ptr->outputBrightness < 0) {
         ptr->outputBrightness = 0;
      }
      else if (ptr->outputBrightness > 100) {
         ptr->outputBrightness = 100;
      }
      // Reset brightness targets depending on the ramp mode 
      switch (ptr->rampMode) {
      case IND_RAMP_UP:
         if (ptr->outputBrightness >= ptr->targetBrightness) {
            // Restart ascending sawtooth
            ptr->targetBrightness = ptr->brightness;
            ptr->outputBrightness = 0;
         }
         break;
      case IND_RAMP_DOWN:
         if (ptr->outputBrightness <= ptr->targetBrightness) {
            // restart descending sawtooth
            ptr->outputBrightness = ptr->brightness;
            ptr->targetBrightness = 0;
         }
         break;
      case IND_RAMP_UP_DOWN:
         if (ptr->rampDirection) {
            // We were going up.
            if (ptr->outputBrightness >= ptr->targetBrightness) {
               // Now go down
               ptr->rampDirection = 0;
               ptr->targetBrightness = 0;
               ptr->outputBrightness = ptr->brightness;
            }
         }
         else {
            // We are going down
            if (ptr->outputBrightness == 0) {
               // go back up
               ptr->rampDirection = 1;
               ptr->targetBrightness = ptr->brightness;
            }
            break;

         }
      }
      // Reset the ramp_timer
      u8 numSteps = ptr->brightness/BRIGHTNESS_RAMP_PERCENT_DELTA;
      ptr->ramp_timer_ms = BRIGHTNESS_RAMP_TIME_MS / numSteps;
    //  DEBUG_MSG("IND_UpdateRampTimer:  brightness=%d timer=%d numSteps=%d",ptr->outputBrightness,ptr->ramp_timer_ms,numSteps);
   }

   else {
      // Otherwise timer not expired so just update it
      ptr->ramp_timer_ms--;
   }
}
///////////////////////////////////////////////////////////////////////////
// Sets all Indicators to Off.
///////////////////////////////////////////////////////////////////////////
void IND_ClearAll() {
   for (int i = 0;i < NUM_LED_INDICATORS;i++) {
      indicator_fullstate_t* ptr = &indicator_states[i];
      ptr->state = IND_OFF;
      ptr->target_state = IND_OFF;
      ptr->brightnessOutputState = 0;
      ptr->flashOutputState = 0;
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
      ptr->flashOutputState = 0;
      ptr->flash_timer_ms = 1;
      IND_UpdateFlashTimer(ptr);
      // Clear the temporary timer
      ptr->timer_ms = 0;
      ptr->target_state = IND_OFF;
      ptr->flashOutputState = 1;
   }
   MIOS32_BOARD_J10_Set(0);
}
///////////////////////////////////////////////////////////////////////////
// Public function sets a blip indicator with explicit frequency
// indicatorNum:  Number of led starting from left with indicator 1.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetBlipIndicator(u8 indicatorNum, u8 inverse, float frequency, u8 brightness) {
   if (inverse) {
      IND_SetFullIndicatorState(indicatorNum,
         IND_FLASH_INVERSE_BLIP, brightness,
         frequency, 100 - FLASH_BLIP_PERCENT_DUTY_CYCLE, IND_RAMP_NONE);
   }
   else {
      IND_SetFullIndicatorState(indicatorNum,
         IND_FLASH_BLIP, brightness, frequency, FLASH_BLIP_PERCENT_DUTY_CYCLE, IND_RAMP_NONE);
   }
}

///////////////////////////////////////////////////////////////////////////
// Public function sets a 50% duty cycle flash indicator with explicit frequency
// indicatorNum:  Number of led starting from left with indicator 1.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetFlashIndicator(u8 indicatorNum, float frequency, u8 brightness) {
   IND_SetFullIndicatorState(indicatorNum, 0, brightness, frequency, 50, IND_RAMP_NONE);
}

///////////////////////////////////////////////////////////////////////////
// Public function sets the indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// state to set the indicator to.
//
/////////////////////////////////////////////////////////////////////////////
void IND_SetIndicatorState(u8 indicatorNum, indicator_state_t state, u8 brightness, indicator_ramp_t rampMode) {
   // set the output pin and init the flash timer
   switch (state) {
   case IND_FLASH_FAST:
      IND_SetFullIndicatorState(indicatorNum, IND_FLASH_FAST, brightness, FLASH_FAST_FREQ, 50, rampMode);
      break;
   case IND_FLASH_SLOW:
      IND_SetFullIndicatorState(indicatorNum, IND_FLASH_SLOW, brightness, FLASH_SLOW_FREQ, 50, rampMode);
      break;
   case IND_ON:
      IND_SetFullIndicatorState(indicatorNum, IND_ON, brightness, 1, 100, rampMode);
      break;
   case IND_FLASH_BLIP:
      IND_SetFullIndicatorState(indicatorNum,
         IND_FLASH_BLIP, brightness, DEFAULT_FLASH_BLIP_FREQ, FLASH_BLIP_PERCENT_DUTY_CYCLE, rampMode);
      break;
   case IND_FLASH_INVERSE_BLIP:
      IND_SetFullIndicatorState(indicatorNum, IND_FLASH_INVERSE_BLIP, brightness,
         DEFAULT_FLASH_BLIP_FREQ, 100 - FLASH_BLIP_PERCENT_DUTY_CYCLE, rampMode);
      break;
   case IND_OFF:
      IND_SetFullIndicatorState(indicatorNum, IND_OFF, brightness, 1, 100, rampMode);
      break;
   }
}

///////////////////////////////////////////////////////////////////////////
// Internal helper function sets the indicator state.
// indicatorNum:  Number of led starting from left with indicator 1.
// state to set the indicator to.
// indicatorNum from 1 to 8
// state
// flashFreq:  flash frequency in Hz
// flashDutyCycle: flash duty cycle
/////////////////////////////////////////////////////////////////////////////
void IND_SetFullIndicatorState(u8 indicatorNum, indicator_state_t state, u8 brightness, float flashFreq, u8 flashDutyCycle, indicator_ramp_t rampMode) {
   if (indicatorNum > 8) {
      DEBUG_MSG("Invalid indicator number=%d", indicatorNum);
      return;
   }
#ifdef DEBUG
   DEBUG_MSG("IND_SetIndicatorState:  indicatorNum=%d", indicatorNum);
#endif
   indicator_fullstate_t* ptr = &indicator_states[indicatorNum - 1];

   ptr->state = state;
   ptr->flash_timer_freq = flashFreq;
   ptr->flash_timer_duty_cycle_percent = flashDutyCycle;
   ptr->targetBrightness = brightness;
   // Set current to percent and target for now.  Will be overwrriten if we are ramping.
   ptr->outputBrightness = brightness;
   ptr->rampMode = rampMode;

   // set the output states to zero.  They will get set correctly in the UpdateXXXTimer calls below.
   ptr->flashOutputState = 0;
   ptr->brightnessOutputState = 0;

   // Now set initial brightness states if ramping
   switch (ptr->rampMode) {
   case IND_RAMP_DOWN:
      ptr->targetBrightness = 0;
      ptr->outputBrightness = ptr->brightness;
      break;
   case IND_RAMP_UP:
   case IND_RAMP_UP_DOWN:
      ptr->targetBrightness = ptr->brightness;
      ptr->outputBrightness = 0;
      break;
   }

   //  expire the timers to trigger proper output values in the Update calls below
   ptr->flash_timer_ms = 1;
   ptr->ramp_timer_ms = 1;
   ptr->brightness_timer_ms = 1;

   IND_UpdateFlashTimer(ptr);
   IND_UpdateBrightnessTimer(ptr);
   IND_UpdateRampTimer(ptr);
   // And the two outputs to set the initial state
   u8 outputState = ptr->flashOutputState & ptr->brightnessOutputState;
   MIOS32_BOARD_J10_PinSet(IND_GetLEDPin(indicatorNum), outputState);
}
///////////////////////////////////////////////////////////////////////////
// function to change the indicator state temporarily
// indicatorNum:  Number of led starting from left with indicator 1.
// tempState:  state to set the indicator to.
// duration_ms:  duration of the state in milliseconds
// targetState:  state to set at the end.
/////////////////////////////////////////////////////////////////////////////
void IND_SetTempIndicatorState(u8 indicatorNum, indicator_state_t tempState, u16 duration_ms, indicator_state_t targetState, u8 brightness) {
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
   ptr->brightness = brightness;
   ptr->timer_ms = duration_ms;

   // Now set the state to the temporary state
   IND_SetIndicatorState(indicatorNum, tempState, 100, brightness);
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
