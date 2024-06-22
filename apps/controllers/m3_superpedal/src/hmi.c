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
#undef HW_DEBUG
//#undef DEBUG

/////////////////////////////////////////////////////////////////////////////
// HMI defines
/////////////////////////////////////////////////////////////////////////////


#define LONG_PRESS_TIME_MS 3000

#define DISPLAY_CHAR_WIDTH 20

/////////////////////////////////////////////////////////////////////////////
// Display page variables
/////////////////////////////////////////////////////////////////////////////

typedef enum {
   PAGE_HOME = 0,
   PAGE_MAIN_MENU = 1,
   PAGE_EDIT_VOICE_PRESET = 2,
   PAGE_EDIT_PATTERN_PRESET = 3,
   PAGE_MIDI_PROGRAM_SELECT = 4,
} pageID_t;

struct page_s {
   pageID_t pageID;
   char* pPageTitle;
   void (*pUpdateDisplayCallback)();
   void (*pRotaryEncoderChangedCallback)(s8 increment);
   void (*pRotaryEncoderSelectCallback)();
   void (*pPedalSelectedCallback)(u8 pedalNum);
   struct page_s* pBackPage;
};

struct page_s homePage;
struct page_s selectMIDIProgramPage;
struct page_s editToePatternPresetPage;

struct page_s* currentPage;
struct page_s* lastPage;

// Main Page variables
struct page_s mainPage;
typedef enum {
   MAIN_PAGE_ENTRY_EDIT_TOE_MIDI_PRESET = 0,
   MAIN_PAGE_ENTRY_EDIT_TOE_PATTERN_PRESET = 1,
   MAIN_PAGE_ENTRY_EDIT_STOMP_SWITCH = 2
} main_page_entries_t;

#define NUM_MAIN_PAGE_ENTRIES 3
char* mainPageEntries[] = { "Edit Voice Preset","Edit Pattern Preset","Edit Stomp Switch" };
u8 lastSelectedMainPageEntry;

// Edit Toe Voice Preset page variables
struct page_s editVoicePresetPage;
typedef enum {
   EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER = 0,
   EDIT_VOICE_PRESET_PAGE_ENTRY_OCTAVE = 1,
   EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_OUTPUT = 2,
   EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_CHANNEL = 3
} edit_voice_preset_page_entries_t;
#define NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES 4

char* editVoicePageEntries[] = { "MIDI Program Number","Octave","MIDI Output","MIDI Channel" };
u8 lastSelectedEditVoicePresetPageEntry;



/////////////////////////////////////////////////////////////////////////////
// Toe Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////

// Following type is used as an array index so must be 0 based and consecutive


// Text for the toe switch
char* pToeSwitchModeTitles[] = { "Octave","MIDI Presets","Pattern Presets","Arpeggiator" };

typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;
switchState_t toeSwitchState[NUM_TOE_SWITCHES];

/////////////////////////////////////////////////////////////////////////////
// Stomp Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////

switchState_t stompSwitchState[NUM_STOMP_SWITCHES];

switchState_t backSwitchState;
switchState_t encoderSwitchState;

/////////////////////////////////////////////////////////////////////////////
// Persisted data to E^2
/////////////////////////////////////////////////////////////////////////////

persisted_hmi_settings_t settings;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void HMI_RenderLine(u8, const char*, u8);
void HMI_ClearLine(u8);
void HMI_InitPages();

void HMI_HomePage_UpdateDisplay();
void HMI_HomePage_RotaryEncoderChanged(s8);
void HMI_HomePage_RotaryEncoderSelect();

void HMI_MainPage_UpdateDisplay();
void HMI_MainPage_RotaryEncoderChanged(s8);
void HMI_MainPage_RotaryEncoderSelected();

void HMI_EditVoicePresetPage_UpdateDisplay();
void HMI_EditVoicePresetPage_RotaryEncoderChanged(s8 increment);
void HMI_EditVoicePresetPage_RotaryEncoderSelected();

