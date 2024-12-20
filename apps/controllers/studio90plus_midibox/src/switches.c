// $Id$
/*
 * Functions to read Studio 90 switches connected through J10.
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "switches.h"
#include "hmi.h"

//#define DEBUG

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define NUM_SWITCHES 4

// Defines for each switch in J10 with both a mask and index into the switches arrays below
#define SWITCH_UP_BIT_MASK 0x0001     
#define SWITCH_UP_INDEX 0

#define SWITCH_DOWN_BIT_MASK 0x0004
#define SWITCH_DOWN_INDEX 1

#define SWITCH_ENTER_BIT_MASK 0x0002   
#define SWITCH_ENTER_INDEX 2

#define SWITCH_BACK_BIT_MASK 0x0008 
#define SWITCH_BACK_INDEX 3


// Debounce count as multiple of the read intervals.  Total debounce time = SWITCH_READ_TIME_MS * DEBOUNCE_COUNT
#define DEBOUNCE_COUNT 3

void SWITCHES_SWITCHChanged(u8 switchIndex);

static switch_state_t lastSwitchState[NUM_SWITCHES];
static u8 debounceCount[NUM_SWITCHES];
static switch_state_t switchState[NUM_SWITCHES];
static u32 switchPressTimeStamps[NUM_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the J10 inputs
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Init(void)
{
   // initialize all J10 pins to inputs and clear debounce arrays
   int pin;

   for (pin = 0; pin < NUM_SWITCHES; ++pin) {
      MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PD);
      switchState[pin] = SWITCH_RELEASED;
      lastSwitchState[pin] = SWITCH_RELEASED;
      debounceCount[pin] = 0;
   }
}

/////////////////////////////////////////////////////////////////////////////
// called periodically to read and debounce the state of the switches
// call rate should be at expected debounce .
/////////////////////////////////////////////////////////////////////////////
void SWITCHES_Read() {
   // Get the time for use in long press detection
   u32 timestamp = MIOS32_TIMESTAMP_Get();

   // Now read J10f for the raw pull-down pin states
   s32 pinStates = MIOS32_BOARD_J10_Get();

#ifdef DEBUG
   DEBUG_MSG("SWITCHES_Read:  pinStates=0x%X",pinStates);
#endif
   
   // Iterate through the switches
   u32 index = 0;
   u32 mask = 0;
   switch_state_t newSwitchState = SWITCH_RELEASED;
   for (index = 0;index < NUM_SWITCHES; index++) {
      switch(index){
         case SWITCH_UP_INDEX:
            mask = SWITCH_UP_BIT_MASK;
            break;
        case SWITCH_DOWN_INDEX:
            mask = SWITCH_DOWN_BIT_MASK;
            break;
        case SWITCH_ENTER_INDEX:
            mask = SWITCH_ENTER_BIT_MASK;
            break;
        case SWITCH_BACK_INDEX:
            mask = SWITCH_BACK_BIT_MASK;
            break;
         default:
            DEBUG_MSG("SWITCHES_Read:  ERROR Invalid switch index=%d",index);
            return;
      }      
      // Compute the state as a pull down
      newSwitchState = ((pinStates & mask) > 0 ? SWITCH_PRESSED : SWITCH_RELEASED);

   #ifdef DEBUG
      DEBUG_MSG("SWITCHES_Read:  index=%d state=%d",index,newSwitchState);
   #endif

      // Was switch pressed on last cycle?
      if (lastSwitchState[index] == SWITCH_PRESSED) {
         if (newSwitchState == SWITCH_PRESSED) {
            // Still pressed.  Handle debounce if not yet handled.
            if (switchState[index] != SWITCH_PRESSED) {
               // See if count exceeded debounce threshold
               if (debounceCount[index] == DEBOUNCE_COUNT) {
                  // this is a valid transition so change to PRESSED
                  switchState[index] = SWITCH_PRESSED;
                  // Call pressed handler
                  SWITCHES_SWITCHChanged(index);

               }
               else {
                  // Debounce count not yet exceeded
                  debounceCount[index] = debounceCount[index] + 1;
               }
            }
         }
         else {
            // Switch released
            if (switchState[index] == SWITCH_PRESSED) {
               // set state back to released
               switchState[index] = SWITCH_RELEASED;
               // Call release handler
               SWITCHES_SWITCHChanged(index);
               // reset debounce count for next time
               debounceCount[index] = 0;
            }
            else {
               // Invalid release before press debounce counter expired.  Reset counter and ignore
               debounceCount[index] = 0;
            }

         }
      }   // lastSwitchState > 0
      if (lastSwitchState[index] == SWITCH_RELEASED){
         // initialize press.  save the timestamp for use in press hold computation
         switchPressTimeStamps[index] = timestamp;
      }
      // Transfer last state
      lastSwitchState[index] = newSwitchState;
   }  // for
}

void SWITCHES_SWITCHChanged(u8 switchIndex) {
   switch_state_t state = switchState[switchIndex];
   switch (switchIndex) {
   case SWITCH_BACK_INDEX:
      HMI_NotifyBackToggle(state);
      return;
   case SWITCH_UP_INDEX:
      HMI_NotifyUpToggle(state);
      return;
   case SWITCH_DOWN_INDEX:
      HMI_NotifyDownToggle(state);
      return;
   case SWITCH_ENTER_INDEX:
      HMI_NotifyEnterToggle(state);
      return;
   default:
      // Invalid index
      DEBUG_MSG("Invalid switchIndex=%d", switchIndex);
      return;
   }

#ifdef DEBUG
   DEBUG_MSG("SWITCHES_SWITCHChanged: index=%d timestamp=%d", switchIndex,timestamp);
#endif
}
