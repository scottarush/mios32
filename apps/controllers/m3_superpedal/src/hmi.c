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
#include "midi_presets.h"
#include "indicators.h"
#include <string.h>
#include <stdio.h>

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG
//#undef DEBUG

/////////////////////////////////////////////////////////////////////////////
// HMI defines
/////////////////////////////////////////////////////////////////////////////
#define NUM_STOMP_SWITCHES 5
#define NUM_TOE_SWITCHES 8

#define LONG_PRESS_TIME_MS 3000

#define DISPLAY_CHAR_WIDTH 20

/////////////////////////////////////////////////////////////////////////////
// Display page variables
/////////////////////////////////////////////////////////////////////////////

typedef enum {
   PAGE_HOME = 0,
   PAGE_MAIN = 1,
   PAGE_ASSIGN_TOE_SWITCH_MIDI_PRESET = 2,
   PAGE_ASSIGN_TOE_SWITCH_PATTERN_PRESET = 3,
   PAGE_GEN_MIDI_PRESET_SELECT = 4,
   PAGE_PATTERN_PRESET_SELECT = 5
} pageID_t;

struct page_s {
   pageID_t pageID;
   char* pPageTitle;
   void (*pRotaryEncoderChangedCallback)(s8 increment);
   void (*pRotaryEncoderSelectCallback)();
   void (*pPedalSelectedCallback)(u8 pedalNum);
   struct page_s* pBackPage;
};

struct page_s homePage;
struct page_s mainPage;
struct page_s selectGenMIDIPresetPage;
struct page_s assignToeMIDIPresetPage;
struct page_s assignToePatternPresetPage;

struct page_s* currentPage;
struct page_s* lastPage;

typedef enum {
   MAIN_PAGE_ASSIGN_TOE_MIDI_PRESET = 0,
   MAIN_PAGE_ASSIGN_TOE_PATTERN_PRESET = 1,
   MAIN_PAGE_ASSIGN_STOMP_SWITCH = 2
} main_page_entries_t;
#define NUM_MAIN_PAGE_ENTRIES 3

char* mainPageEntries[] = { "Assign Toe MIDI","Assign Toe Pattern","Assign Stomp Switch" };
u8 lastSelectedMainPageEntry;


/////////////////////////////////////////////////////////////////////////////
// Toe Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   TOE_SWITCH_OCTAVE = 0,
   TOE_SWITCH_GEN_MIDI_PRESETS = 1,
   TOE_SWITCH_PATTERN_PRESETS = 2,
} toeSwitchMode_t;
#define NUM_TOE_SWITCH_MODES 4

// Text for the toe switch
char* pToeSwitchModeTitles[] = { "Octave","General MIDI Presets","Pattern Presets" };

// When > 0, toe selection mode is active
u8 toeSelectionModeActive;
u8 pendingToeSelection;

typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;
switchState_t toeSwitchState[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// Stomp Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   STOMP_SWITCH_UNASSIGNED = 0,
   STOMP_SWITCH_OCTAVE = 1,
   STOMP_SWITCH_MIDI_PRESETS = 2,
   STOMP_SWITCH_PATTERN_PRESETS = 3
} stomp_switch_setting_t;


switchState_t stompSwitchState[NUM_STOMP_SWITCHES];

switchState_t backSwitchState;
switchState_t encoderSwitchState;

/////////////////////////////////////////////////////////////////////////////
// Persisted data to E^2
/////////////////////////////////////////////////////////////////////////////
typedef struct persistedData_s {
   // The current mode of the toe switches
   toeSwitchMode_t toeSwitchMode;

   // Last selected toe switch in each mode
   u8 selectedToeSwitches[NUM_TOE_SWITCH_MODES];

   // presetNumbers for toe switch gen MIDI presets. 0 is unset
   u8 toeSwitchMIDIPresetNumbers[NUM_TOE_SWITCHES];

   // Last selected MIDI preset program number in the assignment dialog
   u8 lastSelectedGenMIDIProgNumber;

   // Midi output to be used for preset editing
   u8 lastSelectedMidiOutput;

   // Midi channel to be used for preset editing
   u8 lastSelectedMidiChannel;

   // stomp switch settings
   stomp_switch_setting_t stompSwitchSetting[NUM_STOMP_SWITCHES];

} persistedData_t;

