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
#include <string.h>
#include <stdio.h>

#include "hmi.h"
#include "arp.h"
#include "arp_pattern.h"
#include "arp_hmi.h"
#include "pedals.h"
#include "indicators.h"
#include "persist.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#undef DEBUG
#undef HW_DEBUG

//----------------------------------------------------------------------------
// HMI defines and macros
//----------------------------------------------------------------------------


#define LONG_PRESS_TIME_MS 3000
#define DISPLAY_CHAR_WIDTH 20


//----------------------------------------------------------------------------
// Global display Page variable declarations
//----------------------------------------------------------------------------
struct page_s homePage;
struct page_s arpSettingsPage;
struct page_s arpPatternPage;
struct page_s modeGroupPage;
struct page_s dialogPage;
struct page_s* pCurrentPage;


// Buffer for dialog Page Title
char dialogPageTitle[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];

//----------------------------------------------------------------------------
// Toe Switch types and non-persisted variables
//----------------------------------------------------------------------------

// Text for the toe switch index by toeSwitchMode_e
static const char* pToeSwitchModeTitles[] = { "OCTAVE","VOLUME","CHORD","ARP","CHANNEL","UNUSED" };

typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;
static switchState_t toeSwitchState[NUM_TOE_SWITCHES];

// Toe volume levels even split from 5 to 127
static u8 toeVolumeLevels[NUM_TOE_SWITCHES] = { 5,23,41,58,77,93,110,127 };

//----------------------------------------------------------------------------
// Stomp Switch types and non-persisted variables
//----------------------------------------------------------------------------

static switchState_t stompSwitchState[NUM_STOMP_SWITCHES];

static switchState_t backSwitchState;
static switchState_t encoderSwitchState;

// Persisted data to E2
static persisted_hmi_settings_t hmiSettings;

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();
void HMI_HomePage_RotaryEncoderChanged(s8);
void HMI_HomePage_RotaryEncoderSelect();
indicator_id_t HMI_GetStompIndicatorID(stomp_switch_setting_t);

s32 HMI_PersistData();
void HMI_UpdateOctaveIncDecIndicators();
u8 HMI_GetToeVolumeIndex();

