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
#include "persist.h"


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#undef DEBUG
#undef HW_DEBUG

//----------------------------------------------------------------------------
// HMI defines and macros
//----------------------------------------------------------------------------


#define LONG_PRESS_TIME_MS 3000
#define DISPLAY_CHAR_WIDTH 16

#define LEARNING_FLASH_TIME_MS 500

#define SPACER_CHAR '_'

//----------------------------------------------------------------------------
// Local variable declarations
//----------------------------------------------------------------------------

// Page variables
struct page_s homePage;
struct page_s dialogPage;
struct page_s splitLearningPage;
struct page_s midiConfigPage;
struct page_s octavePage;
struct page_s* pCurrentPage;

// current flash state.
static flash_state_t flashState;

// global flash timer
static u32 flashTimerCount;

// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];

// Names of presets for display indeed by zone_preset_ids_t
static const char* zonePresetNames[] = { "Single Zone","Dual Zone","Dual Zone Bass","Triple Zone","Triple Zone Bass" };

// Current 1ms tick timer from application
static u32 tickTimerMS;

// temporary preset to use in split learning mode
zone_preset_t tempPreset;

// Persisted data to E2
static persisted_hmi_settings_t hmiSettings;

// Key count for midi config key learning.
static u8 zoneLearningKeyCount;

typedef enum home_page_views_e {
   SPLIT_POINT_VIEW = 0,
   CHANNEL_CONFIG_VIEW = 1
} home_page_views_t;

// Flag for when home page is showing midi channel config
static home_page_views_t homePageView;

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();
void HMI_HomePage_BackCallback();
void HMI_HomePage_EnterCallback();

void HMI_SplitLearningPage_UpdateDisplay();
void HMI_LearningPages_BackCallback();
void HMI_SplitLearningPage_EnterCallback();
int HMI_SplitLearningKeyCallbackHandler(u8);

void HMI_MidiConfigPage_UpdateDisplay();
void HMI_MidiConfigPage_EnterCallback();
int HMI_MidiConfigKeyCallbackHandler(u8);

void HMI_OctavePage_UpdateDisplay();
void HMI_OctavePage_EnterCallback();
int HMI_OctaveKeyCallbackHandler(u8);
int HMI_OctaveKeyCallbackHandler(u8 noteNumber);

void HMI_KeyLearningCallback(u8 noteNumber);
void HMI_FlashDisplay_TimerCallback();

s32 HMI_PersistData();

static char* HMI_RenderSplitPointString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]);
static char* HMI_RenderMidiConfigString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]);
static char* HMI_RenderOctaveOffsetString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]);

void HMI_InitPresetDefaults();
static void HMI_HomePage_UpDownCallback(u8 up);


/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(u8 resetDefaults) {

   //-----------------------------------------------------
   // Local initializations
   tickTimerMS = 0;

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = -1;

   if (resetDefaults == 0) {

      // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
      hmiSettings.serializationID = 0x484D4901;   // 'HMI1'
      valid = PERSIST_ReadBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   }
   // Check if invalid read or we are resetting defaults
   if (valid < 0) {
      DEBUG_MSG("HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

      HMI_InitPresetDefaults();

      HMI_PersistData();
   }
   // Now that settings are init'ed/restored,  synch the HMI

   // Init the display pages
   HMI_InitPages();

   // Clear the display and update
   MIOS32_LCD_Clear();

   // Init the home page display
   HMI_HomePage_UpdateDisplay();
}

/////////////////////////////////////////////////////////////////////////////
//  Helper initializes the zone_preset defaults
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPresetDefaults() {
   // default ports
   u16 defMidiPorts = 0x3033;  // USB and two UARTs

   // Single zone
   zone_preset_t* pPreset = &hmiSettings.zone_presets[SINGLE_ZONE];
   pPreset->numZones = 1;
   pPreset->presetID = SINGLE_ZONE;

   zone_params_t* pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;  // A-1
   pZone->octaveOffset = 0;

   //---------------------------------------
   // Dual_Zone split at middle C
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE];
   pPreset->presetID = DUAL_ZONE;
   pPreset->numZones = 2;

   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->octaveOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 60;
   pZone->octaveOffset = -2;   // Two octave shift down

   //---------------------------------------
   // Dual_Zone Bass with two left octaves
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE_BASS];
   pPreset->presetID = DUAL_ZONE_BASS;
   pPreset->numZones = 2;

   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->octaveOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 45;
   pZone->octaveOffset = 0;    // no shift

   //---------------------------------------
   // Triple zone split evenly
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE];
   pPreset->presetID = TRIPLE_ZONE;
   pPreset->numZones = 3;

   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->octaveOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 50;
   pZone->octaveOffset = -1;   // Single octave down

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 79;
   pZone->octaveOffset = -4;    // four octaves down

   //---------------------------------------
    // Triple zone bass with two left octaves and two top octaves
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE_BASS];
   pPreset->presetID = TRIPLE_ZONE_BASS;
   pPreset->numZones = 3;

   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->octaveOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 45;
   pZone->octaveOffset = 0;

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 85;
   pZone->octaveOffset = 0;

}

