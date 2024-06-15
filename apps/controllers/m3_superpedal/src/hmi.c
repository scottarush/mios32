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
#include "pedals.h"
#include "indicators.h"
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// HMI defines
/////////////////////////////////////////////////////////////////////////////
#define NUM_STOMP_SWITCHES 5
#define NUM_TOE_SWITCHES 8

#define DISPLAY_LINE1_TITLE_STR "M3 SuperPedal"


/////////////////////////////////////////////////////////////////////////////
// Local types and variables
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   NONE_MODE = 0,
   OCTAVE_MODE = 1,
   VOLUME_MODE = 2,
   PRESET_MODE = 3,
   DRUM_PATTERN_MODE = 4
} toe_switch_mode_t;

// The current mode of the toe switches
toe_switch_mode_t toe_switch_mode;

// Modes assigned to stomp switches.
u8 stompSwitchModes[NUM_STOMP_SWITCHES];

static u8 stompSwitchState[NUM_STOMP_SWITCHES];
static s32 stompSwitchTimestamp[NUM_STOMP_SWITCHES];

static u8 toeSwitchState[NUM_TOE_SWITCHES];
static s32 toeSwitchTimestamp[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void HMI_UpdateDisplay();

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

   toe_switch_mode = OCTAVE_MODE;

   stompSwitchModes[4] = OCTAVE_MODE;
   stompSwitchModes[3] = DRUM_PATTERN_MODE;
   stompSwitchModes[2] = NONE_MODE;
   stompSwitchModes[1] = PRESET_MODE;
   stompSwitchModes[0] = VOLUME_MODE;

   // Init the display
   HMI_UpdateDisplay();
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a toe switch
// toeNum = 1 to NUM_TOE_SWITCHES
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyToeToggle(u8 toeNum, u8 pressed, s32 timestamp) {
   if ((toeNum < 1) || (toeNum > NUM_TOE_SWITCHES)) {
      DEBUG_MSG("Invalid toe switch=%d", toeNum);
      return;
   }

   if (!pressed) {
      switch (toe_switch_mode) {
      case OCTAVE_MODE:
         DEBUG_MSG("Setting to octave mode");
         // Set the octave
         PEDALS_SetOctave(toeNum);
         // Clear the indicators to turn off the current one (if any)
         IND_ClearAll();
         // Now set the one for the toe after flashing fast for 500 ms
 //        IND_SetIndicatorState(toeNum,IND_FLASH_FAST);
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 500, IND_ON);
         // And update the display
         HMI_UpdateDisplay();
         break;
      case VOLUME_MODE:
         // TODO
         break;
      default:
      }
   }

   // Save the state although we may not need this
   if (pressed) {
      DEBUG_MSG("Toe switch %d pressed",toeNum);
      toeSwitchState[toeNum - 1] = 1;
   }
   else {
      DEBUG_MSG("Toe switch %d released",toeNum);
      toeSwitchState[toeNum - 1] = 0;
   }

}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a stomp switch
// stompNum = 1 to NUM_STOMP_SWITCHES
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyStompToggle(u8 stompNum, u8 pressed, s32 timestamp) {
   if ((stompNum < 1) || (stompNum > NUM_STOMP_SWITCHES)) {
      DEBUG_MSG("Invalid stomp switch=%d", stompNum);
      return;
   }
   if (pressed) {
      DEBUG_MSG("Stomp switch %d pressed",stompNum);
      stompSwitchState[stompNum - 1] = 1;
   }
   else {
      DEBUG_MSG("Stomp switch %d released", stompNum);
      stompSwitchState[stompNum-1] = 0;
   }
   // Set the toe switch mode
   // TODO
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder.
// incrementer:  +/- change since last call.  positive for clockwise, 
//                                            negative for counterclockwise.
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderChange(s32 incrementer) {
   DEBUG_MSG("Rotary encoder change: %d", incrementer);
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(u8 pressed, s32 timestamp) {
   if (pressed) {
      DEBUG_MSG("Back button pressed");
   }
   else {
      DEBUG_MSG("Back button released");
   }

}

////////////////////////////////////////////////////////////////////////////
// Updates the display.
////////////////////////////////////////////////////////////////////////////
void HMI_UpdateDisplay() {
   MIOS32_LCD_CursorSet(0, 0);  // X, Y
   MIOS32_LCD_PrintString(DISPLAY_LINE1_TITLE_STR);

   switch (toe_switch_mode) {
   case OCTAVE_MODE:
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("Octave Mode");
      MIOS32_LCD_CursorSet(0, 2);
      MIOS32_LCD_PrintFormattedString("Octave=%d", PEDALS_GetOctave());
      break;

   case VOLUME_MODE:
      // TODO;
      break;
   case DRUM_PATTERN_MODE:
      // TODO;
      break;
   case PRESET_MODE:
      // TODO;
      break;
   default:
      DEBUG_MSG("HMI_UpdateDisplay: Invalid toe_switch_mode=%d", toe_switch_mode);
   }
}