u8 HMI_DebounceSwitchChange(switchState_t* pState, u8 pressed, s32 timestamp);

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(u8 resetDefaults) {
   int i = 0;
   for (i = 0; i < NUM_STOMP_SWITCHES;i++) {
      stompSwitchState[i].switchState = 0;
      stompSwitchState[i].switchPressTimeStamp = 0;
      stompSwitchState[i].handled = 0;
   }
   for (i = 0; i < NUM_TOE_SWITCHES;i++) {
      toeSwitchState[i].switchState = 0;
      toeSwitchState[i].switchPressTimeStamp = 0;
      toeSwitchState[i].handled = 0;
   }
   backSwitchState.switchState = 0;
   backSwitchState.handled = 0;

   encoderSwitchState.switchState = 0;
   encoderSwitchState.handled = 0;

   pCurrentPage = &homePage;

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = -1;

   if (resetDefaults == 0) {

      // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
      hmiSettings.serializationID = 0x484D4901;   // 'HMI1'
      valid = PERSIST_ReadBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   }

   if (valid < 0) {
      DEBUG_MSG("HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

      // stomp switch settings 

      // Set deefault TOE mode
      hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;

      HMI_PersistData();
   }
   // Now that settings are init'ed/restored, then synch the HMI

   // Init the display pages now that settings are restored
   HMI_InitPages();

   // Set the indicators to the curren toe switch mode
   HMI_UpdateIndicators();

   // Clear the display and update
   MIOS32_LCD_Clear();

   // Init the home page display
   HMI_HomePage_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Initializes the HMI pages
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPages() {

   char* pHomeTitle = { "---M3 SUPERPEDAL---" };
   homePage.pPageTitle = pHomeTitle;
   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pRotaryEncoderChangedCallback = HMI_HomePage_RotaryEncoderChanged;
   homePage.pRotaryEncoderSelectCallback = HMI_HomePage_RotaryEncoderSelect;
   homePage.pPedalSelectedCallback = NULL;
   homePage.pBackButtonCallback = NULL;
   homePage.pBackPage = NULL;

   char* arpSettingsTitle = { "ARP SETTINGS" };
   arpSettingsPage.pPageTitle = arpSettingsTitle;
   arpSettingsPage.pUpdateDisplayCallback = ARP_HMI_ARPSettingsPage_UpdateDisplay;
   arpSettingsPage.pRotaryEncoderChangedCallback = ARP_HMI_ARPSettingsPage_RotaryEncoderChanged;
   arpSettingsPage.pRotaryEncoderSelectCallback = ARP_HMI_ARPSettingsPage_RotaryEncoderSelected;
   arpSettingsPage.pPedalSelectedCallback = NULL;
   arpSettingsPage.pBackButtonCallback = NULL;
   arpSettingsPage.pBackPage = &homePage;

   char* modeGroupTitle = { "MODE GROUP" };
   modeGroupPage.pPageTitle = modeGroupTitle;
   modeGroupPage.pUpdateDisplayCallback = ARP_HMI_ModeGroupPage_UpdateDisplay;
   modeGroupPage.pRotaryEncoderChangedCallback = ARP_HMI_ModeGroupPage_RotaryEncoderChanged;
   modeGroupPage.pRotaryEncoderSelectCallback = NULL;
   modeGroupPage.pPedalSelectedCallback = NULL;
   modeGroupPage.pBackButtonCallback = NULL;
   modeGroupPage.pBackPage = &homePage;

   char* arpPatternTitle = { "ARP PATTERNS" };
   arpPatternPage.pPageTitle = arpPatternTitle;
   arpPatternPage.pUpdateDisplayCallback = ARP_HMI_ARPPatternPage_UpdateDisplay;
   arpPatternPage.pRotaryEncoderChangedCallback = ARP_HMI_ARPPatternPage_RotaryEncoderChanged;
   arpPatternPage.pRotaryEncoderSelectCallback = NULL;
   arpPatternPage.pPedalSelectedCallback = NULL;
   arpPatternPage.pBackButtonCallback = NULL;
   arpPatternPage.pBackPage = &homePage;

   dialogPage.pPageTitle = dialogPageTitle;
   dialogPageTitle[0] = '\0';
   dialogPage.pBackButtonCallback = NULL;
   dialogPage.pPedalSelectedCallback = NULL;
   dialogPage.pUpdateDisplayCallback = HMI_DialogPage_UpdateDisplay;
   dialogPage.pRotaryEncoderChangedCallback = NULL;
   dialogPage.pRotaryEncoderSelectCallback = NULL;
   dialogPage.pBackPage = NULL;

   dialogPageMessage1[0] = '\0';
   dialogPageMessage2[0] = '\0';
}
/////////////////////////////////////////////////////////////////////////////
// Helper does debounce and long press detection on a switch
// returns 0 if should be ignored.
// returns +1 if change valid
u8 HMI_DebounceSwitchChange(switchState_t* pState, u8 pressed, s32 timestamp) {
   if (pressed) {
      // Transition from release to press. Check if timestamp from last press
      // is greater than debounce interval.
      s32 delta = timestamp - pState->switchPressTimeStamp;
      if (delta > DEBOUNCE_TIME_MS) {
         // It is so this is a valid initial press.
         pState->switchPressTimeStamp = timestamp;
         pState->switchState = 0;
         return 1;
      }
      else {
         // This is a bounce press so ignore it.
         return 0;
      }
   }
   else {
      // Release.  For now, just ignore it
      return 0;
   }
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a toe switch
// toeNum = 1 to NUM_TOE_SWITCHES
// pressed = 1 for switch pressed, 0 = released struct 
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyToeToggle(u8 toeNum, u8 pressed, s32 timestamp) {
   if ((toeNum < 1) || (toeNum > NUM_TOE_SWITCHES)) {
      DEBUG_MSG("HMI_NotifyToeToggle: Invalid toe switch=%d", toeNum);
      return;
   }
   // Debounce the switch. Ignore unless a valid press greater than debounce interval
   u8 valid = HMI_DebounceSwitchChange(&toeSwitchState[toeNum - 1], pressed, timestamp);
   if (!valid) {
      return;
   }


   // process the toe switch
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
      s8 currentOctave = PEDALS_GetOctave();
      switch (toeNum) {
      case 7:
         // Toenum 7 always decrements.  Note that we don't have to check
         // range here.  PEDALS will do that and will call HMI_NotifyOctaveChange iff
         // there was a change.
         PEDALS_SetOctave(currentOctave - 1);
         // Flash the indicator for confirmation, and then shut off
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_OFF, 100);
         break;
      case 8:
         // Toenum 8 always increments
         PEDALS_SetOctave(currentOctave + 1);
         // Flash the indicator for confirmation, and then shut off
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_OFF, 100);
         break;
      default:
         // Toe switch numbers 1 through 6 are fixed from octaves 0 through 5
         PEDALS_SetOctave(toeNum - 1);
         // Note that we don't have to call HMI_UpdateIndicators for 1 through 6 because this will be 
         // called in HMI_NotifyOctaveChange
         break;
      }

      break;
   case TOE_SWITCH_VOLUME:
      // Get the volumeLevel corresponding to the pressed switch
      u8 volumeLevel = toeVolumeLevels[toeNum - 1];
      // Set the volume in PEDALS
      PEDALS_SetVolume(volumeLevel);
      // Update the indicators
      HMI_UpdateIndicators();
      // And update the current page display
      pCurrentPage->pUpdateDisplayCallback();
      // Flash the indicator for confirmation, but then leave on
      IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON, 100);
      break;
   case TOE_SWITCH_CHORD:
      ARP_HMI_HandleChordToeToggle(toeNum, HMI_GetStompIndicatorID(STOMP_SWITCH_CHORD));
      break;
   case TOE_SWITCH_ARP:
      switch (toeNum) {
      case 7:
         // Toe 7 decrements
         PEDALS_SetOctave(PEDALS_GetOctave() - 1);
         break;
      case 8:
         // Toe 8 increments
         PEDALS_SetOctave(PEDALS_GetOctave() + 1);
         break;
      default:
         // switches 1-6 handled by separate module/function
         ARP_HMI_HandleArpToeToggle(toeNum, pressed);
      }
      break;
   default:
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
      DEBUG_MSG("HMI_NotifyStompToggle:  Invalid stomp switch=%d", stompNum);
      return;
   }
   // Debounce the switch. Ignore unless a valid press greater than debounce interval
   u8 valid = HMI_DebounceSwitchChange(&stompSwitchState[stompNum - 1], pressed, timestamp);
   if (!valid) {
      return;
   }

   // Process the change according to the current switch setting
   switch (stompNum) {
   case STOMP_SWITCH_OCTAVE:
      hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;
      break;
   case STOMP_SWITCH_VOLUME:
      hmiSettings.toeSwitchMode = TOE_SWITCH_VOLUME;
      break;
   case STOMP_SWITCH_CHORD:
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_CHORD) {
         // This is a second stomp so toggle Chord mode on/off
         if (ARP_GetArpMode() == ARP_MODE_OFF) {
            ARP_SetArpMode(ARP_MODE_CHORD_PAD);
         }
         else {
            ARP_SetArpMode(ARP_MODE_OFF);
         }
      }
      else {
         // Initial press
         hmiSettings.toeSwitchMode = TOE_SWITCH_CHORD;
         // Set arp to chord mode
         ARP_SetArpMode(ARP_MODE_CHORD_PAD);
         // Set arp disabled so we play chords
         ARP_SetEnabled(0);
      }
      // Set (back) to home page in case it changed
      pCurrentPage = &homePage;
      break;
   case STOMP_SWITCH_ARPEGGIATOR:
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_ARP) {
         // This is a second stomp so toggle the arp on/off
         ARP_SetEnabled(!ARP_GetEnabled());
      }
      else {
         // Initial press
         hmiSettings.toeSwitchMode = TOE_SWITCH_ARP;
         // Set arp to arp mode
         ARP_SetArpMode(ARP_MODE_ONEKEY_CHORD_ARP);
         // TODO - Need a separate setting for Key vs Chord mode and use that setting here

         // Set enabled so arpeggiator running
         ARP_SetEnabled(1);
      }
      // Set (back) to home page in case it changed
      hmiSettings.toeSwitchMode = TOE_SWITCH_ARP;
      // Set (back) to home page
      pCurrentPage = &homePage;
      break;
   case STOMP_SWITCH_MIDI_CHANNEL:
      hmiSettings.toeSwitchMode = TOE_SWITCH_MIDI_CHANNEL;
      // Set (back) to home page
      pCurrentPage = &homePage;
      break;
   default:
      // Invalid.  Just return;
      DEBUG_MSG("HMI_NotifyStompToggle:  Invalid stompNum %d", stompNum);
      return;
   }
   // Update the indicators and the display in case the mode changed.
   HMI_UpdateIndicators();

   // And persist the data in case anything changed
   HMI_PersistData();

   // update the current page display
   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Helper returns the indicator ID associated with a stomp switch to keep
