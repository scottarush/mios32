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

#define SPLIT_LEARNING_FLASH_TIME_MS 500

//----------------------------------------------------------------------------
// Local variable declarations
//----------------------------------------------------------------------------

// Page variables
struct page_s homePage;
struct page_s dialogPage;
struct page_s splitLearningPage;
struct page_s* pCurrentPage;


// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];


// Names of presets for display indeed by zone_preset_ids_t
static const char* zonePresetNames[] = { "Single Zone","Dual Zone","Dual Zone Bass","Triple Zone","Triple Zone Bass" };

// need handled flags for all switches with long press and hold functionality
static u8 backSwitchHandled;

// Current 1ms tick timer from application
static u32 tickTimerMS;

// temporary preset to use in split learning mode
zone_preset_t tempPreset;

// Persisted data to E2
static persisted_hmi_settings_t hmiSettings;

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();

void HMI_SplitLearningPage_TimerCallback();
void HMI_SplitLearningPage_UpdateDisplay();
void HMI_SplitLearningPage_BackCallback();
void HMI_HomePage_EnterCallback();
void HMI_KeyLearningCallback(u8 noteNumber);

s32 HMI_PersistData();

static char* HMI_RenderSplitPointString(zone_preset_t* pPreset, char line[DISPLAY_CHAR_WIDTH + 1]);
void HMI_InitPresetDefaults();
static void HMI_HomePage_UpDownCallback(u8 up);


/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(u8 resetDefaults) {

   //-----------------------------------------------------
   // Local initializations
   backSwitchHandled = 0;
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
   zone_params_t* pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;  // A-1
   pZone->transposeOffset = 0;

   //---------------------------------------
   // Dual_Zone split at middle C
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE];
   pPreset->presetID = DUAL_ZONE;
   pPreset->numZones = 2;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 60;
   pZone->transposeOffset = -24;   // Two octave shift down

   //---------------------------------------
   // Dual_Zone Bass with two left octaves
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE_BASS];
   pPreset->presetID = DUAL_ZONE_BASS;
   pPreset->numZones = 2;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 45;
   pZone->transposeOffset = 0;    // no shift

   //---------------------------------------
   // Triple zone split evenly
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE];
   pPreset->presetID = TRIPLE_ZONE;
   pPreset->numZones = 3;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 50;
   pZone->transposeOffset = -12;   // Single octave down

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 79;
   pZone->transposeOffset = -48;    // four octaves down

   //---------------------------------------
    // Triple zone bass with two left octaves and two top octaves
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE_BASS];
   pPreset->presetID = TRIPLE_ZONE_BASS;
   pPreset->numZones = 3;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 21;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 45;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 85;
   pZone->transposeOffset = 0;

}