/////////////////////////////////////////////////////////////////////////////
// Initializes the HMI pages.  Called every time, not just on ee2 initi
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPages() {

   homePage.pageID = PAGE_HOME;
   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pBackButtonCallback = HMI_HomePage_BackCallback;
   homePage.pEnterButtonCallback = HMI_HomePage_EnterCallback;
   homePage.pUpDownButtonCallback = HMI_HomePage_UpDownCallback;
   homePage.pBackPage = NULL;

   homePageView = SPLIT_POINT_VIEW;

   splitLearningPage.pageID = PAGE_SPLIT_LEARNING;
   splitLearningPage.pBackButtonCallback = HMI_LearningPages_BackCallback;
   splitLearningPage.pEnterButtonCallback = HMI_SplitLearningPage_EnterCallback;
   splitLearningPage.pUpDownButtonCallback = NULL;
   splitLearningPage.pUpdateDisplayCallback = HMI_SplitLearningPage_UpdateDisplay;
   splitLearningPage.pBackPage = NULL;

   midiConfigPage.pageID = PAGE_MIDI_CONFIG;
   midiConfigPage.pBackButtonCallback = HMI_LearningPages_BackCallback;
   midiConfigPage.pEnterButtonCallback = HMI_MidiConfigPage_EnterCallback;
   midiConfigPage.pUpDownButtonCallback = NULL;
   midiConfigPage.pUpdateDisplayCallback = HMI_MidiConfigPage_UpdateDisplay;
   midiConfigPage.pBackPage = &homePage;

   octavePage.pageID = PAGE_OCTAVE;
   octavePage.pBackButtonCallback = HMI_LearningPages_BackCallback;
   octavePage.pEnterButtonCallback = HMI_OctavePage_EnterCallback;
   octavePage.pUpDownButtonCallback = NULL;
   octavePage.pUpdateDisplayCallback = HMI_OctavePage_UpdateDisplay;
   octavePage.pBackPage = &homePage;


   dialogPage.pageID = PAGE_DIALOG;
   dialogPage.pBackButtonCallback = NULL;
   dialogPage.pEnterButtonCallback = NULL;
   dialogPage.pUpDownButtonCallback = NULL;
   dialogPage.pUpdateDisplayCallback = HMI_DialogPage_UpdateDisplay;
   dialogPage.pBackPage = NULL;

   dialogPageMessage1[0] = '\0';
   dialogPageMessage2[0] = '\0';

   flashState = FLASH_OFF;
   flashTimerCount = LEARNING_FLASH_TIME_MS;

   pCurrentPage = &homePage;
}
////////////////////////////////////////////////////////////////////////////
// General tick timer called from application every 1 ms used for general
// HMI timing.
////////////////////////////////////////////////////////////////////////////
void HMI_1msTick() {
   // advance tick timer
   tickTimerMS++;

   // Check for display flash event pending on current page.
   if (flashTimerCount > 0) {
      flashTimerCount--;
      if (flashTimerCount == 0) {
         // Timer expired call the callback
         HMI_FlashDisplay_TimerCallback();
         // and reset for next time
         flashTimerCount = LEARNING_FLASH_TIME_MS;
      }
   }
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Down switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyDownToggle(switch_state_t state) {
   if (state != SWITCH_PRESSED)
      return;
   if (pCurrentPage->pUpDownButtonCallback != NULL) {
      pCurrentPage->pUpDownButtonCallback(0);
   }
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Up switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyUpToggle(switch_state_t state) {
   if (state != SWITCH_PRESSED)
      return;
   if (pCurrentPage->pUpDownButtonCallback != NULL) {
      pCurrentPage->pUpDownButtonCallback(1);
   }
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Enter switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEnterToggle(switch_state_t state) {
   if (state == SWITCH_RELEASED) {
      if (pCurrentPage->pEnterButtonCallback != NULL) {
         pCurrentPage->pEnterButtonCallback();
      }
   }
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(switch_state_t state) {
   if (state == SWITCH_RELEASED) {

      if (pCurrentPage->pBackButtonCallback != NULL) {
         pCurrentPage->pBackButtonCallback();
      }
      // Else if the back page is non-null then pop back to that page
      if (pCurrentPage->pBackPage != NULL) {
         pCurrentPage = pCurrentPage->pBackPage;

         // And force an update to the current page display
         pCurrentPage->pUpdateDisplayCallback();
      }
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

   // Clear flashing state in case last page left it active
   flashState = FLASH_OFF;

   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];
   if (homePageView == SPLIT_POINT_VIEW) {
      // Show the split point view view
      zone_preset_t* pPreset = KEYBOARD_GetCurrentZonePreset();
      const char* pName = &zonePresetNames[pPreset->presetID][0];

      HMI_RenderLine(0, pName, RENDER_LINE_CENTER);

      char* pLineTwo = HMI_RenderSplitPointString(pPreset, lineBuffer);
      HMI_RenderLine(1, pLineTwo, RENDER_LINE_LEFT);
   }
   else {
      // Show the MIDI channel view instead
      zone_preset_t* pCurrentPreset = KEYBOARD_GetCurrentZonePreset();
      const char* pName = &zonePresetNames[pCurrentPreset->presetID][0];
      HMI_RenderLine(0, pName, RENDER_LINE_CENTER);

      HMI_RenderMidiConfigString(pCurrentPreset, lineBuffer);

      HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);

   }
}

////////////////////////////////////////////////////////////////////////////
// On Back from home page, show the midi channels temporarily
////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_BackCallback() {
   // Toggle between the channel and split views
   if (homePageView == SPLIT_POINT_VIEW) {
      homePageView = CHANNEL_CONFIG_VIEW;
   }
   else {
      homePageView = SPLIT_POINT_VIEW;
   }
   HMI_HomePage_UpdateDisplay();
}

////////////////////////////////////////////////////////////////////////////
// On Enter from home page, go to the split learning page
////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_EnterCallback() {

   zone_preset_t* pCurrentPreset = KEYBOARD_GetCurrentZonePreset();
   // Copy current preset over to the temp
   KEYBOARD_CopyZonePreset(pCurrentPreset, &tempPreset);
   // Reset the zone counter to 0
   zoneLearningKeyCount = 0;

   // Register the callback for the key learning
   KEYBOARD_SetKeyLearningCallback(&HMI_KeyLearningCallback);

   // Enable flashing on the to be determined child page
   flashState = FLASH_STATE_ONE;

   if (pCurrentPreset->numZones == 1) {
      // for single zone go directly to MidiConfig page 
      pCurrentPage = &midiConfigPage;
   }
   else {
      // more than one zone so go to split page.
       pCurrentPage = &splitLearningPage;
   }

   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Called when a key is pressed for both split learning and setting the
// midi config.
////////////////////////////////////////////////////////////////////////////
void HMI_KeyLearningCallback(u8 noteNumber) {
   //   DEBUG_MSG("HMI_KeyLearningCallback: got note=%d", noteNumber);

   int done = 0;
   struct page_s* pNextPage = NULL;

   switch (pCurrentPage->pageID) {
   case PAGE_SPLIT_LEARNING:
      done = HMI_SplitLearningKeyCallbackHandler(noteNumber);
      pNextPage = &midiConfigPage;
      break;
   case PAGE_MIDI_CONFIG:
      done = HMI_MidiConfigKeyCallbackHandler(noteNumber);
      pNextPage = &octavePage;
      break;
   case PAGE_OCTAVE:
      done = HMI_OctaveKeyCallbackHandler(noteNumber);
      pNextPage = &homePage;
      break;
   }

   if (done > 0) {
      // This is the last key for the page so automatically save the new one
      for (int i = 0;i < NUM_ZONE_PRESETS;i++) {
         zone_preset_t* pPreset = &hmiSettings.zone_presets[i];
         if (pPreset->presetID == tempPreset.presetID) {
            // This is the one.  copy temp into both this one and set 
            // to the keyboard
            KEYBOARD_CopyZonePreset(&tempPreset, pPreset);
            KEYBOARD_SetCurrentZonePreset(pPreset);
            break;
         }
      }
      if (pNextPage == &homePage) {
         //  unregister the callback by setting it null
         KEYBOARD_SetKeyLearningCallback(NULL);
      }
      // go to the next page
      pCurrentPage = pNextPage;
      pCurrentPage->pUpdateDisplayCallback();
   }
}
////////////////////////////////////////////////////////////////////////////
// Utility function handles a split learning key input.
// returns 0 if still more zones to set, 1 if channel zones complete.
////////////////////////////////////////////////////////////////////////////
int HMI_SplitLearningKeyCallbackHandler(u8 noteNumber) {
   // Increment the counter before seetting the left end since we are setting
   // the next zone
   zoneLearningKeyCount++;

   zone_params_t* pZoneParams = &tempPreset.zoneParams[zoneLearningKeyCount];
   pZoneParams->startNoteNum = noteNumber;
  
   if (zoneLearningKeyCount >= tempPreset.numZones-1) {
      // Return 1 to indicate done
      return 1;
   }
   else {
      // More zones left
      return 0;
   }
}


////////////////////////////////////////////////////////////////////////////
// Callback for page flash timer
////////////////////////////////////////////////////////////////////////////
void HMI_FlashDisplay_TimerCallback() {
   if (flashState == FLASH_OFF) {
      // Flashing disabled.  Just return
      return;
   }

   // Else toggle the flash states and call the update display on the current
   // display
   if (flashState == FLASH_STATE_ONE) {
      flashState = FLASH_STATE_TWO;
   }
   else {
      flashState = FLASH_STATE_ONE;
   }
   // Reset the timer
   flashTimerCount = LEARNING_FLASH_TIME_MS;
   // And update the display
   pCurrentPage->pUpdateDisplayCallback();
}

////////////////////////////////////////////////////////////////////////////
// Split learning Page UpdateDisplay
////////////////////////////////////////////////////////////////////////////
void HMI_SplitLearningPage_UpdateDisplay() {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];

   // Render the title
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "Splits: Set %d", tempPreset.numZones - 1);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);

   // Then flash the second line between the split point string and blank
   if (flashState == FLASH_STATE_ONE) {
      HMI_RenderSplitPointString(&tempPreset, lineBuffer);
   }
   else {
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "________________");
   }
   HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);

}