// all settings for which stomp is which in hmi.c
/////////////////////////////////////////////////////////////////////////////
indicator_id_t HMI_GetStompIndicatorID(stomp_switch_setting_t stomp) {
   switch (stomp) {
   case STOMP_SWITCH_OCTAVE:
      return IND_STOMP_1;
   case STOMP_SWITCH_CHORD:
      return IND_STOMP_2;
   case STOMP_SWITCH_MIDI_CHANNEL:
      return IND_STOMP_3;
   case STOMP_SWITCH_ARPEGGIATOR:
      return IND_STOMP_4;
   case STOMP_SWITCH_VOLUME:
      return IND_STOMP_5;
   }
   return IND_STOMP_1;  // Invalid indicator.  This won't happen
}

/////////////////////////////////////////////////////////////////////////////
// Helper to update the toe indicators
/////////////////////////////////////////////////////////////////////////////
void HMI_UpdateIndicators() {
   IND_ClearAll();

   //  process according to toeSwitchMode
   s8 octave = PEDALS_GetOctave();
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_VOLUME:
      IND_SetIndicatorState(HMI_GetToeVolumeIndex(), IND_ON, 100, IND_RAMP_NONE);
      // Set Volume indicator to red
      IND_SetIndicatorColor(HMI_GetStompIndicatorID(STOMP_SWITCH_VOLUME), IND_COLOR_RED);
      IND_SetIndicatorState(HMI_GetStompIndicatorID(STOMP_SWITCH_VOLUME), IND_ON, 100, IND_RAMP_NONE);
      break;
   case TOE_SWITCH_OCTAVE:
      // Octaves are mapped from -2 to 8 with flashing 1 and 6 indicators for the < 0 and > 6
      switch (octave) {
      case -2:
         IND_SetBlipIndicator(1, 0, 1.0, 100);
         break;
      case -1:
         IND_SetBlipIndicator(1, 0, 2.0, 100);
         break;
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
         // Direct indicators
         IND_SetIndicatorState(octave + 1, IND_ON, 100, IND_RAMP_NONE);
         break;
      case 6:
         IND_SetBlipIndicator(6, 0, 1.0, 100);
         break;
      case 7:
         IND_SetBlipIndicator(7, 0, 2.0, 100);
         break;
      case 8:
         IND_SetBlipIndicator(8, 0, 4.0, 100);
         break;
      }
      // Set Octave indicator to Red

      IND_SetIndicatorColor(HMI_GetStompIndicatorID(STOMP_SWITCH_OCTAVE), IND_COLOR_RED);
      IND_SetIndicatorState(HMI_GetStompIndicatorID(STOMP_SWITCH_OCTAVE), IND_ON, 100, IND_RAMP_NONE);
      break;
   case TOE_SWITCH_ARP:
      if (octave < -1) {
         IND_SetBlipIndicator(1, 0, 2.0, 100);
      }
      else if (octave > 6) {
         IND_SetBlipIndicator(8, 0, 2.0, 100);
      }
      else {
         // Call separate function in ARP_HMI to handle toe indicators 1-6 and stomp indicator
         ARP_HMI_UpdateArpStompIndicator(HMI_GetStompIndicatorID(STOMP_SWITCH_ARPEGGIATOR));
      }
      break;
   case TOE_SWITCH_CHORD:
      if (octave < -1) {
         IND_SetBlipIndicator(1, 0, 2.0, 100);
      }
      else if (octave > 6) {
         IND_SetBlipIndicator(8, 0, 2.0, 100);
      }
      else {
         // Call separate function in ARP_HMI to handle toe indicators 1-6 and stomp indicator
         ARP_HMI_UpdateChordStompIndicator(HMI_GetStompIndicatorID(STOMP_SWITCH_CHORD));
      }
      break;
   case TOE_SWITCH_MIDI_CHANNEL:
      IND_SetIndicatorColor(HMI_GetStompIndicatorID(STOMP_SWITCH_MIDI_CHANNEL), IND_COLOR_RED);
      IND_SetIndicatorState(HMI_GetStompIndicatorID(STOMP_SWITCH_MIDI_CHANNEL), IND_ON, 100, IND_RAMP_NONE);      break;
   }

}