persistedData_t settings;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void HMI_UpdateDisplay();
void HMI_RenderLine(u8, char*, u8);
void HMI_ClearLine(u8);
void HMI_InitPages();
void HMI_HomePage_RotaryEncoderChanged(s8);
void HMI_MainPage_RotaryEncoderChanged(s8);
void HMI_MainPage_RotaryEncoderSelected();
void HMI_GenMIDISelectPage_RotaryEncoderChanged(s8 increment);
void HMI_GenMIDISelectPage_RotaryEncoderSelected();
void HMI_EditToePresetPage_PedalSelected(u8);
void HMI_HomePage_RotaryEncoderSelect();


/////////////////////////////////////////////////////////////////////////////
// called at Init to initialize the HMI
/////////////////////////////////////////////////////////////////////////////
void HMI_Init(void) {
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

   lastSelectedMainPageEntry = MAIN_PAGE_ASSIGN_TOE_MIDI_PRESET;

   toeSelectionModeActive = 0;
   pendingToeSelection = 0;

   currentPage = &homePage;
   lastPage = currentPage;

   // Restore settings from E^2 if they exist.  If not the initialize to defaults
   // TODO
   u8 valid = 0;
   // valid = PRESETS_GetHMISettings(&settings);
   if (!valid) {
      settings.stompSwitchSetting[4] = STOMP_SWITCH_OCTAVE;
      settings.stompSwitchSetting[3] = STOMP_SWITCH_MIDI_PRESETS;
      settings.stompSwitchSetting[2] = STOMP_SWITCH_PATTERN_PRESETS;
      settings.stompSwitchSetting[1] = STOMP_SWITCH_UNASSIGNED;
      settings.stompSwitchSetting[0] = STOMP_SWITCH_UNASSIGNED;
      settings.toeSwitchMode = TOE_SWITCH_OCTAVE;

      settings.lastSelectedGenMIDIProgNumber = 1;
      settings.lastSelectedMidiOutput = 1;
      settings.lastSelectedMidiChannel = 1;

      settings.selectedToeSwitches[TOE_SWITCH_OCTAVE] = 3;
      settings.selectedToeSwitches[TOE_SWITCH_GEN_MIDI_PRESETS] = 3;
      settings.selectedToeSwitches[STOMP_SWITCH_PATTERN_PRESETS] = 3;
   }

   // Init the display pages now that settings are restored
   HMI_InitPages();

   // Initialize the consumer of the current toe switch mode
   // TODO

   // Clear the display and update
   MIOS32_LCD_Clear();

   // Init the display
   HMI_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Initializes the HMI pages
/////////////////////////////////////////////////////////////////////////////
void HMI_InitPages() {
   homePage.pageID = PAGE_HOME;

   char* pHomeTitle = { "---M3 SuperPedal---" };
   homePage.pPageTitle = pHomeTitle;
   homePage.pRotaryEncoderChangedCallback = &HMI_HomePage_RotaryEncoderChanged;
   homePage.pRotaryEncoderSelectCallback = &HMI_HomePage_RotaryEncoderSelect;
   homePage.pPedalSelectedCallback = NULL;
   homePage.pBackPage = NULL;

   mainPage.pageID = PAGE_MAIN;
   char* pMainTitle = { "  --- Main Menu ---" };
   mainPage.pPageTitle = pMainTitle;
   mainPage.pRotaryEncoderChangedCallback = HMI_MainPage_RotaryEncoderChanged;
   mainPage.pRotaryEncoderSelectCallback = HMI_MainPage_RotaryEncoderSelected;
   mainPage.pPedalSelectedCallback = NULL;
   mainPage.pBackPage = &homePage;

   assignToeMIDIPresetPage.pageID = PAGE_ASSIGN_TOE_SWITCH_MIDI_PRESET;
   char* presetTitle = { "Assign Toe MIDI Pset" };
   assignToeMIDIPresetPage.pPageTitle = presetTitle;
   assignToeMIDIPresetPage.pRotaryEncoderChangedCallback = NULL;
   assignToeMIDIPresetPage.pRotaryEncoderSelectCallback = NULL;
   assignToeMIDIPresetPage.pPedalSelectedCallback = NULL;
   assignToeMIDIPresetPage.pBackPage = &mainPage;


   assignToePatternPresetPage.pageID = PAGE_ASSIGN_TOE_SWITCH_MIDI_PRESET;
   char* patternPresetTitle = { "Assign Toe Ptrn Pset" };
   assignToePatternPresetPage.pPageTitle = patternPresetTitle;
   assignToePatternPresetPage.pRotaryEncoderChangedCallback = NULL;
   assignToePatternPresetPage.pRotaryEncoderSelectCallback = NULL;
   assignToePatternPresetPage.pPedalSelectedCallback = NULL;
   assignToePatternPresetPage.pBackPage = &mainPage;


   selectGenMIDIPresetPage.pageID = PAGE_GEN_MIDI_PRESET_SELECT;
   char* selectPresetTitle = { "Select Gen MIDI Pset" };
   selectGenMIDIPresetPage.pPageTitle = selectPresetTitle;
   selectGenMIDIPresetPage.pRotaryEncoderChangedCallback = HMI_GenMIDISelectPage_RotaryEncoderChanged;
   selectGenMIDIPresetPage.pRotaryEncoderSelectCallback = HMI_GenMIDISelectPage_RotaryEncoderSelected;
   selectGenMIDIPresetPage.pPedalSelectedCallback = NULL;
   selectGenMIDIPresetPage.pBackPage = &assignToeMIDIPresetPage;
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in a toe switch
// toeNum = 1 to NUM_TOE_SWITCHES
// pressed = 1 for switch pressed, 0 = released struct 
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyToeToggle(u8 toeNum, u8 pressed, s32 timestamp) {
   if ((toeNum < 1) || (toeNum > NUM_TOE_SWITCHES)) {
      DEBUG_MSG("Invalid toe switch=%d", toeNum);
      return;
   }

   // Save the state for long press detection although we may not need this
   if (pressed) {
      //DEBUG_MSG("Toe switch %d pressed",toeNum);
      if (toeSwitchState[toeNum - 1].switchState == 0) {
         // Save initial press timestamp         
         toeSwitchState[toeNum - 1].switchPressTimeStamp = timestamp;
      }
      toeSwitchState[toeNum - 1].switchState = 1;
   }
   else {
      //DEBUG_MSG("Toe switch %d released",toeNum);
      toeSwitchState[toeNum - 1].switchState = 0;
   }

   // For now ignore the release
   if (!pressed) {
      return;
   }

   if (toeSelectionModeActive) {
      // Toe selection dialog is up so end the dialog
      // after shutting off the flashing indicators and setting the selected one.
      DEBUG_MSG("Toeselection mode.  Toe #: %d", toeNum);
      IND_ClearAll();
      IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 450, IND_ON);
      toeSelectionModeActive = 0;  // clear to restore selection mode
      // Now process the selection by going to the selection page
      switch (currentPage->pageID) {
      case PAGE_ASSIGN_TOE_SWITCH_MIDI_PRESET:
         // Save the pendingToeSelection
         pendingToeSelection = toeNum;

         // Transition to the selection page
         lastPage = currentPage;
         currentPage = &selectGenMIDIPresetPage;
         HMI_UpdateDisplay();
         break;
      case PAGE_ASSIGN_TOE_SWITCH_PATTERN_PRESET:
         // TODO;
         lastPage = currentPage;
         currentPage = &homePage;
         break;
      }
      return;
   }
   // Otherwise, process a mode change.
   switch (settings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
   #ifdef DEBUG
      DEBUG_MSG("Setting to octave mode");
   #endif
      // Set the octave
      PEDALS_SetOctave(toeNum - 1);
      // Clear the indicators to turn off the current one (if any)
      IND_ClearAll();
      // Now set the one for the toe after flashing fast for 3 cycles
//        IND_SetIndicatorState(toeNum,IND_FLASH_FAST);
      IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 450, IND_ON);
      // And update the display
      HMI_UpdateDisplay();
      break;
   case TOE_SWITCH_GEN_MIDI_PRESETS:
      u8 presetNum = settings.toeSwitchMIDIPresetNumbers[toeNum-1];
      if (presetNum == 0){
         // no preset on this one.  Just flash indicator briefly to provide feedback
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 450, IND_OFF);
         return;
      }
      else{
         // Activate this preset
         u8 errorNum = MIDI_PRESET_ActivateMIDIPreset(presetNum);
         if (errorNum > 0){
            IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 450, IND_ON);
         }
         else{
            IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, 450, IND_OFF);
         }
      }
      break;
   case TOE_SWITCH_PATTERN_PRESETS:
      // TODO
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
      DEBUG_MSG("Invalid stomp switch=%d", stompNum);
      return;
   }
   // Save the state for long press detection although we may not need this
   if (pressed) {
      //DEBUG_MSG("Toe switch %d pressed", stompNum);
      if (stompSwitchState[stompNum - 1].switchState == 0) {
         // Save initial press timestamp         
         stompSwitchState[stompNum - 1].switchPressTimeStamp = timestamp;
      }
      stompSwitchState[stompNum - 1].switchState = 1;
   }
   else {
      //DEBUG_MSG("Toe switch %d released", stompNum);
      stompSwitchState[stompNum - 1].switchState = 0;
   }
   switch(settings.stompSwitchSetting[stompNum]){
      case STOMP_SWITCH_OCTAVE:
         settings.toeSwitchMode = TOE_SWITCH_OCTAVE;
         break;
      case STOMP_SWITCH_MIDI_PRESETS:
         settings.toeSwitchMode = TOE_SWITCH_GEN_MIDI_PRESETS;
         break;
      case STOMP_SWITCH_PATTERN_PRESETS:
         settings.toeSwitchMode = TOE_SWITCH_PATTERN_PRESETS;
         break;
      case STOMP_SWITCH_UNASSIGNED:
         //unassigned on, so just return;
         return;
   }
   // Update the toe switch indicators and the display in case the mode changed.
   IND_ClearAll();
   IND_SetIndicatorState(settings.selectedToeSwitches[settings.toeSwitchMode],IND_ON);
   HMI_UpdateDisplay();
}