////////////////////////////////////////////////////////////////////////////
// Midi Config Page display update
////////////////////////////////////////////////////////////////////////////
void HMI_MidiConfigPage_UpdateDisplay() {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];

   // Render the title
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "MIDI Chnl: Set %d", tempPreset.numZones);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);

   // Then flash the second line between the split point string and blank
   if (flashState == FLASH_STATE_ONE) {
      HMI_RenderMidiConfigString(&tempPreset, lineBuffer);
   }
   else {
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "________________");
   }
   HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);
}
////////////////////////////////////////////////////////////////////////////
// Utility function handles a midi config key input
// returns 0 if still more zones to set, 1 if channel zones complete.
////////////////////////////////////////////////////////////////////////////
int HMI_MidiConfigKeyCallbackHandler(u8 noteNumber) {

   // decode the MIDI channel using the below note number array
   // which are MIDI notes from C4 to D6 corresponding to channels 1 to 16
   const int channelArray[] = { 60,62,64,65,67,69,71,72,74,76,77,79,81,83,84,86 };
   int channelNum = -1;
   for (int i = 0;i < 16;i++) {
      if (noteNumber == channelArray[i]) {
         channelNum = i + 1;
      }
   }
   if (channelNum < 0) {
      return 0;
   }
   // Record the channel number in the current zoneCount and increment to the next one
   zone_params_t* pZoneParams = &tempPreset.zoneParams[zoneLearningKeyCount++];
   pZoneParams->midiChannel = channelNum;

   if (zoneLearningKeyCount >= tempPreset.numZones) {
      // Return 1 to indicate done
      return 1;
   }
   else {
      // More zones left
      return 0;
   }

}