/////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder.
// incrementer:  +/- change since last call.  positive for clockwise, 
//                                            negative for counterclockwise.
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderChange(s32 incrementer) {
#ifdef HW_DEBUG
   DEBUG_MSG("HMI_NotifyEncoderChange:  Rotary encoder change: %d", incrementer);
#endif

   // Send the encoder to the page if the callback is non-null 
   if (pCurrentPage->pRotaryEncoderChangedCallback != NULL) {
      pCurrentPage->pRotaryEncoderChangedCallback(incrementer);
   }

}
////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderSwitchToggle(u8 pressed, s32 timestamp) {

   if (pressed) {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyEncoderSwitchToggle switch pressed");
#endif      
      if (encoderSwitchState.handled) {
         return;  // ignore 
      }
   }
   else {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyEncoderSwitchToggle switch released");
#endif          
      // clear handled flag and ignore the release
      encoderSwitchState.handled = 0;
      return;
   }

   encoderSwitchState.handled = 1;
   // Send the encoder press to the page if the callback is non-null
   if (pCurrentPage->pRotaryEncoderSelectCallback != NULL) {
      pCurrentPage->pRotaryEncoderSelectCallback();
   }
   else {
      // Default behavior is back
      if (pCurrentPage->pBackPage != NULL) {
         pCurrentPage = pCurrentPage->pBackPage;
      }
      else {
         pCurrentPage = &homePage;
      }
   }
   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(u8 pressed, s32 timestamp) {

   // TODO:  Add long-press handling, but requires change to app.c architecture to 
   // poll DIN for continued presses.  Right now the below will never detect
   // a long press because this function is only called on switch press and release.

   u8 longPress = 0;

   // Debounce the switch. Ignore unless a valid press greater than debounce interval
   u8 valid = HMI_DebounceSwitchChange(&backSwitchState, pressed, timestamp);
   if (!valid) {
      return;
   }

   if (longPress && !backSwitchState.handled) {
      // Go back to home page 
      pCurrentPage = &homePage;
      // And force an update to the current page display
      pCurrentPage->pUpdateDisplayCallback();
      // Set handled to ignore the pending release
      backSwitchState.handled = 1;
      return;
   }

   // Otherwise, not a long press nor handled.  Check if there is a registered handler.  If so call it 
   if (pCurrentPage->pBackButtonCallback != NULL) {
      pCurrentPage->pBackButtonCallback();
   }
   // If the back page is non-null then transition to that page.
   if (pCurrentPage->pBackPage != NULL) {
      pCurrentPage = pCurrentPage->pBackPage;
      // Update indicators in case they are different now
      HMI_UpdateIndicators();
      // And force an update to the current page display
      pCurrentPage->pUpdateDisplayCallback();
   }
}

