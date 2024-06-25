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
#include "pedals.h"
#include "midi_presets.h"
#include "indicators.h"
#include "persist.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG
#undef HW_DEBUG

/////////////////////////////////////////////////////////////////////////////
// HMI defines and macros
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
   PAGE_EDIT_ARP_PRESET = 5
} pageID_t;

struct page_s {
   pageID_t pageID;
   char* pPageTitle;
   void (*pUpdateDisplayCallback)();
   void (*pRotaryEncoderChangedCallback)(s8 increment);
   void (*pRotaryEncoderSelectCallback)();
   void (*pPedalSelectedCallback)(u8 pedalNum);
   void (*pBackButtonCallback)();
   struct page_s* pBackPage;
};

struct page_s homePage;
struct page_s midiProgramSelectPage;

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

// Edit Voice Preset page variables
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

// Edit Pattern Preset page variables
struct page_s editPatternPresetPage;

typedef enum {
   RENDER_LINE_LEFT = 0,
   RENDER_LINE_CENTER = 1,
   RENDER_LINE_SELECT = 2,
   RENDER_LINE_RIGHT = 3
} render_line_mode_t;

/////////////////////////////////////////////////////////////////////////////
// Toe Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////

// Total list of functions availabe in ARP Live mode.  Can be mapped to toe switches
// in preesets
typedef enum {
   ARP_LIVE_TOE_MAJOR_KEY = 0,
   ARP_LIVE_TOE_MINOR_KEY = 1,
   ARP_LIVE_TOE_ASCENDING = 2,
   ARP_LIVE_TOE_DESCENDING = 3,
   ARP_LIVE_TOE_RANDOM = 4,
   ARP_LIVE_TOE_TEMPO_DECREMENT = 6,
   ARP_LIVE_TOE_TEMPO_INCREMENT = 7,
} arp_live_toe_functions_t;


// Text for the toe switch
char* pToeSwitchModeTitles[] = { "Octave","Volume","MIDI Presets","Pattern Presets","Arpeggiator" };

typedef struct switchState_s {
   u8 switchState;
   s32 switchPressTimeStamp;
   u8 handled;
} switchState_t;
switchState_t toeSwitchState[NUM_TOE_SWITCHES];

// Toe volume levels even split from 5 to 127
u8 toeVolumeLevels[NUM_TOE_SWITCHES] = { 5,23,41,58,77,93,110,127 };

/////////////////////////////////////////////////////////////////////////////
// Stomp Switch types and non-persisted variables
/////////////////////////////////////////////////////////////////////////////

switchState_t stompSwitchState[NUM_STOMP_SWITCHES];

switchState_t backSwitchState;
switchState_t encoderSwitchState;

/////////////////////////////////////////////////////////////////////////////
// Persisted data to E^2
/////////////////////////////////////////////////////////////////////////////

persisted_hmi_settings_t hmiSettings;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
void HMI_RenderLine(u8, const char*, render_line_mode_t);
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
void HMI_MIDIProgramSelectPage_BackButtonCallback();