////////////////////////////////////////////////////////////////////////////
// Enter on MidiConfigPage goes to OctavePage
////////////////////////////////////////////////////////////////////////////
void HMI_MidiConfigPage_EnterCallback() {
   // Recopy the tempPreset so we keep the current zone splits and midi config
   zone_preset_t* pCurrentPreset = KEYBOARD_GetCurrentZonePreset();
   KEYBOARD_CopyZonePreset(pCurrentPreset, &tempPreset);

   // Reset the zone counter to 0
   zoneLearningKeyCount = 0;

   // reset the flash state and timer so that the next screen renders properly
   flashState = FLASH_STATE_ONE;
   flashTimerCount = LEARNING_FLASH_TIME_MS;

   pCurrentPage = &octavePage;
   pCurrentPage->pUpdateDisplayCallback();
}

////////////////////////////////////////////////////////////////////////////
// Enter on OctavePage goes back to home after turning off flashing as
// Tranpose is the end of the change
////////////////////////////////////////////////////////////////////////////
void HMI_OctavePage_EnterCallback() {
   // Shut off flashing
   flashState = FLASH_OFF;
   //  unregister the key learning callback since Octave is the 
   // end of the chain and the learning wasn't completed.
   KEYBOARD_SetKeyLearningCallback(NULL);

   pCurrentPage = &homePage;
   pCurrentPage->pUpdateDisplayCallback();
}