/////////////////////////////////////////////////////////////////////////////
// Helper function renders a line and pads out to full display width or
// truncates to width
/////////////////////////////////////////////////////////////////////////////
void HMI_RenderLine(u8 lineNum, const char* pLineText, renderline_justify_t renderMode) {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1] = "";
   u8 leftIndent = 0;
   switch (renderMode) {
   case RENDER_LINE_CENTER:
      if (strlen(pLineText) < DISPLAY_CHAR_WIDTH) {
         leftIndent = (DISPLAY_CHAR_WIDTH - strlen(pLineText) + 1) / 2;
      }
      for (int i = 0;i < leftIndent;i++) {
         strcat(lineBuffer, " ");
      }
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH);
      while (strlen(lineBuffer) < DISPLAY_CHAR_WIDTH) {
         strcat(lineBuffer, " ");
      }
      break;
   case RENDER_LINE_LEFT:
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH);
      while (strlen(lineBuffer) < DISPLAY_CHAR_WIDTH) {
         strcat(lineBuffer, " ");
      }
      break;
   case RENDER_LINE_RIGHT:
      if (strlen(pLineText) < DISPLAY_CHAR_WIDTH) {
         leftIndent = DISPLAY_CHAR_WIDTH - strlen(pLineText);
      }
      for (int i = 0;i < leftIndent;i++) {
         strcat(lineBuffer, " ");
      }
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH);
      break;
   case RENDER_LINE_SELECT:
      // Compute the number of spaces in addition to the selection marker on each end
      u8 spaceCount = 0;
      u8 leftIndent = 0;
      if (strlen(pLineText) < (DISPLAY_CHAR_WIDTH - 2)) {
         // At least one space to add
         spaceCount = DISPLAY_CHAR_WIDTH - strlen(pLineText) - 2;
         leftIndent = spaceCount / 2;
      }
      strcpy(lineBuffer, "<");
      for (int i = 0;i < leftIndent;i++) {
         strcat(lineBuffer, " ");
      }
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH - 2);
      // Compute right side spaces
      spaceCount -= leftIndent;
      for (int i = 0;i < spaceCount;i++) {
         strcat(lineBuffer, " ");
      }
      strcat(lineBuffer, ">");
      break;
   }