/////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder.
// incrementer:  +/- change since last call.  positive for clockwise, 
//                                            negative for counterclockwise.
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderChange(s32 incrementer) {
#ifdef DEBUG
   DEBUG_MSG("Rotary encoder change: %d", incrementer);
#endif

   // Send the encoder to the page if the callback is non-null
   if (currentPage->pRotaryEncoderChangedCallback != NULL) {
      currentPage->pRotaryEncoderChangedCallback(incrementer);
   }

}
////////////////////////////////////////////////////////////////////////////
// Called on a change in the rotary encoder switch
// pressed = 1 for switch pressed, 0 = released
// timestamp
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyEncoderSwitchToggle(u8 pressed, s32 timestamp) {

   if (pressed) {
#ifdef DEBUG
      DEBUG_MSG("HMI_NotifyEncoderSwitchToggle switch pressed");
#endif      
      if (encoderSwitchState.handled) {
         return;  // ignore 
      }
   }
   else {
#ifdef DEBUG
      DEBUG_MSG("HMI_NotifyEncoderSwitchToggle switch released");
#endif          
      // clear handled flag and ignore the release
      encoderSwitchState.handled = 0;
      return;
   }

   encoderSwitchState.handled = 1;

   switch (currentPage->pageID) {
   case PAGE_HOME:
      // On encoder switch initial press go to main page
      lastPage = currentPage;
      currentPage = &mainPage;
      // And force an update to the display
      HMI_UpdateDisplay();
      break;
   default:
      // Send the encoder press to the page if the callback is non-null
      if (currentPage->pRotaryEncoderSelectCallback != NULL) {
         currentPage->pRotaryEncoderSelectCallback();
      }
   }
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
   if (pressed && !backSwitchState.handled) {
#ifdef DEBUG
      DEBUG_MSG("Back button pressed");
#endif
      if (backSwitchState.switchState == 0) {
         // Save initial press timestamp         
         backSwitchState.switchPressTimeStamp = timestamp;
      }
      backSwitchState.switchState = 1;
      // Check if this is a long press
      u32 delay = timestamp - backSwitchState.switchPressTimeStamp;
      if (delay >= LONG_PRESS_TIME_MS) {
         longPress = 1;
      }
   }
   else {
#ifdef DEBUG
      DEBUG_MSG("Back button released");
#endif
      backSwitchState.switchState = 0;
      if (backSwitchState.handled) {
         // Clear handled flag and return to ignore this release
         backSwitchState.handled = 0;
         return;
      }
   }

   if (longPress && !backSwitchState.handled) {
      // Go back to home page 
      lastPage = currentPage;
      currentPage = &homePage;
      // And force an update to the display
      HMI_UpdateDisplay();
      // Set handled to ignore the release
      backSwitchState.handled = 1;
      return;
   }

   // Otherwise, on a release, go back one page
   if (currentPage->pBackPage != NULL) {
      lastPage = currentPage;
      currentPage = currentPage->pBackPage;
      HMI_UpdateDisplay();
   }
}

