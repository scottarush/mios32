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
// Global display Page variable declarations
//----------------------------------------------------------------------------
struct page_s homePage;
struct page_s dialogPage;
struct page_s* pCurrentPage;


// Buffer for dialog Page Title
char dialogPageTitle[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];


typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;



static switchState_t backSwitchState;
static switchState_t enterSwitchState;
static switchState_t upSwitchState;
static switchState_t downSwitchState;

// Persisted data to E2
static persisted_hmi_settings_t hmiSettings;

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();

s32 HMI_PersistData();

u8 HMI_DebounceSwitchChange(switchState_t* pState, u8 pressed, s32 timestamp);

/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(u8 resetDefaults) {
   int i = 0;
 
   backSwitchState.switchState = 0;
   backSwitchState.handled = 0;

  pCurrentPage = &homePage;

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = -1;

   if (resetDefaults == 0) {

      // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
      hmiSettings.serializationID = 0x484D4901;   // 'HMI1'
   //   valid = PERSIST_ReadBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   }

   if (valid < 0) {
      DEBUG_MSG("HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

//      HMI_PersistData();
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

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Down switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyDownToggle(u8 pressed, s32 timestamp) {
   if (pressed)
      HMI_RenderLine(0,"Down Switch Pressed",RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0,"Down Switch Released",RENDER_LINE_CENTER);   
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Up switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyUpToggle(u8 pressed, s32 timestamp) {
  if (pressed)
      HMI_RenderLine(0,"Up Switch Pressed",RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0,"Up Switch Released",RENDER_LINE_CENTER);   
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Enter switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEnterToggle(u8 pressed, s32 timestamp) {
  if (pressed)
      HMI_RenderLine(0,"Enter Switch Pressed",RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0,"Enter Switch Released",RENDER_LINE_CENTER);   
}

////////////////////////////////////////////////////////////////////////////
// Called on a change in the Back switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyBackToggle(u8 pressed, s32 timestamp) {
  if (pressed)
      HMI_RenderLine(0,"Back Switch Pressed",RENDER_LINE_CENTER);
   else
      HMI_RenderLine(0,"Back Switch Released",RENDER_LINE_CENTER);   

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

   HMI_RenderLine(0, "Display Test Line 0", RENDER_LINE_RIGHT);

   HMI_RenderLine(1, "Display Test Line 1", RENDER_LINE_RIGHT);

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