#ifdef DEBUG
   // DEBUG_MSG("HMI_RenderLine %d: '%s'", lineNum, lineBuffer);
#endif
   MIOS32_LCD_CursorSet(0, lineNum);
   MIOS32_LCD_PrintString(lineBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// Helper function to clear a line
/////////////////////////////////////////////////////////////////////////////
void HMI_ClearLine(u8 lineNum) {
   HMI_RenderLine(lineNum, "                    ", 0);
}

////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Home page.
////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_UpdateDisplay() {

   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];
   char scratchBuffer[DISPLAY_CHAR_WIDTH + 1];

   ///////////////////////////////////////
   // line 0 based oin toeSwitchMode
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s",
      pToeSwitchModeTitles[hmiSettings.toeSwitchMode]);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);

   // Current octave, volume and midi channel always on line 3
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "Oct:%d Vol:%d Chn:%d",
      PEDALS_GetOctave(), HMI_GetToeVolumeIndex(), PEDALS_GetMIDIChannel());
   HMI_RenderLine(3, lineBuffer, RENDER_LINE_LEFT);

   ///////////////////////////////////////
   // Render lines 1 & 2 based on the current toe switch mode
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
   case TOE_SWITCH_MIDI_CHANNEL:
      // Spacer on line 1
      HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);

      HMI_ClearLine(2);
      break;
   case TOE_SWITCH_CHORD:
   case TOE_SWITCH_ARP:
      // Line 1 is root key and mode for both
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s %s",
         ARP_MODES_GetNoteName(ARP_GetRootKey()), SEQ_SCALE_NameGet(ARP_GetModeScale()));
      HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);

      // Do line 2 specific to Chord for Arp
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_ARP) {
         // Line 2 is pattern name and bpm for ARP mode
         u16 bpm = ARP_GetBPM();
         // Truncate scale/mode name.  Had problems with snprintf for some reason so
         // went to memcpy instead.  
         // TODO: Figure out why snprintf didn't work.
         // const char* scaleName = SEQ_SCALE_NameGet(ARP_GetModeScale());
         // u8 truncLength = 9;
         // u8 nameLength = strlen(scaleName);
         // if (nameLength < truncLength) {
         //    truncLength = nameLength;
         // }
         memcpy(scratchBuffer, ARP_PAT_GetPatternName(ARP_PAT_GetCurrentPatternIndex()), 16);
         scratchBuffer[16] = '\0';

         snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s %d", scratchBuffer, bpm);
         HMI_RenderLine(2, lineBuffer, RENDER_LINE_CENTER);
      }
      else {
         // Just clear line 2 for chord mode 
         HMI_ClearLine(2);
      }
      break;
   default:
#ifdef DEBUG
      DEBUG_MSG("HMI_UpdateDisplay: Invalid toeSwitchMode=%d", hmiSettings.toeSwitchMode);
      return;
#endif
   }

}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on home page.
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderChanged(s8 increment) {
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_MIDI_CHANNEL:
      PEDALS_SetMIDIChannel(PEDALS_GetMIDIChannel() + increment);
      break;
   case TOE_SWITCH_ARP:
      // Go directly to the arp pattern select page
      arpPatternPage.pBackPage = pCurrentPage;
      pCurrentPage = &arpPatternPage;
      break;
   case TOE_SWITCH_CHORD:
      // Go directly to the mode groups page
      modeGroupPage.pBackPage = pCurrentPage;
      pCurrentPage = &modeGroupPage;
      break;
   case TOE_SWITCH_OCTAVE:
      // Change the octave directly
      PEDALS_SetOctave(PEDALS_GetOctave() + increment);
      break;
   case TOE_SWITCH_VOLUME:
      // Change the volume directly
      PEDALS_SetVolume(PEDALS_GetVolume() + increment);
      break;
   default:
      return;
   }
   // update the current page display
   pCurrentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on home page
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderSelect() {
   switch (hmiSettings.toeSwitchMode) {

   case TOE_SWITCH_CHORD:
   case TOE_SWITCH_ARP:
      // Go to arp settings page
      arpSettingsPage.pBackPage = pCurrentPage;
      pCurrentPage = &arpSettingsPage;
      break;
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
   case TOE_SWITCH_MIDI_CHANNEL:
      if (pCurrentPage != &dialogPage) {
         snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "About M3-SuperPedal");
         snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", M3_SUPERPEDAL_VERSION);
         snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", M3_SUPERPEDAL_VERSION_DATE);

         dialogPage.pBackPage = pCurrentPage;
         pCurrentPage = &dialogPage;
      }
      else {
         // Already on about page
         return;
      }
      break;
   default:
      return;
   }
   // update the current page display
   pCurrentPage->pUpdateDisplayCallback();

}
////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Dialog page.
// dialogPageTitle, Message1, and Message2 must be filled prior to entry
////////////////////////////////////////////////////////////////////////////
void HMI_DialogPage_UpdateDisplay() {
   HMI_RenderLine(0, dialogPageTitle, RENDER_LINE_CENTER);

   HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);
   // Render the message lines left so that if used for setting values from the encoder
   // the display doesn't flicker as much.
   HMI_RenderLine(2, dialogPageMessage1, RENDER_LINE_LEFT);
   HMI_RenderLine(3, dialogPageMessage2, RENDER_LINE_LEFT);
}


/////////////////////////////////////////////////////////////////////////////
// Helper to store persisted data 
/////////////////////////////////////////////////////////////////////////////
s32 HMI_PersistData() {
#ifdef DEBUG
   DEBUG_MSG("HMI_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(persisted_hmi_settings_t));
#endif
   s32 valid = PERSIST_StoreBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   if (valid < 0) {
      DEBUG_MSG("HMI_PersistData:  Error persisting setting to EEPROM");
   }
   return valid;
}

/////////////////////////////////////////////////////////////////////////////
// Notifies HMI on an octave change.
// Called by PEDALS_SetOctave whenever the octave is changed.
// Used to automatically synch the Indicators
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyOctaveChange(u8 octave) {
   // Update the Indicators and the current display
   HMI_UpdateIndicators();
   // And update the current display in case it is showing Octave
   pCurrentPage->pUpdateDisplayCallback();
}

// Helper computes the volume level from 1 to 8 for both display and
// indicator updates:
u8 HMI_GetToeVolumeIndex() {
   u8 volume = PEDALS_GetVolume();
   u8 index = 1;
   for (int i = 0;i < sizeof(toeVolumeLevels) - 1;i++) {
      if ((volume >= toeVolumeLevels[i]) && (volume < toeVolumeLevels[i + 1])) {
         break;
      }
      else {
         index++;
      }
   }
   return index;
}