////////////////////////////////////////////////////////////////////////////
// Split learning Page UpdateDisplay
////////////////////////////////////////////////////////////////////////////
void HMI_OctavePage_UpdateDisplay() {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];

   // Render the title
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "Octave: Set %d", tempPreset.numZones);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);

   // Then flash the second line between the split point string and blank
   if (flashState == FLASH_STATE_ONE) {
      HMI_RenderOctaveOffsetString(&tempPreset, lineBuffer);
   }
   else {
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "________________");
   }
   HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);

}

////////////////////////////////////////////////////////////////////////////
// Utility function handles a midi config key input
// returns 0 if still more zones to set, 1 if channel zones complete.
////////////////////////////////////////////////////////////////////////////
int HMI_OctaveKeyCallbackHandler(u8 noteNumber) {

   // The octave offset is set by the number of white keys above or below middle C
   int offset = 0;
   switch (noteNumber) {
   case 60:   // C4
      offset = 0;
      break;
   case 59:
      offset = -1;
      break;
   case 58:
      offset = -2;
      break;
   case 57:
      offset = -3;
      break;
   case 62:
      offset = 1;
      break;
   case 64:
      offset = 2;
      break;
   case 65:
      offset = 3;
      break;
   default:
      return 0;  // invali octave learning key
   }

   // Record the offset in the zone
   zone_params_t* pZoneParams = &tempPreset.zoneParams[zoneLearningKeyCount++];
   pZoneParams->octaveOffset = offset;
   //  DEBUG_MSG("octave offset=%d",pZoneParams->octaveOffset);
   if (zoneLearningKeyCount >= tempPreset.numZones) {
      // Return 1 to indicate done
      return 1;
   }
   else {
      // More zones left
      return 0;
   }

}
////////////////////////////////////////////////////////////////////////////
// Back callback on either split learning or octave cancels learning and goes to home
// This same function is registered to both pages.
////////////////////////////////////////////////////////////////////////////
void HMI_LearningPages_BackCallback() {
   // unregister the call by setting it null
   KEYBOARD_SetKeyLearningCallback(NULL);

   // and go back to home page
   pCurrentPage = &homePage;
   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Enter on SplitLearningPage goes to midi config page
////////////////////////////////////////////////////////////////////////////
void HMI_SplitLearningPage_EnterCallback() {

   // Recopy the tempPreset so we keep the zone splits and just overwrite
   // the midi channels in the MidiConfigPage
   zone_preset_t* pCurrentPreset = KEYBOARD_GetCurrentZonePreset();
   KEYBOARD_CopyZonePreset(pCurrentPreset, &tempPreset);
   // Reset the zone counter to 0
   zoneLearningKeyCount = 0;

   // reset the flash state and timer so that the next screen renders properly
   flashState = FLASH_STATE_ONE;
   flashTimerCount = LEARNING_FLASH_TIME_MS;

   // Go to midi Config Page and update the page
   pCurrentPage = &midiConfigPage;
   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Dialog page.
// dialogPageTitle, Message1, and Message2 must be filled prior to entry
////////////////////////////////////////////////////////////////////////////
void HMI_DialogPage_UpdateDisplay() {

   HMI_RenderLine(0, dialogPageMessage1, RENDER_LINE_LEFT);
   HMI_RenderLine(1, dialogPageMessage2, RENDER_LINE_LEFT);
}
////////////////////////////////////////////////////////////////////////////
// Up/Down Callback for the home page
////////////////////////////////////////////////////////////////////////////
static void HMI_HomePage_UpDownCallback(u8 up) {
   // On up or down force the view back to split mode
   homePageView = SPLIT_POINT_VIEW;

   zone_preset_t* pPreset = KEYBOARD_GetCurrentZonePreset();

   int nextPresetID = pPreset->presetID;
   if (up) {
      nextPresetID++;
   }
   else {
      nextPresetID--;
   }
   if ((nextPresetID < 0) || (nextPresetID >= NUM_ZONE_PRESETS)) {
      return;  // Already at ends
   }
   // Otherwise set the keyboard driver to the newPreset from the HMI persisted settings
   zone_preset_t* pNewPreset = NULL;
   for (int i = 0;i < NUM_ZONE_PRESETS;i++) {
      pNewPreset = &hmiSettings.zone_presets[i];
      //      DEBUG_MSG("presetid=%d",pNewPreset->presetID);
      if (pNewPreset->presetID == nextPresetID) {
         KEYBOARD_SetCurrentZonePreset(pNewPreset);
         HMI_HomePage_UpdateDisplay();
         return;
      }
   }
   // Shouldn't ever happen
   DEBUG_MSG("HMI_HomePage_UpDownCallback:  ERROR.  Invalid nextPresetID=%d", nextPresetID);
}


////////////////////////////////////////////////////////////////////////////
// Helper renders the split point string
////////////////////////////////////////////////////////////////////////////
static char* HMI_RenderSplitPointString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]) {

#define NOTE_NAME_LENGTH 3

   char note_name[NOTE_NAME_LENGTH + 1];

   // Compute number of space chars without displaying the first zone
   int numSpaces = DISPLAY_CHAR_WIDTH - (pPreset->numZones - 1) * NOTE_NAME_LENGTH;
   numSpaces = numSpaces / pPreset->numZones + 1;

   int index = 0;
   // Put leading spaces in
   for (int i = 0;i < numSpaces;i++) {
      line[index++] = SPACER_CHAR;
   }
   // Now put in the notes
   for (int i = 1;i < pPreset->numZones;i++) {
      zone_params_t* pZoneParams = &pPreset->zoneParams[i];
      if (pZoneParams->startNoteNum < 0) {
         // This is an invalid note number during a learning sessing so render with two ?? marks
         line[index++] = '?';
         line[index++] = '?';
      }
      else {
         // render the note
         KEYBOARD_GetNoteName(pZoneParams->startNoteNum, note_name);
         for (int j = 0;j < NOTE_NAME_LENGTH;j++) {
            if (note_name[j] != '\0') {
               line[index++] = note_name[j];
            }
         }
      }
      // Now put the spaces in between to the next one
      for (int j = 0;j < numSpaces;j++) {
         line[index++] = SPACER_CHAR;
      }
   }
   // Add any more needed spaces to the end
   for (;index < DISPLAY_CHAR_WIDTH;) {
      line[index++] = SPACER_CHAR;
   }
   // Terminate the string
   line[index] = '\0';
   return line;
}
////////////////////////////////////////////////////////////////////////////
// Helper renders the midi config channel string
////////////////////////////////////////////////////////////////////////////
static char* HMI_RenderMidiConfigString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]) {
   // Compute number of space chars between the  MIDI channel nums
   int digitCount = 0;
   for (int i = 0;i < pPreset->numZones;i++) {
      zone_params_t* pZoneParams = &pPreset->zoneParams[i];
      digitCount++;
      if (pZoneParams->midiChannel > 9) {
         digitCount++;
      }
   }
   int numSpaces = DISPLAY_CHAR_WIDTH - digitCount;
   numSpaces = numSpaces / (pPreset->numZones + 1) + 1;
   char midiChString[3];

   int index = 0;
   // Put leading spaces 
   for (int i = 0;i < numSpaces;i++) {
      line[index++] = SPACER_CHAR;
   }
   // Now put in the channel numbers
   for (int i = 0;i < pPreset->numZones;i++) {
      zone_params_t* pZoneParams = &pPreset->zoneParams[i];
      sprintf(midiChString, "%d", pZoneParams->midiChannel);
      for (int j = 0;j < 2;j++) {
         if (midiChString[j] > 0) {
            line[index++] = midiChString[j];
         }
         else {
            // End of midi channel string
            break;
         }
      }
      // Now put the spaces in between to the next one
      for (int j = 0;j < numSpaces;j++) {
         line[index++] = SPACER_CHAR;
      }
   }
   // Add any more needed spaces to the end
   for (;index < DISPLAY_CHAR_WIDTH;) {
      line[index++] = SPACER_CHAR;
   }
   // Terminate the string
   line[index] = '\0';
   return line;
}