void HMI_PersistData();

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

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = 0;

   // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
   hmiSettings.serializationID = 0x484D4901;   // 'HMI1'

   valid = PERSIST_ReadBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   if (valid < 0) {
      DEBUG_MSG("HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

      // stomp switch settings
      hmiSettings.stompSwitchSetting[4] = STOMP_SWITCH_OCTAVE;
      hmiSettings.stompSwitchSetting[3] = STOMP_SWITCH_VOLUME;
      hmiSettings.stompSwitchSetting[2] = STOMP_SWITCH_VOICE_PRESETS;
      hmiSettings.stompSwitchSetting[1] = STOMP_SWITCH_PATTERN_PRESETS;
      hmiSettings.stompSwitchSetting[0] = STOMP_SWITCH_ARPEGGIATOR;
      hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;

      // Toe switch settings for all but arpeggiator mode which uses bit fieldsinstead
      hmiSettings.selectedToeIndicator[TOE_SWITCH_OCTAVE] = PEDALS_DEFAULT_OCTAVE_NUMBER;
      hmiSettings.selectedToeIndicator[TOE_SWITCH_VOLUME] = 8;     // Corresponds to maximum volume
      hmiSettings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS] = 1;
      hmiSettings.selectedToeIndicator[TOE_SWITCH_PATTERN_PRESETS] = 1;
      hmiSettings.selectedToeIndicator[TOE_SWITCH_ARP_PRESETS] = 1;


      
      for (int i = 0;i < NUM_TOE_SWITCHES;i++) {
         hmiSettings.toeSwitchVoicePresetNumbers[i] = i + 1;
      }

      // Misc persisted stats in the HMI
      hmiSettings.lastSelectedMIDIProgNumber = 1;
      hmiSettings.lastSelectedMidiOutput = DEFAULT_PRESET_MIDI_PORTS;
      hmiSettings.lastSelectedMidiChannel = DEFAULT_PRESET_MIDI_CHANNEL;

      valid = PERSIST_StoreBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
      if (valid < 0) {
         DEBUG_MSG("HMI_Init:  Error persisting setting to EEPROM");
      }
   }
   // Now that settings are init'ed/restored, then synch the HMI

   // Init the display pages now that settings are restored
   HMI_InitPages();

   // Set toe indicators to the current mode
   IND_SetIndicatorState(hmiSettings.selectedToeIndicator[hmiSettings.toeSwitchMode], IND_ON);

   // Activate the last selected preset
   u8 presetNumber = hmiSettings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS];
   MIDI_PRESETS_ActivateMIDIPreset(presetNumber);

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
   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pRotaryEncoderChangedCallback = HMI_HomePage_RotaryEncoderChanged;
   homePage.pRotaryEncoderSelectCallback = HMI_HomePage_RotaryEncoderSelect;
   homePage.pPedalSelectedCallback = NULL;
   homePage.pBackButtonCallback = NULL;
   homePage.pBackPage = NULL;

   mainPage.pageID = PAGE_MAIN_MENU;
   char* pMainTitle = { "  --- Main Menu ---" };
   mainPage.pPageTitle = pMainTitle;
   mainPage.pUpdateDisplayCallback = HMI_MainPage_UpdateDisplay;
   mainPage.pRotaryEncoderChangedCallback = HMI_MainPage_RotaryEncoderChanged;
   mainPage.pRotaryEncoderSelectCallback = HMI_MainPage_RotaryEncoderSelected;
   mainPage.pPedalSelectedCallback = NULL;
   mainPage.pBackButtonCallback = NULL;
   mainPage.pBackPage = &homePage;

   editVoicePresetPage.pageID = PAGE_EDIT_VOICE_PRESET;
   char* editVoicePresetTitle = { "Edit Toe Voice Pset" };
   editVoicePresetPage.pPageTitle = editVoicePresetTitle;
   editVoicePresetPage.pUpdateDisplayCallback = HMI_EditVoicePresetPage_UpdateDisplay;
   editVoicePresetPage.pRotaryEncoderChangedCallback = HMI_EditVoicePresetPage_RotaryEncoderChanged;
   editVoicePresetPage.pRotaryEncoderSelectCallback = HMI_EditVoicePresetPage_RotaryEncoderSelected;
   editVoicePresetPage.pPedalSelectedCallback = NULL;
   editVoicePresetPage.pBackButtonCallback = NULL;
   editVoicePresetPage.pBackPage = &mainPage;


   editPatternPresetPage.pageID = PAGE_EDIT_PATTERN_PRESET;
   char* patternPresetTitle = { "Edit Toe Ptrn Pset" };
   editPatternPresetPage.pPageTitle = patternPresetTitle;
   // TODO - add callback functions below and replace dummy home page one
   editPatternPresetPage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   editPatternPresetPage.pRotaryEncoderChangedCallback = NULL;
   editPatternPresetPage.pRotaryEncoderSelectCallback = NULL;
   editPatternPresetPage.pPedalSelectedCallback = NULL;
   editPatternPresetPage.pBackButtonCallback = NULL;
   editPatternPresetPage.pBackPage = &mainPage;


   midiProgramSelectPage.pageID = PAGE_MIDI_PROGRAM_SELECT;
   char* selectPresetTitle = { "Select MIDI Program" };
   midiProgramSelectPage.pPageTitle = selectPresetTitle;
   midiProgramSelectPage.pUpdateDisplayCallback = HMI_MIDIProgramSelectPage_UpdateDisplay;
   midiProgramSelectPage.pRotaryEncoderChangedCallback = HMI_MIDIProgramSelectPage_RotaryEncoderChanged;
   midiProgramSelectPage.pRotaryEncoderSelectCallback = HMI_MIDIProgramSelectPage_RotaryEncoderSelected;
   midiProgramSelectPage.pPedalSelectedCallback = NULL;
   midiProgramSelectPage.pBackButtonCallback = HMI_MIDIProgramSelectPage_BackButtonCallback;
   midiProgramSelectPage.pBackPage = &editVoicePresetPage;
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
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
      // Set the octave in PEDALS.  Note that PEDALS will directly call HMI_NotifyOctaveChange
      // which will update everything else.
      PEDALS_SetOctave(toeNum - 1);
      break;
   case TOE_SWITCH_VOLUME:
      // Get the volumeLevel corresponding to the pressed switch
      u8 volumeLevel = toeVolumeLevels[toeNum - 1];
      // Set the volume in PEDALS
      PEDALS_SetVolume(volumeLevel);
      // Save the selected change using GetVolume to close loop
      hmiSettings.selectedToeIndicator[TOE_SWITCH_VOLUME] = PEDALS_GetVolume() + 1;
      // Persist the updated volume indicator setting
      HMI_PersistData();
      // Clear the indicators to turn off the current one (if any)
      IND_ClearAll();
      // Now set the one for the selected toe 
      IND_SetIndicatorState(toeNum, IND_ON);
      // And update the current page display
      currentPage->pUpdateDisplayCallback();
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      u8 presetNum = hmiSettings.toeSwitchVoicePresetNumbers[toeNum - 1];
      //DEBUG_MSG("Toe switch: %d presetNum %d",toeNum,presetNum);
            // Activate this preset
      u8 errorNum = MIDI_PRESETS_ActivateMIDIPreset(presetNum);
      if (errorNum > 0) {
         // Clear the indicators to turn off the current one (if any)
         IND_ClearAll();
         // Now set the one for the toe     
         IND_SetIndicatorState(toeNum, IND_ON);
         //  IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON);
         hmiSettings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS] = toeNum;
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
   case TOE_SWITCH_ARP_LIVE:

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

   switch (hmiSettings.stompSwitchSetting[stompNum - 1]) {
   case STOMP_SWITCH_OCTAVE:
      hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;
      break;
   case STOMP_SWITCH_VOICE_PRESETS:
      hmiSettings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
      break;
   case STOMP_SWITCH_PATTERN_PRESETS:
      hmiSettings.toeSwitchMode = TOE_SWITCH_PATTERN_PRESETS;
      break;
   case STOMP_SWITCH_ARPEGGIATOR:
      hmiSettings.toeSwitchMode = TOE_SWITCH_ARP_LIVE;
      // Call dedicated function to set the indicators to the ARP settings since multiple
      // indicators can be set in this mode.
      HMI_SetArpSettingsIndicators();
      break;
   case STOMP_SWITCH_VOLUME:
      hmiSettings.toeSwitchMode = TOE_SWITCH_VOLUME;
      break;
   default:
      // Invalid.  Just return;
      DEBUG_MSG("HMI_NotifyStompToggle:  Invalid setting %d", hmiSettings.stompSwitchSetting[stompNum - 1]);
      return;
   }
   // Update the toe switch indicators and the display in case the mode changed.
   IND_ClearAll();
   IND_SetIndicatorState(hmiSettings.selectedToeIndicator[hmiSettings.toeSwitchMode], IND_ON);
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

   // Otherwise, on a release, check if there is a registered handler.  If so call it 
   if (currentPage->pBackButtonCallback != NULL){
      currentPage->pBackButtonCallback();
   }
   // If the back page is non-null then transition to that page.
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
void HMI_RenderLine(u8 lineNum, const char* pLineText, render_line_mode_t renderMode) {
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
      if (strlen(pLineText) < (DISPLAY_CHAR_WIDTH-2)){
         // At least one space to add
         spaceCount = DISPLAY_CHAR_WIDTH-strlen(pLineText)-2;
         leftIndent = spaceCount/2;
      }
      strcpy(lineBuffer, "<");
      for(int i=0;i < leftIndent;i++){
         strcat(lineBuffer," ");       
      }
      strncat(lineBuffer, pLineText, DISPLAY_CHAR_WIDTH - 2);
      // Compute right side spaces
      spaceCount -= leftIndent;
      for(int i=0;i < spaceCount;i++){
         strcat(lineBuffer," ");       
      }      
      strcat(lineBuffer, ">");
      break;
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

   char lineBuffer[DISPLAY_CHAR_WIDTH];

   // Current octave and volume always go on line 3
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s #%d %s %d",
      pToeSwitchModeTitles[TOE_SWITCH_OCTAVE], PEDALS_GetOctave(),
      pToeSwitchModeTitles[TOE_SWITCH_VOLUME], PEDALS_GetVolume());
   HMI_RenderLine(3, lineBuffer, RENDER_LINE_LEFT);

   // Now render reset of lines according to according to the mode
   u8 selectedToeSwitch = hmiSettings.selectedToeIndicator[hmiSettings.toeSwitchMode];
   // Show the Current Mode on top line.
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH, "%s Mode",
      pToeSwitchModeTitles[hmiSettings.toeSwitchMode]);
   HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);
   HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);

   // Render lines 2 and 3 based on the current mode
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
      HMI_ClearLine(2);
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      u8 presetNum = hmiSettings.toeSwitchVoicePresetNumbers[selectedToeSwitch - 1];
      const midi_preset_t* preset = MIDI_PRESETS_GetMidiPreset(presetNum);
      if (preset == NULL) {
         HMI_RenderLine(2, "Invalid Preset Number", RENDER_LINE_CENTER);
         DEBUG_MSG("Invalid pset#:  %d", presetNum);
      }
      else {
         const char* pName = MIDI_PRESETS_GetGenMIDIVoiceName(preset->programNumber);
         HMI_RenderLine(2, pName, RENDER_LINE_CENTER);
      }
      break;
   case TOE_SWITCH_PATTERN_PRESETS:
      // TODO
      HMI_ClearLine(2);
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
   HMI_RenderLine(2, mainPageEntries[lastSelectedMainPageEntry], RENDER_LINE_SELECT);
   if (lastSelectedMainPageEntry == 0) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, mainPageEntries[lastSelectedMainPageEntry - 1], RENDER_LINE_RIGHT);
   }
   if (lastSelectedMainPageEntry >= NUM_MAIN_PAGE_ENTRIES - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, mainPageEntries[lastSelectedMainPageEntry + 1], RENDER_LINE_RIGHT);
   }
}

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on main page
/////////////////////////////////////////////////////////////////////////////
void HMI_MainPage_RotaryEncoderChanged(s8 increment) {

   // Check if we are already at the top or bottom
   if (increment < 0) {
      if (lastSelectedMainPageEntry == 0) {
         return;  // already at the top
      }
   }
   else if (lastSelectedMainPageEntry == NUM_MAIN_PAGE_ENTRIES - 1) {
      return;  // already at the bottom
   }
   // Otherwise, at least one more detent either way so update.
   lastSelectedMainPageEntry += increment;
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
   HMI_RenderLine(2, editVoicePageEntries[lastSelectedEditVoicePresetPageEntry], RENDER_LINE_SELECT);
   if (lastSelectedEditVoicePresetPageEntry == 0) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, mainPageEntries[lastSelectedEditVoicePresetPageEntry - 1], RENDER_LINE_CENTER);
   }
   if (lastSelectedEditVoicePresetPageEntry == NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, mainPageEntries[lastSelectedEditVoicePresetPageEntry + 1], RENDER_LINE_CENTER);
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on Edit Voice Preset page
/////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_RotaryEncoderChanged(s8 increment) {
   // Check if we are already at the top or bottom
   if (increment < 0) {
      if (lastSelectedEditVoicePresetPageEntry == 0) {
         return;  // at the top
      }
   }
   else if (lastSelectedEditVoicePresetPageEntry == NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES - 1) {
      return;  // at the bottom
   }
   // Otherwise, at least one more detent either way so update.
   lastSelectedEditVoicePresetPageEntry += increment;
   // And force an update to the current page display
   currentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on Edit Voice Preset Page