void HMI_MIDIProgramSelectPage_UpdateDisplay();
void HMI_MIDIProgramSelectPage_RotaryEncoderChanged(s8 increment);
void HMI_MIDIProgramSelectPage_RotaryEncoderSelected();


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

   lastSelectedMainPageEntry = MAIN_PAGE_ENTRY_EDIT_TOE_MIDI_PRESET;

   currentPage = &homePage;
   lastPage = currentPage;

   // Restore settings from E^2 if they exist.  If not the initialize to defaults
   // TODO
   u8 valid = 0;
   // valid = PRESETS_GetHMISettings(&settings);
   if (!valid) {
      // Update this ID whenever the persisted structure changes.  
      settings.serializationID = 0x484D4901;   // 'HMI1'

      // stomp switch settings
      settings.stompSwitchSetting[4] = STOMP_SWITCH_OCTAVE;
      settings.stompSwitchSetting[3] = STOMP_SWITCH_VOICE_PRESETS;
      settings.stompSwitchSetting[2] = STOMP_SWITCH_PATTERN_PRESETS;
      settings.stompSwitchSetting[1] = STOMP_SWITCH_UNASSIGNED;
      settings.stompSwitchSetting[0] = STOMP_SWITCH_ARPEGGIATOR;
      settings.toeSwitchMode = TOE_SWITCH_OCTAVE;

      // Toe switch settings
      settings.selectedToeIndicator[TOE_SWITCH_OCTAVE] = DEFAULT_OCTAVE_NUMBER;
      settings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS] = 1;
      settings.selectedToeIndicator[TOE_SWITCH_PATTERN_PRESETS] = 1;
      settings.selectedToeIndicator[TOE_SWITCH_ARPEGGIATOR] = 1;

      for (int i = 0;i < NUM_TOE_SWITCHES;i++) {
         settings.toeSwitchVoicePresetNumbers[i] = i + 1;
      }

      // Misc persisted stats in the HMI
      settings.lastSelectedMIDIProgNumber = 1;
      settings.lastSelectedMidiOutput = DEFAULT_PRESET_MIDI_PORTS;
      settings.lastSelectedMidiChannel = DEFAULT_PRESET_MIDI_CHANNEL;
   }
   // Initialize toe indicators to the current mode
   IND_SetIndicatorState(settings.selectedToeIndicator[settings.toeSwitchMode], IND_ON);

   // Init the display pages now that settings are restored
   HMI_InitPages();

   // Initialize the consumer of the current toe switch mode
   // TODO

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

   char* pHomeTitle = { "---M3 SuperPedal---" };
   homePage.pPageTitle = pHomeTitle;
   homePage.pRotaryEncoderChangedCallback = &HMI_HomePage_RotaryEncoderChanged;
   homePage.pRotaryEncoderSelectCallback = &HMI_HomePage_RotaryEncoderSelect;
   homePage.pPedalSelectedCallback = NULL;
   homePage.pBackPage = NULL;

   mainPage.pageID = PAGE_MAIN_MENU;
   char* pMainTitle = { "  --- Main Menu ---" };
   mainPage.pPageTitle = pMainTitle;
   mainPage.pRotaryEncoderChangedCallback = HMI_MainPage_RotaryEncoderChanged;
   mainPage.pRotaryEncoderSelectCallback = HMI_MainPage_RotaryEncoderSelected;
   mainPage.pPedalSelectedCallback = NULL;
   mainPage.pBackPage = &homePage;

   editVoicePresetPage.pageID = PAGE_EDIT_VOICE_PRESET;
   char* editVoicePresetTitle = { "Edit Toe Voice Pset" };
   editVoicePresetPage.pPageTitle = editVoicePresetTitle;
   editVoicePresetPage.pRotaryEncoderChangedCallback = HMI_EditVoicePresetPage_RotaryEncoderChanged;
   editVoicePresetPage.pRotaryEncoderSelectCallback = HMI_EditVoicePresetPage_RotaryEncoderSelected;
   editVoicePresetPage.pPedalSelectedCallback = NULL;
   editVoicePresetPage.pBackPage = &mainPage;


   editToePatternPresetPage.pageID = PAGE_EDIT_PATTERN_PRESET;
   char* patternPresetTitle = { "Edit Toe Ptrn Pset" };
   editToePatternPresetPage.pPageTitle = patternPresetTitle;
   editToePatternPresetPage.pRotaryEncoderChangedCallback = NULL;
   editToePatternPresetPage.pRotaryEncoderSelectCallback = NULL;
   editToePatternPresetPage.pPedalSelectedCallback = NULL;
   editToePatternPresetPage.pBackPage = &mainPage;


   selectMIDIProgramPage.pageID = PAGE_MIDI_PROGRAM_SELECT;
   char* selectPresetTitle = { "Select MIDI Program" };
   selectMIDIProgramPage.pPageTitle = selectPresetTitle;
   selectMIDIProgramPage.pRotaryEncoderChangedCallback = HMI_MIDIProgramSelectPage_RotaryEncoderChanged;
   selectMIDIProgramPage.pRotaryEncoderSelectCallback = HMI_MIDIProgramSelectPage_RotaryEncoderSelected;
   selectMIDIProgramPage.pPedalSelectedCallback = NULL;
   selectMIDIProgramPage.pBackPage = &editVoicePresetPage;
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

   // Save the state for long press detection although we may not need this
   if (pressed) {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyToeToggle: Toe switch %d pressed", toeNum);
#endif
      if (toeSwitchState[toeNum - 1].switchState == 0) {
         // Save initial press timestamp         
         toeSwitchState[toeNum - 1].switchPressTimeStamp = timestamp;
      }
      toeSwitchState[toeNum - 1].switchState = 1;
   }
   else {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyToeToggle: Toe switch %d released", toeNum);
#endif
      toeSwitchState[toeNum - 1].switchState = 0;
   }

   // Wait for the release so we don't have to deal with bouncing
   if (pressed) {
      return;
   }

   // process the stomp switch
   switch (settings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
      // Set the octave
      PEDALS_SetOctave(toeNum - 1);
      // Save the selected change using GetOctave to close loop
      settings.selectedToeIndicator[TOE_SWITCH_OCTAVE] = PEDALS_GetOctave() + 1;
      // Clear the indicators to turn off the current one (if any)
      IND_ClearAll();
      // Now set the one for the toe after flashing fast for 3 cycles
      IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON);
      // And update the current page display
      currentPage->pUpdateDisplayCallback();
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      u8 presetNum = settings.toeSwitchVoicePresetNumbers[toeNum - 1];
      //DEBUG_MSG("Toe switch: %d presetNum %d",toeNum,presetNum);
            // Activate this preset
      u8 errorNum = MIDI_PRESET_ActivateMIDIPreset(presetNum);
      if (errorNum > 0) {
         // Clear the indicators to turn off the current one (if any)
         IND_ClearAll();
         // Now set the one for the toe after flashing fast for 3 cycles         
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON);
         settings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS] = toeNum;
         // And update the current page display
         currentPage->pUpdateDisplayCallback();
      }
      else {
         // Invalid preset.  Flash and then off
         IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_OFF);
      }

      break;
   case TOE_SWITCH_PATTERN_PRESETS:

      // TODO
      break;
   case TOE_SWITCH_ARPEGGIATOR:

      // TODO
   break;  default:
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
   // Save the state for long press detection although we likely will not need this
   if (pressed) {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyStompToggle:  stomp switch %d pressed", stompNum);
#endif
      if (stompSwitchState[stompNum - 1].switchState == 0) {
         // Save initial press timestamp         
         stompSwitchState[stompNum - 1].switchPressTimeStamp = timestamp;
      }
      stompSwitchState[stompNum - 1].switchState = 1;
   }
   else {
#ifdef HW_DEBUG
      DEBUG_MSG("HMI_NotifyStompToggle: stomp switch %d released", stompNum);
#endif
      stompSwitchState[stompNum - 1].switchState = 0;
   }

   // Wait for the release so we don't have to deal with bouncing
   if (pressed) {
      return;
   }

   switch (settings.stompSwitchSetting[stompNum - 1]) {
   case STOMP_SWITCH_OCTAVE:
      settings.toeSwitchMode = TOE_SWITCH_OCTAVE;
      break;
   case STOMP_SWITCH_VOICE_PRESETS:
      settings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
      break;
   case STOMP_SWITCH_PATTERN_PRESETS:
      settings.toeSwitchMode = TOE_SWITCH_PATTERN_PRESETS;
      break;
   case STOMP_SWITCH_ARPEGGIATOR:
      settings.toeSwitchMode = TOE_SWITCH_ARPEGGIATOR;
      break;
   case STOMP_SWITCH_UNASSIGNED:
      //unassigned on, so just return;
      return;
   }
   // Update the toe switch indicators and the display in case the mode changed.
   IND_ClearAll();
   IND_SetIndicatorState(settings.selectedToeIndicator[settings.toeSwitchMode], IND_ON);
   // And update the current page display
   currentPage->pUpdateDisplayCallback();
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

   switch (currentPage->pageID) {
   case PAGE_HOME:
      // On encoder switch initial press go to main page
      lastPage = currentPage;
      currentPage = &mainPage;
      // And force an update to the current page display
      currentPage->pUpdateDisplayCallback();
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
#ifdef HW_DEBUG
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
#ifdef HW_DEBUG
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
      // And force an update to the current page display
      currentPage->pUpdateDisplayCallback();
      // Set handled to ignore the release
      backSwitchState.handled = 1;
      return;
   }

   // Otherwise, on a release, go back one page
   if (currentPage->pBackPage != NULL) {
      lastPage = currentPage;
      currentPage = currentPage->pBackPage;
      // And force an update to the current page display
      currentPage->pUpdateDisplayCallback();
   }
}