////////////////////////////////////////////////////////////////////////////
// Updates the display.
////////////////////////////////////////////////////////////////////////////
void HMI_UpdateDisplay() {
   if (lastPage != currentPage) {
      // Clear display
      MIOS32_LCD_Clear();
      // Set to keep from redrawing the entire display next time.
      lastPage = currentPage;
   }
   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(currentPage->pPageTitle);

   char lineBuffer[DISPLAY_CHAR_WIDTH];

   // Now render according to the mode
   u8 selectedToeSwitch = settings.selectedToeSwitches[TOE_SWITCH_GEN_MIDI_PRESETS];
   switch (currentPage->pageID) {
   case PAGE_HOME:
      switch (settings.toeSwitchMode) {
      case TOE_SWITCH_OCTAVE:
         snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s %d", pToeSwitchModeTitles[settings.toeSwitchMode], PEDALS_GetOctave());
         HMI_RenderLine(1, lineBuffer, 0);
         break;
      case TOE_SWITCH_GEN_MIDI_PRESETS:
         HMI_RenderLine(1,pToeSwitchModeTitles[settings.toeSwitchMode],0);
         u8 presetNum = settings.toeSwitchMIDIPresetNumbers[selectedToeSwitch];
         midi_preset_t * preset = MIDI_PRESETS_GetMidiPreset(presetNum);
         if (preset == NULL){
            HMI_RenderLine(1,"Invalid Preset Number",0);
            DEBUG_MSG("Invalid preset number:  %d",presetNum);
         }
         else{
            char * pName = MIDI_PRESETS_GetGenMIDIPresetName(preset->programNumber);
            HMI_RenderLine(1,pName, 0);
         }
         break;
      case TOE_SWITCH_PATTERN_PRESETS:
         snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s", pToeSwitchModeTitles[settings.toeSwitchMode]);
         HMI_RenderLine(1, lineBuffer, 0);
         break;
      default:
#ifdef DEBUG
         DEBUG_MSG("HMI_UpdateDisplay: Invalid toeSwitchMode=%d", settings.toeSwitchMode);
         return;
#endif
      }
      break;
   case PAGE_GEN_MIDI_PRESET_SELECT:
      // Print 3 presets centered around the current selection.

      // Selected entry on line 2
      u8 lastSelectedPreset = settings.lastSelectedGenMIDIProgNumber;
      HMI_RenderLine(2, MIDI_PRESETS_GetGenMIDIPresetName(lastSelectedPreset), 1);
      if (lastSelectedPreset == 0) {
         // At the beginning. Clear line 1
         HMI_ClearLine(1);
      }
      else {
         HMI_RenderLine(1, MIDI_PRESETS_GetGenMIDIPresetName(lastSelectedPreset - 1), 0);
      }
      if (lastSelectedPreset == MIDI_PRESETS_GetNumGenMIDIPresets() - 1) {
         // At the end, clear line 3
         HMI_ClearLine(3);
      }
      else {
         HMI_RenderLine(3, MIDI_PRESETS_GetGenMIDIPresetName(lastSelectedPreset + 1), 0);
      }
      break;
   case PAGE_MAIN:
      // Selected entry on line 2
      HMI_RenderLine(2, mainPageEntries[lastSelectedMainPageEntry], 1);
      if (lastSelectedMainPageEntry == 0) {
         // At the beginning. Clear line 1
         HMI_ClearLine(1);
      }
      else {
         HMI_RenderLine(1, mainPageEntries[lastSelectedMainPageEntry - 1], 0);
      }
      if (lastSelectedMainPageEntry == NUM_MAIN_PAGE_ENTRIES - 1) {
         // At the end, clear line 3
         HMI_ClearLine(3);
      }
      else {
         HMI_RenderLine(3, mainPageEntries[lastSelectedMainPageEntry + 1], 0);
      }
      break;
   case PAGE_ASSIGN_TOE_SWITCH_MIDI_PRESET:
   case PAGE_ASSIGN_TOE_SWITCH_PATTERN_PRESET:
      // Don't have a switch yet
      HMI_RenderLine(2, "Select Toe Switch", 0);

      break;
   }
}