/////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_RotaryEncoderSelected() {
   switch (lastSelectedMainPageEntry) {
   case EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER:
      lastPage = &editVoicePresetPage;
      currentPage = &midiProgramSelectPage;
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
   u8 lastProgNumber = hmiSettings.lastSelectedMIDIProgNumber;
   HMI_RenderLine(2, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber), RENDER_LINE_SELECT);
   if (lastProgNumber == 1) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber - 1), RENDER_LINE_CENTER);
   }
   if (lastProgNumber == MIDI_PRESETS_GetNumGenMIDIVoices() - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, MIDI_PRESETS_GetGenMIDIVoiceName(lastProgNumber + 1), RENDER_LINE_CENTER);
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on general MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderChanged(s8 increment) {
   u8 progNumber = hmiSettings.lastSelectedMIDIProgNumber;
   progNumber += increment;
   if (progNumber < 1) {
      progNumber = 1;
   }
   if (progNumber > MIDI_PRESETS_GetNumGenMIDIVoices()) {
      progNumber = MIDI_PRESETS_GetNumGenMIDIVoices();
   }
   if (hmiSettings.lastSelectedMIDIProgNumber != progNumber){
      hmiSettings.lastSelectedMIDIProgNumber = progNumber;
      // Activate the MIDI voice temporarily using midi config from current preset
      const midi_preset_t* preset = MIDI_PRESETS_GetLastActivatedPreset();
      // TODO handle bankNumber too
      MIDI_PRESETS_ActivateMIDIVoice(progNumber,0,preset->midiPorts,preset->midiChannel);
      // And force an update to the current page display
      currentPage->pUpdateDisplayCallback();
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderSelected() {
   // Assign the selected General MIDI preset to the currently selected voice preset
   u8 selectedToeSwitch = hmiSettings.selectedToeIndicator[TOE_SWITCH_VOICE_PRESETS];
   const midi_preset_t* presetPtr = MIDI_PRESETS_SetMIDIPreset(
      selectedToeSwitch,    // toe number is preset number
      hmiSettings.lastSelectedMIDIProgNumber,
      0,
      PEDALS_GetOctave(),
      hmiSettings.lastSelectedMidiOutput,
      hmiSettings.lastSelectedMidiChannel);
   if (presetPtr == NULL) {
      DEBUG_MSG("HMI_MIDIProgramSelectPage_RotaryEncoderSelected:  Error editing preset");
   }
   else {
      // Success, set the toe switch mode to VOICE_PRESET and flash the indicator of the change preset
      hmiSettings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
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

/////////////////////////////////////////////////////////////////////////////
// Callback for back button pressed on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_BackButtonCallback(){
   // On a back button press, user did not select a new preset so re-activate the
   // last one.
   const midi_preset_t * preset = MIDI_PRESETS_GetLastActivatedPreset();
   MIDI_PRESETS_ActivateMIDIPreset(preset->presetNumber);
}


/////////////////////////////////////////////////////////////////////////////
// Helper to store persisted data 
/////////////////////////////////////////////////////////////////////////////
void HMI_PersistData() {
#ifdef DEBUG_ENABLED
   DEBUG_MSG("MIDI_PRESETS_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(presets));
#endif
   s32 valid = PERSIST_StoreBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   if (valid < 0) {
      DEBUG_MSG("HMI_PersistData:  Error persisting setting to EEPROM");
   }
   return;
}

/////////////////////////////////////////////////////////////////////////////
// Notifies HMI on an octave change.
// Called by PEDALS_SetOctave whenever the octave is changed.
// Used to automatically synch the Indicators
/////////////////////////////////////////////////////////////////////////////
void HMI_NotifyOctaveChange(u8 octave){
   u8 indicator = octave+1;
   if (hmiSettings.selectedToeIndicator[TOE_SWITCH_OCTAVE] != indicator){
      // Indicator has change. 
      hmiSettings.selectedToeIndicator[TOE_SWITCH_OCTAVE] = indicator;
      // Save the new indicator position
      HMI_PersistData();

      // If TOE_SWITCH_OCTAVE also currently active, then update the 
      // indicator
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_OCTAVE){
         IND_ClearAll();
         IND_SetIndicatorState(indicator,IND_ON);
      }
      // And update the current display in case it is showing Octave
      currentPage->pUpdateDisplayCallback();
   }

}

/////////////////////////////////////////////////////////////////////////////
// Sets/updates the indicators for the current ARP_LIVE mode
/////////////////////////////////////////////////////////////////////////////
void HMI_SetArpSettingsIndicators(){
   IND_ClearAll();

}
