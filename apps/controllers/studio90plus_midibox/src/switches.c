// $Id$
/*
 * Functions to read Studio 90 switches connected through J10.
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "switches.h"

#define DEBOUNCE_COUNT 5
#define SWITCH_PRESSED 1
#define SWITCH_RELEASED 0

void SWITCHPressed(u8);
void SWITCHReleased(u8);

static u8 lastSwitchState[NUM_STUDIO90_SWITCHES];
static u8 debounceCount[NUM_STUDIO90_SWITCHES];
static u8 switchState[NUM_STUDIO90_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10 inputs
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Init(void)
{
  // initialize all J10 pins to inputs and clear debounce arrays
  int pin;
  for(pin=0; pin < 16; ++pin) {
    MIOS32_BOARD_J10_PinInit	(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);
    switchState[pin] = SWITCH_RELEASED;
    debounceCount[pin] = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////
// called periodically to read and debounce the state of the switches
// call rate should be at expected debounce .
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Read(){
  u8 newPinState;
  u8 pin;

  newPinState = 0;
  
  for(pin=0; pin < NUM_STUDIO90_SWITCHES; pin++) {
    newPinState = MIOS32_BOARD_J10_PinGet(pin);  

    // Was switch pressed on last cycle?
    if (lastSwitchState[pin] > 0){
      if (newPinState > 0){
        // Still pressed.  Handle debounce if not yet handled.
        if (switchState[pin]!= SWITCH_PRESSED) {
          // See if count exceeded debounce threshold
          if (debounceCount[pin] == DEBOUNCE_COUNT) {
            // this is a valid transition so change to PRESSED
            switchState[pin] = SWITCH_PRESSED;
            // Call pressed handler
            SWITCHPressed(pin);

          }
          else{
            // Debounce count not yet exceeded
            debounceCount[pin] = debounceCount[pin] + 1;
          }
        }
      }
      else{
          // Switch reporting released
          if (switchState[pin] == SWITCH_PRESSED){
            // set state back to released
            switchState[pin] = SWITCH_RELEASED;
            // Call release handler
            SWITCHReleased(pin);
            // reset debounce count for next time
            debounceCount[pin] = 0;
          }
          else{
            // Invalid release before debug count.  Reset counter and ignore
            debounceCount[pin] = 0;
          }
        
      }     
    }   // lastSwitchState > 0
    // Transfer last state
    lastSwitchState[pin] = newPinState;
  }  // for
}

void SWITCHPressed(u8 switchIndex){
  // TODO.  Switch case of functional switch press behavior.
}

void SWITCHReleased(u8 switchIndex){
  // TODO.  Switch case of functional switch release behavior.
}