/////////////////////////////////////////////////////////////////////////////
// Helper function renders a line and pads out to full display width or
// truncates to width
/////////////////////////////////////////////////////////////////////////////
void HMI_RenderLine(u8 lineNum, char* pLineText, u8 selected) {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1] = "";
   if (selected) {
      strcpy(lineBuffer,"<");
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH - 2);
      while(strlen(lineBuffer) < DISPLAY_CHAR_WIDTH-1){
         strcat(lineBuffer," ");
      }
      strcat(lineBuffer, ">");
   }
   else {
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH);
      while(strlen(lineBuffer) < DISPLAY_CHAR_WIDTH){
         strcat(lineBuffer," ");
      }
   }
#ifdef DEBUG
   DEBUG_MSG("HMI_RenderLine %d: '%s'", lineNum, lineBuffer);
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

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on home page.
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderChanged(s8 increment) {
   // On any change of encoder go to PAGE_MAIN
   lastPage = currentPage;
   currentPage = &mainPage;
   // And force an update to the display
   HMI_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on home page.
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderSelect() {
   HMI_HomePage_RotaryEncoderChanged(1);
}

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on main page
/////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_RotaryEncoderChanged(s8 increment) {
   // Increment selection and wrap if necessary
   lastSelectedMainPageEntry += increment;
   if (lastSelectedMainPageEntry >= NUM_MAIN_PAGE_ENTRIES) {
      lastSelectedMainPageEntry = NUM_MAIN_PAGE_ENTRIES-1;
   }
   else if (lastSelectedMainPageEntry < 0){
      lastSelectedMainPageEntry = 0;
   }
   // Update the display
   HMI_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on main page
/////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_RotaryEncoderSelected() {
   switch (lastSelectedMainPageEntry) {
   case MAIN_PAGE_ASSIGN_TOE_MIDI_PRESET:
      lastPage = &mainPage;
      currentPage = &assignToeMIDIPresetPage;
      toeSelectionModeActive = 1;
      IND_FlashAll(0);
      break;
   case MAIN_PAGE_ASSIGN_TOE_PATTERN_PRESET:
      // TODO
      break;
   case MAIN_PAGE_ASSIGN_STOMP_SWITCH:
      // TODO
      break;
   default:
      return;
   }

   HMI_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on general MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_GenMIDISelectPage_RotaryEncoderChanged(s8 increment) {
   settings.lastSelectedGenMIDIProgNumber += increment;
   if (settings.lastSelectedGenMIDIProgNumber  < 1) {
      settings.lastSelectedGenMIDIProgNumber  = 1;
   }
   if (settings.lastSelectedGenMIDIProgNumber  > MIDI_PRESETS_GetNumGenMIDIPresets()) {
      settings.lastSelectedGenMIDIProgNumber  = MIDI_PRESETS_GetNumGenMIDIPresets();
   }

   HMI_UpdateDisplay();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_GenMIDISelectPage_RotaryEncoderSelected() {
   // Assign the selected General MIDI preset to the currently selected toe switch
   // for this mode
   if (pendingToeSelection > 0) {
      u8* ptr = &settings.toeSwitchMIDIPresetNumbers[pendingToeSelection - 1];
      u8 presetNumber = 0;
      if (*ptr > 0){
         // There was already a preset on this one so replace it
         presetNumber = MIDI_PRESETS_ReplaceMIDIPreset(*ptr,
            settings.lastSelectedGenMIDIProgNumber,
            0,
            settings.lastSelectedMidiOutput,
            settings.lastSelectedMidiChannel);
      }
      else{
         // No preset so add another one.
         presetNumber = MIDI_PRESETS_AddMIDIPreset(
            settings.lastSelectedGenMIDIProgNumber,            
            0,
            settings.lastSelectedMidiOutput,
            settings.lastSelectedMidiChannel);
      }
      if (presetNumber == 0){
         DEBUG_MSG("HMI_GenMIDISelectPage_RotaryEncoderSelected:  cannot add preset");
      }
   }
   // Clear for next time to go back to the current toe mode 
   pendingToeSelection = 0; 
   // Change the toe switch mode to MIDI preset
   settings.toeSwitchMode = TOE_SWITCH_GEN_MIDI_PRESETS;

   // And restore the currently selected mode toe indicator.
   IND_ClearAll();
   IND_SetIndicatorState(settings.selectedToeSwitches[settings.toeSwitchMode],IND_ON);

   lastPage = currentPage;
   currentPage = &homePage;
   HMI_UpdateDisplay();

   // And send out the preset change
}

/////////////////////////////////////////////////////////////////////////////
// Callback for pedal select on edit toe switch page
////////////////////////////////////////////////////////////////////////////
void HMI_EditToePresetPage_PedalSelected(u8 pedalNum) {

}