/////////////////////////////////////////////////////////////////////////////
// Helper function renders a line and pads out to full display width or
// truncates to width
/////////////////////////////////////////////////////////////////////////////
void HMI_RenderLine(u8 lineNum, const char* pLineText, u8 selected) {
   char lineBuffer[DISPLAY_CHAR_WIDTH + 1] = "";
   if (selected) {
      strcpy(lineBuffer, "<");
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH - 2);
      while (strlen(lineBuffer) < DISPLAY_CHAR_WIDTH - 1) {
         strcat(lineBuffer, " ");
      }
      strcat(lineBuffer, ">");
   }
   else {
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH);
      while (strlen(lineBuffer) < DISPLAY_CHAR_WIDTH) {
         strcat(lineBuffer, " ");
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

////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Home page.
////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_UpdateDisplay() {
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
   u8 selectedToeSwitch = settings.selectedToeIndicator[settings.toeSwitchMode];
   char* pModeName = pToeSwitchModeTitles[settings.toeSwitchMode];

   switch (settings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s %d", pModeName, PEDALS_GetOctave());
      HMI_RenderLine(1, lineBuffer, 0);
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      HMI_RenderLine(1, pModeName, 0);
      u8 presetNum = settings.toeSwitchVoicePresetNumbers[selectedToeSwitch - 1];
      const midi_preset_t* preset = MIDI_PRESETS_GetMidiPreset(presetNum);
      if (preset == NULL) {
         HMI_RenderLine(1, "Invalid Preset Number", 0);
         DEBUG_MSG("Invalid pset#:  %d", presetNum);
      }
      else {
         const char* pName = MIDI_PRESETS_GetGenMIDIVoiceName(preset->programNumber);
         HMI_RenderLine(2, pName, 0);
      }
      break;
   case TOE_SWITCH_PATTERN_PRESETS:
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s", pModeName);
      HMI_RenderLine(1, lineBuffer, 0);
      break;
   default:
#ifdef DEBUG
      DEBUG_MSG("HMI_UpdateDisplay: Invalid toeSwitchMode=%d", settings.toeSwitchMode);
      return;
#endif
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on home page.
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderChanged(s8 increment) {
   // On any change of encoder go to PAGE_MAIN_MENU
   lastPage = currentPage;
   currentPage = &mainPage;
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on home page.
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderSelect() {
   HMI_HomePage_RotaryEncoderChanged(1);
}

////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Main page.
////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_UpdateDisplay() {
   if (lastPage != currentPage) {
      // Clear display
      MIOS32_LCD_Clear();
      // Set to keep from redrawing the entire display next time.
      lastPage = currentPage;
   }
   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(currentPage->pPageTitle);

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
}

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on main page
/////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_RotaryEncoderChanged(s8 increment) {
   // Increment selection and wrap if necessary
   lastSelectedMainPageEntry += increment;
   if (lastSelectedMainPageEntry >= NUM_MAIN_PAGE_ENTRIES) {
      lastSelectedMainPageEntry = NUM_MAIN_PAGE_ENTRIES - 1;
   }
   else if (lastSelectedMainPageEntry < 0) {
      lastSelectedMainPageEntry = 0;
   }
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on main page
/////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_RotaryEncoderSelected() {
   switch (lastSelectedMainPageEntry) {
   case MAIN_PAGE_ENTRY_EDIT_TOE_MIDI_PRESET:
      lastPage = &mainPage;
      currentPage = &editVoicePresetPage;
      break;
   case MAIN_PAGE_ENTRY_EDIT_TOE_PATTERN_PRESET:
      // TODO
      break;
   case MAIN_PAGE_ENTRY_EDIT_STOMP_SWITCH:
      // TODO
      break;
   default:
      return;
   }
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Voice Preset Edit Page
////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_UpdateDisplay() {
   if (lastPage != currentPage) {
      // Clear display
      MIOS32_LCD_Clear();
      // Set to keep from redrawing the entire display next time.
      lastPage = currentPage;
   }
   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(currentPage->pPageTitle);

   // Selected entry on line 2
   HMI_RenderLine(2, editVoicePageEntries[lastSelectedEditVoicePresetPageEntry], 1);
   if (lastSelectedEditVoicePresetPageEntry == 0) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, mainPageEntries[lastSelectedEditVoicePresetPageEntry - 1], 0);
   }
   if (lastSelectedEditVoicePresetPageEntry == NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, mainPageEntries[lastSelectedEditVoicePresetPageEntry + 1], 0);
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on Edit Voice Preset page
/////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_RotaryEncoderChanged(s8 increment) {
   // Increment selection and wrap if necessary
   lastSelectedEditVoicePresetPageEntry += increment;
   if (lastSelectedEditVoicePresetPageEntry >= NUM_MAIN_PAGE_ENTRIES) {
      lastSelectedEditVoicePresetPageEntry = NUM_MAIN_PAGE_ENTRIES - 1;
   }
   else if (lastSelectedEditVoicePresetPageEntry < 0) {
      lastSelectedEditVoicePresetPageEntry = 0;
   }
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on Edit Voice Preset Page
/////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_RotaryEncoderSelected() {
   switch (lastSelectedMainPageEntry) {
   case EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER:
      lastPage = &mainPage;
      currentPage = &editVoicePresetPage;
      break;
   case EDIT_VOICE_PRESET_PAGE_ENTRY_OCTAVE:
      // TODO
      break;
   case EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_OUTPUT:
      // TODO
      break;
   case EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_CHANNEL:
      // TODO
      break;
   default:
      return;
   }
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the MIDIProgramSelectPage
////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_UpdateDisplay() {
   if (lastPage != currentPage) {
      // Clear display
      MIOS32_LCD_Clear();
      // Set to keep from redrawing the entire display next time.
      lastPage = currentPage;
   }
   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(currentPage->pPageTitle);

   // Print 3 presets centered around the current selection.

   // Selected entry on line 2
   u8 lastProgNumber = settings.lastSelectedMIDIProgNumber;
   HMI_RenderLine(2, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber), 1);
   if (lastProgNumber == 1) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber - 1), 0);
   }
   if (lastProgNumber == MIDI_PRESETS_GetNumGenMIDIVoices() - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber + 1), 0);
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on general MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderChanged(s8 increment) {
   settings.lastSelectedMIDIProgNumber += increment;
   if (settings.lastSelectedMIDIProgNumber < 1) {
      settings.lastSelectedMIDIProgNumber = 1;
   }
   if (settings.lastSelectedMIDIProgNumber > MIDI_PRESETS_GetNumGenMIDIVoices()) {
      settings.lastSelectedMIDIProgNumber = MIDI_PRESETS_GetNumGenMIDIVoices();
   }
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderSelected() {
   // Assign the selected General MIDI preset to the currently selected voice preset
   u8 selectedToeSwitch = settings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS];
   const midi_preset_t* presetPtr = MIDI_PRESETS_SetMIDIPreset(
      selectedToeSwitch,    // toe number is preset number
      settings.lastSelectedMIDIProgNumber,
      0,
      settings.lastSelectedMidiOutput,
      settings.lastSelectedMidiChannel);
   if (presetPtr == NULL) {
      DEBUG_MSG("HMI_MIDIProgramSelectPage_RotaryEncoderSelected:  Error editing preset");
   }
   else {
      // Success, set the toe switch mode to VOICE_PRESET and flash the indicator of the change preset
      settings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
      IND_ClearAll();
      IND_SetTempIndicatorState(selectedToeSwitch, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON);
   }

   // Go back to the home page
   lastPage = currentPage;
   currentPage = &homePage;
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
   // And send out the preset change
}
