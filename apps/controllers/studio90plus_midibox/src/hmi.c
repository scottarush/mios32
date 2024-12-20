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


//----------------------------------------------------------------------------
// Page variable declarations
//----------------------------------------------------------------------------
struct page_s homePage;
struct page_s dialogPage;
struct page_s viewSplitPointsPage;
struct page_s midiChannelsPage;
struct page_s* pCurrentPage;


// Buffer for dialog Page Title
char dialogPageTitle[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];


// Switch types and buffers
typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;

static switchState_t backSwitchState;
static switchState_t enterSwitchState;
static switchState_t upSwitchState;
static switchState_t downSwitchState;

// Names of presets for display indeed by zone_preset_ids_t
static const char* zonePresetNames[] = { "Single Zone","Dual Zone","Dual Zone Bass","Triple Zone","Triple Zone Bass" };



// Persisted data to E2
static persisted_hmi_settings_t hmiSettings;

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();

s32 HMI_PersistData();

char * HMI_RenderSplitPointString(zone_preset_t * pPreset,char line[DISPLAY_CHAR_WIDTH+1]);
void HMI_InitPresetDefaults();

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(u8 resetDefaults) {

   backSwitchState.switchState = 0;
   backSwitchState.handled = 0;

   pCurrentPage = &homePage;

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

   // Init the display pages now that settings are restored
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
   pZone->startNoteNum = 1;
   pZone->transposeOffset = 0;

   //---------------------------------------
   // Dual_Zone split at middle C
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE];
   pPreset->numZones = 2;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 1;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 60;
   pZone->transposeOffset = 0;

   //---------------------------------------
   // Dual_Zone Bass with two left octaves
   pPreset = &hmiSettings.zone_presets[DUAL_ZONE_BASS];
   pPreset->numZones = 2;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 1;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 25;
   pZone->transposeOffset = 0;

   //---------------------------------------
   // Triple zone split evenly
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE];
   pPreset->numZones = 3;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 1;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 29;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 58;
   pZone->transposeOffset = 0;

   //---------------------------------------
    // Triple zone bass with two left octaves and two top octaves
   pPreset = &hmiSettings.zone_presets[TRIPLE_ZONE_BASS];
   pPreset->numZones = 3;
   pZone = &pPreset->zoneParams[0];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 1;
   pZone->startNoteNum = 1;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[1];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 2;
   pZone->startNoteNum = 25;
   pZone->transposeOffset = 0;

   pZone = &pPreset->zoneParams[2];
   pZone->midiPorts = defMidiPorts;
   pZone->midiChannel = 3;
   pZone->startNoteNum = 65;
   pZone->transposeOffset = 0;

}

/////////////////////////////////////////////////////////////////////////////
// Initializes the HMI pages
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPages() {
   homePage.pageID = PAGE_HOME;

   char* pHomeTitle = { "Studio90 Midibox" };
   homePage.pPageTitle = pHomeTitle;
   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pBackButtonCallback = NULL;
   homePage.pBackPage = NULL;


   dialogPage.pageID = PAGE_DIALOG;
   dialogPage.pPageTitle = dialogPageTitle;
   dialogPageTitle[0] = '\0';
   dialogPage.pBackButtonCallback = NULL;
   dialogPage.pUpdateDisplayCallback = HMI_DialogPage_UpdateDisplay;
   dialogPage.pBackPage = NULL;

   dialogPageMessage1[0] = '\0';
   dialogPageMessage2[0] = '\0';
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Down switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyDownToggle(switch_state_t state) {
   if (downSwitchState.switchState == SWITCH_RELEASED){
      // TODO
   }
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Up switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyUpToggle(switch_state_t state) {
   if (state == SWITCH_PRESSED)
      HMI_RenderLine(0, "Up Switch Pressed", RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0, "Up Switch Released", RENDER_LINE_CENTER);
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Enter switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEnterToggle(switch_state_t state) {
   if (state == SWITCH_PRESSED)
      HMI_RenderLine(0, "Enter Switch Pressed", RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0, "Enter Switch Released", RENDER_LINE_CENTER);
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// param: state state of the switch
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(switch_state_t state) {

   if ((state == SWITCH_LONG_PRESSED) && !backSwitchState.handled) {
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

   char * pLineTwo = HMI_RenderSplitPointString(pPreset,lineBuffer);
   HMI_RenderLine(1,pLineTwo,RENDER_LINE_LEFT);
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

////////////////////////////////////////////////////////////////////////////
// Helper renders the split point string
////////////////////////////////////////////////////////////////////////////
char * HMI_RenderSplitPointString(zone_preset_t * pPreset,char line[DISPLAY_CHAR_WIDTH+1]) {

#define NOTE_NAME_LENGTH 3
   char note_name[NOTE_NAME_LENGTH+1];

   int index = 0;
   int numSpaces = DISPLAY_CHAR_WIDTH - pPreset->numZones*NOTE_NAME_LENGTH;
   numSpaces = numSpaces / pPreset->numZones;

   for(int i=0;i < pPreset->numZones;i++){      
      KEYBOARD_GetNoteName(pPreset->zoneParams[i].startNoteNum,note_name);
      for(int j=0;j < NOTE_NAME_LENGTH;j++){
         line[index++] = note_name[j];
      }
      if (i != pPreset->numZones -1){
         for(int j=0;j < numSpaces;j++){
            line[index++] = '-';
         }
      }
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