/////////////////////////////////////////////////////////////////////////////
// Initializes the HMI pages
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPages() {
   homePage.pageID = PAGE_HOME;


   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pBackButtonCallback = NULL;
   homePage.pEnterButtonCallback = HMI_HomePage_EnterCallback;
   homePage.pTimerCallback = NULL;
   homePage.timerCounter = -1;
   homePage.nextFlashState = FLASH_OFF;
   homePage.pUpDownButtonCallback = HMI_HomePage_UpDownCallback;
   homePage.pBackPage = NULL;

   splitLearningPage.pageID = PAGE_DIALOG;
   splitLearningPage.pBackButtonCallback = HMI_SplitLearningPage_BackCallback;
   splitLearningPage.pEnterButtonCallback = NULL;
   splitLearningPage.pUpDownButtonCallback = NULL;
   splitLearningPage.pTimerCallback = HMI_SplitLearningPage_TimerCallback;
   splitLearningPage.nextFlashState = FLASH_OFF;
   splitLearningPage.timerCounter = -1;
   splitLearningPage.pUpdateDisplayCallback = HMI_SplitLearningPage_UpdateDisplay;
   splitLearningPage.pBackPage = NULL;


   dialogPage.pageID = PAGE_DIALOG;
   dialogPage.pBackButtonCallback = NULL;
   dialogPage.pEnterButtonCallback = NULL;
   dialogPage.pUpDownButtonCallback = NULL;
   dialogPage.pTimerCallback = NULL;
   dialogPage.nextFlashState = FLASH_OFF;
   dialogPage.timerCounter = -1;
   dialogPage.pUpdateDisplayCallback = HMI_DialogPage_UpdateDisplay;
   dialogPage.pBackPage = NULL;

   dialogPageMessage1[0] = '\0';
   dialogPageMessage2[0] = '\0';

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
   if (pCurrentPage->timerCounter > 0) {
      pCurrentPage->timerCounter--;
      if (pCurrentPage->timerCounter == 0) {
         // Timer expired.  Force it to expired so it doesn't trigger at next 1ms tick
         pCurrentPage->timerCounter = -1;
         // And call the callback
         if (pCurrentPage->pTimerCallback != NULL) {
            pCurrentPage->pTimerCallback();
         }
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
   if ((state == SWITCH_RELEASED) && backSwitchHandled) {
      // Clear for next press
      backSwitchHandled = 0;
      return;
   }

   // On a long press go back to home page 
   if (state == SWITCH_LONG_PRESSED) {
      // Go back to home page 
      pCurrentPage = &homePage;
      // And force an update to the current page display
      pCurrentPage->pUpdateDisplayCallback();
      // Set handled to ignore the pending release
      backSwitchHandled = 1;
      return;
   }

   // Otherwise, not a long press nor handled.  Check if there is a registered handler.  If so call it 
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

///////////////////////////////////////////////////
// Recalls a zone preset
void HMI_RecallZonePreset(u8 presetNum) {

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

   zone_preset_t* pPreset = KEYBOARD_GetCurrentZonePreset();
   const char* pName = &zonePresetNames[pPreset->presetID][0];

   HMI_RenderLine(0, pName, RENDER_LINE_CENTER);

   char* pLineTwo = HMI_RenderSplitPointString(pPreset, lineBuffer);
   HMI_RenderLine(1, pLineTwo, RENDER_LINE_LEFT);
}

////////////////////////////////////////////////////////////////////////////
// On Enter from home page, go to the split learning page
////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_EnterCallback() {

   zone_preset_t* pCurrentPreset = KEYBOARD_GetCurrentZonePreset();
   if (pCurrentPreset->numZones == 1) {
      // Just return.  Can't set a split on single zone
      return;
   }
   // Otherwise more than 1 so copy current preset over to the temp
   KEYBOARD_CopyZonePreset(pCurrentPreset, &tempPreset);
   // But clear out the split points by setting all but the first one (which doesn't display) to -1
   for (int i = 1;i < tempPreset.numZones;i++) {
      tempPreset.zoneParams[i].startNoteNum = -1;
   }

   // Set the time for the split view page
   splitLearningPage.timerCounter = SPLIT_LEARNING_FLASH_TIME_MS;

   // Register the callback for the key learning
   KEYBOARD_SetKeyLearningCallback(&HMI_KeyLearningCallback);

   // Got to the split learning Page
   pCurrentPage = &splitLearningPage;

   pCurrentPage->pUpdateDisplayCallback();

}
////////////////////////////////////////////////////////////////////////////
// On Enter from home page, go to the split learning page
////////////////////////////////////////////////////////////////////////////
void HMI_KeyLearningCallback(u8 noteNumber) {
   DEBUG_MSG("HMI_KeyLearningCallback: got note=%d", noteNumber);

   int count = 0;
   // Compute the default offset based on the number of octaves from the left end of they keyboard
   int leftNoteNumber = tempPreset.zoneParams[0].startNoteNum;
   int transposeOffset = -(noteNumber - leftNoteNumber) / 12 + 1;
   transposeOffset *= 12;

   for (int i = 1;i < tempPreset.numZones;i++) {
      zone_params_t* pZoneParams = &tempPreset.zoneParams[i];
      if (pZoneParams->startNoteNum == -1) {
         pZoneParams->startNoteNum = noteNumber;
         pZoneParams->transposeOffset = transposeOffset;
         DEBUG_MSG("tranposeOffset=%d",transposeOffset);
         // Advance the count to find out if we are done
         count++;
         break;
      }
      else {
         count++;
      }
   }
   if (count == tempPreset.numZones - 1) {
      // This is the last key so automatically save the new one
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
      // Now unregister the call by setting it null
      KEYBOARD_SetKeyLearningCallback(NULL);
      // clear out the timer count in the split learning page to avoid another flash
      splitLearningPage.timerCounter = -1;
      // and go back to home page
      pCurrentPage = &homePage;

      pCurrentPage->pUpdateDisplayCallback();
   }
}
////////////////////////////////////////////////////////////////////////////
// Callback for timer expired on split learning page
////////////////////////////////////////////////////////////////////////////
void HMI_SplitLearningPage_TimerCallback() {
   // Toggle the display
   if (pCurrentPage != &splitLearningPage) {
      // error.  shouldn't happen.  expire the time and return
      splitLearningPage.timerCounter = -1;
      splitLearningPage.nextFlashState = FLASH_OFF;
   }
   // otherwise, reset the timer, toggle the display and update
   if (pCurrentPage->nextFlashState == FLASH_STATE_ONE) {
      pCurrentPage->nextFlashState = FLASH_STATE_TWO;
   }
   else {
      pCurrentPage->nextFlashState = FLASH_STATE_ONE;
   }
   pCurrentPage->timerCounter = SPLIT_LEARNING_FLASH_TIME_MS;
   pCurrentPage->pUpdateDisplayCallback();
}

////////////////////////////////////////////////////////////////////////////
// Split learning Page UpdateDisplay
////////////////////////////////////////////////////////////////////////////
void HMI_SplitLearningPage_UpdateDisplay() {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];

   // Render the title
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "Learn %d keys", tempPreset.numZones - 1);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);

   // Then flash the second line between the split point string and blank
   if (pCurrentPage->nextFlashState == FLASH_STATE_ONE) {
      HMI_RenderSplitPointString(&tempPreset, lineBuffer);
   }
   else {
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "________________");
   }
   HMI_RenderLine(1, lineBuffer, RENDER_LINE_CENTER);

}


////////////////////////////////////////////////////////////////////////////
// Back callback on split learning cancels learning.
////////////////////////////////////////////////////////////////////////////
void HMI_SplitLearningPage_BackCallback() {
   // Now unregister the call by setting it null
   KEYBOARD_SetKeyLearningCallback(NULL);
   // clear out the timer count in the split learning page to avoid another flash
   splitLearningPage.timerCounter = -1;
   // and go back to home page
   pCurrentPage = &homePage;
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
#define SPACER_CHAR '_'

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
            if (note_name[j] != '\0'){
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