////////////////////////////////////////////////////////////////////////////
// Helper renders the octave offset channel string
////////////////////////////////////////////////////////////////////////////
static char* HMI_RenderOctaveOffsetString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]) {
   // Compute number of space chars between the  2-character offsets
   int digitCount = pPreset->numZones * 2;

   int numSpaces = DISPLAY_CHAR_WIDTH - digitCount;
   numSpaces = numSpaces / (pPreset->numZones + 1) + 1;
   char offsetString[3];

   int index = 0;
   // Put leading underlines 
   for (int i = 0;i < numSpaces;i++) {
      line[index++] = SPACER_CHAR;
   }
   // Now put in the octave offsets
   for (int i = 0;i < pPreset->numZones;i++) {
      zone_params_t* pZoneParams = &pPreset->zoneParams[i];
      sprintf(offsetString, "%d", pZoneParams->octaveOffset);
      for (int j = 0;j < 2;j++) {
         if (offsetString[j] > 0) {
            line[index++] = offsetString[j];
         }
         else {
            // End of string
            break;
         }
      }
      // Now put the spaces in between to the next one
      for (int j = 0;j < numSpaces;j++) {
         line[index++] = SPACER_CHAR;
      }
   }
   // Add any more needed spaces to the end
   for (;index < DISPLAY_CHAR_WIDTH;) {
      line[index++] = SPACER_CHAR;
   }
   // Terminate the string
   line[index] = '\0';
   return line;
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
