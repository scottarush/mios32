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
#include "midi_presets.h"
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

#define MIN_DIRECT_OCTAVE_NUMBER 0


//----------------------------------------------------------------------------
// Global display Page variable declarations
//----------------------------------------------------------------------------
struct page_s homePage;
struct page_s midiProgramSelectPage;
struct page_s arpSettingsPage;
struct page_s dialogPage;
struct page_s* pCurrentPage;


// Buffer for dialog Page Title
char dialogPageTitle[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 1
char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];

// Edit Voice Preset page variables
struct page_s editVoicePresetPage;
typedef enum edit_voice_preset_page_entries_e {
   EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER = 0,
   EDIT_VOICE_PRESET_PAGE_ENTRY_OCTAVE = 1,
   EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_OUTPUT = 2,
   EDIT_VOICE_PRESET_PAGE_ENTRY_MIDI_CHANNEL = 3
} edit_voice_preset_page_entries_t;
#define NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES 4

static const char* editVoicePageEntries[] = { "MIDI Program Number","Octave","MIDI Output","MIDI Channel" };
static u8 lastSelectedEditVoicePresetPageEntry;

// Edit Pattern Preset page variables
struct page_s editPatternPresetPage;

//----------------------------------------------------------------------------
// Toe Switch types and non-persisted variables
//----------------------------------------------------------------------------

// Text for the toe switch index by toeSwitchMode_e
static const char* pToeSwitchModeTitles[] = { "OCTAVE","VOLUME","VOICE PST","PATT PST","ARP","CHORD" };

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

void HMI_EditVoicePresetPage_UpdateDisplay();
void HMI_EditVoicePresetPage_RotaryEncoderChanged(s8 increment);
void HMI_EditVoicePresetPage_RotaryEncoderSelected();

void HMI_MIDIProgramSelectPage_UpdateDisplay();
void HMI_MIDIProgramSelectPage_RotaryEncoderChanged(s8 increment);
void HMI_MIDIProgramSelectPage_RotaryEncoderSelected();
void HMI_MIDIProgramSelectPage_BackButtonCallback();

s32 HMI_PersistData();
void HMI_UpdateOctaveIncDecIndicators();
u8 HMI_GetToeVolumeIndex();

void HMI_HandleVoicePresetsToeToggle(u8 toeNum);
u8 HMI_DebounceSwitchChange(switchState_t* pState, u8 pressed, s32 timestamp);

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

   lastSelectedEditVoicePresetPageEntry = EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER;

   pCurrentPage = &homePage;

   // Restore settings from E^2 if they exist.  If not then initialize to defaults
   s32 valid = 0;

   // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
   hmiSettings.serializationID = 0x484D4901;   // 'HMI1'

   valid = PERSIST_ReadBlock(PERSIST_HMI_BLOCK, (unsigned char*)&hmiSettings, sizeof(persisted_hmi_settings_t));
   if (valid < 0) {
      DEBUG_MSG("HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

      // stomp switch settings
      hmiSettings.stompSwitchSetting[4] = STOMP_SWITCH_OCTAVE_VOLUME;
      hmiSettings.stompSwitchSetting[3] = STOMP_SWITCH_ARPEGGIATOR;
      hmiSettings.stompSwitchSetting[2] = STOMP_SWITCH_CHORD_PAD;
      hmiSettings.stompSwitchSetting[1] = STOMP_SWITCH_VOICE_PRESETS;
      hmiSettings.stompSwitchSetting[0] = STOMP_SWITCH_PATTERN_PRESETS;

      // Set deefault TOE mode
      hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;

      // Initialize the voice preset banks
      hmiSettings.numToeSwitchGenMIDIPresetBanks = NUM_TOE_GEN_MIDI_PRESET_BANKS;
      for (s32 bank = 0;bank < hmiSettings.numToeSwitchGenMIDIPresetBanks;bank++) {
         for (s32 i = 0;i < NUM_TOE_PRESETS_PER_BANK;i++) {
            hmiSettings.toeSwitchVoicePresetNumbers[bank][i] = bank * i + 1;
         }
      }
      hmiSettings.currentToeSwitchGenMIDIPresetBank = 1;

      // Misc persisted stats in the HMI
      hmiSettings.lastSelectedMIDIProgNumber = 1;
      hmiSettings.lastSelectedMidiOutput = DEFAULT_PRESET_MIDI_PORTS;
      hmiSettings.lastSelectedMidiChannel = DEFAULT_PRESET_MIDI_CHANNEL;

      HMI_PersistData();
   }
   // Now that settings are init'ed/restored, then synch the HMI

   // Init the display pages now that settings are restored
   HMI_InitPages();

   // Set the indicators to the curren toe switch mode
   HMI_UpdateIndicators();

   // Activate the last selected preset
   const midi_preset_num_t* presetNumber = MIDI_PRESETS_GetLastActivatedMIDIPreset();
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

   char* pHomeTitle = { "---M3 SUPERPEDAL---" };
   homePage.pPageTitle = pHomeTitle;
   homePage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   homePage.pRotaryEncoderChangedCallback = HMI_HomePage_RotaryEncoderChanged;
   homePage.pRotaryEncoderSelectCallback = HMI_HomePage_RotaryEncoderSelect;
   homePage.pPedalSelectedCallback = NULL;
   homePage.pBackButtonCallback = NULL;
   homePage.pBackPage = NULL;

   editVoicePresetPage.pageID = PAGE_EDIT_VOICE_PRESET;
   char* editVoicePresetTitle = { "EDIT VOICE PRESET" };
   editVoicePresetPage.pPageTitle = editVoicePresetTitle;
   editVoicePresetPage.pUpdateDisplayCallback = HMI_EditVoicePresetPage_UpdateDisplay;
   editVoicePresetPage.pRotaryEncoderChangedCallback = HMI_EditVoicePresetPage_RotaryEncoderChanged;
   editVoicePresetPage.pRotaryEncoderSelectCallback = HMI_EditVoicePresetPage_RotaryEncoderSelected;
   editVoicePresetPage.pPedalSelectedCallback = NULL;
   editVoicePresetPage.pBackButtonCallback = NULL;
   editVoicePresetPage.pBackPage = &homePage;

   editPatternPresetPage.pageID = PAGE_EDIT_PATTERN_PRESET;
   char* patternPresetTitle = { "EDIT PATTERN PRESET" };
   editPatternPresetPage.pPageTitle = patternPresetTitle;
   // TODO - add callback functions below and replace dummy home page one
   editPatternPresetPage.pUpdateDisplayCallback = HMI_HomePage_UpdateDisplay;
   editPatternPresetPage.pRotaryEncoderChangedCallback = NULL;
   editPatternPresetPage.pRotaryEncoderSelectCallback = NULL;
   editPatternPresetPage.pPedalSelectedCallback = NULL;
   editPatternPresetPage.pBackButtonCallback = NULL;
   editPatternPresetPage.pBackPage = &homePage;


   midiProgramSelectPage.pageID = PAGE_MIDI_PROGRAM_SELECT;
   char* selectPresetTitle = { "SELECT MIDI PROGRAM" };
   midiProgramSelectPage.pPageTitle = selectPresetTitle;
   midiProgramSelectPage.pUpdateDisplayCallback = HMI_MIDIProgramSelectPage_UpdateDisplay;
   midiProgramSelectPage.pRotaryEncoderChangedCallback = HMI_MIDIProgramSelectPage_RotaryEncoderChanged;
   midiProgramSelectPage.pRotaryEncoderSelectCallback = HMI_MIDIProgramSelectPage_RotaryEncoderSelected;
   midiProgramSelectPage.pPedalSelectedCallback = NULL;
   midiProgramSelectPage.pBackButtonCallback = HMI_MIDIProgramSelectPage_BackButtonCallback;
   midiProgramSelectPage.pBackPage = &editVoicePresetPage;

   arpSettingsPage.pageID = PAGE_ARP_SETTINGS;
   char* arpSettingsTitle = { "ARP SETTINGS" };
   arpSettingsPage.pPageTitle = arpSettingsTitle;
   arpSettingsPage.pUpdateDisplayCallback = ARP_HMI_ARPSettings_UpdateDisplay;
   arpSettingsPage.pRotaryEncoderChangedCallback = ARP_HMI_ARPSettingsPage_RotaryEncoderChanged;
   arpSettingsPage.pRotaryEncoderSelectCallback = ARP_HMI_ARPSettingsPage_RotaryEncoderSelected;
   arpSettingsPage.pPedalSelectedCallback = NULL;
   arpSettingsPage.pBackButtonCallback = NULL;
   arpSettingsPage.pBackPage = &homePage;

   dialogPage.pageID = PAGE_DIALOG;
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
      switch (toeNum) {
      case 1:
         // Toenum 1 always decrements.  Note that we don't have to check
         // range here.  PEDALS will do that and will call HMI_NotifyOctaveChange iff
         // there was a change.
         PEDALS_SetOctave(PEDALS_GetOctave() - 1);
         break;
      case 8:
         // Toenum 8 always increments
         PEDALS_SetOctave(PEDALS_GetOctave() + 1);
         break;
      default:
         // Toe switch numbers 2 through 7 are fixed starting at MIN_DIRECT_OCTAVE_NUMBER
         PEDALS_SetOctave(toeNum - 2 + MIN_DIRECT_OCTAVE_NUMBER);
         break;
      }
      // Note that we don't have to call HMI_UpdateIndicators because this will be 
      // call in HMI_NotifyOctaveChange
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
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      HMI_HandleVoicePresetsToeToggle(toeNum);
      break;
   case TOE_SWITCH_PATTERN_PRESETS:

      // TODO
      break;
   case TOE_SWITCH_ARP:
      switch (toeNum) {
      case 1:
         // Toe 1 decrements
         PEDALS_SetOctave(PEDALS_GetOctave() - 1);
         break;
      case 8:
         // Toe3 increments
         PEDALS_SetOctave(PEDALS_GetOctave() + 1);
         break;
      default:
         // switches 2-7 handled by separate module/function
         ARP_HMI_HandleArpLiveToeToggle(toeNum, pressed);
      }
      break;
   case TOE_SWITCH_CHORD:
      switch (toeNum) {
      case 1:
         // Toe 1 decrements
         PEDALS_SetOctave(PEDALS_GetOctave() - 1);
         break;
      case 8:
         // Toe3 increments
         PEDALS_SetOctave(PEDALS_GetOctave() + 1);
         break;
      default:
         // switches 2-7TODO
      }
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
   // Debounce the switch. Ignore unless a valid press greater than debounce interval
   u8 valid = HMI_DebounceSwitchChange(&stompSwitchState[stompNum - 1], pressed, timestamp);
   if (!valid) {
      return;
   }

   // Process the change according to the current switch setting
   switch (hmiSettings.stompSwitchSetting[stompNum - 1]) {
   case STOMP_SWITCH_OCTAVE_VOLUME:
      // On first press go to Octave.  On subsequent presses, alternate
      // between OCTAVE and VOLUME toe switches.
      switch (hmiSettings.toeSwitchMode) {
      case TOE_SWITCH_VOLUME:
         hmiSettings.toeSwitchMode = TOE_SWITCH_OCTAVE;
         break;
      case TOE_SWITCH_OCTAVE:
      default:
         hmiSettings.toeSwitchMode = TOE_SWITCH_VOLUME;
         break;
      }
      break;
   case STOMP_SWITCH_VOICE_PRESETS:
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_VOICE_PRESETS) {
         // Already in voice preset mode so cycle to the next bank
         hmiSettings.currentToeSwitchGenMIDIPresetBank++;
         if (hmiSettings.currentToeSwitchGenMIDIPresetBank > hmiSettings.numToeSwitchGenMIDIPresetBanks) {
            hmiSettings.currentToeSwitchGenMIDIPresetBank = 1;
         }
      }
      else {
         // just switch into preset mode
         hmiSettings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
      }
      break;
   case STOMP_SWITCH_PATTERN_PRESETS:
      hmiSettings.toeSwitchMode = TOE_SWITCH_PATTERN_PRESETS;
      break;
   case STOMP_SWITCH_ARPEGGIATOR:
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_ARP) {
         // Already selected toe mode so toggle it
         if (ARP_GetArpMode() != ARP_MODE_CHORD_ARP) {
            ARP_SetArpMode(ARP_MODE_CHORD_ARP);
            // And turn on Arpeggiator
            ARP_SetEnabled(1);
         }
         else {
            // Toggle the run state
            ARP_SetEnabled(!ARP_GetEnabled());
         }
      }
      hmiSettings.toeSwitchMode = TOE_SWITCH_ARP;
      // Set to home page
      pCurrentPage = &homePage;
      break;
   case STOMP_SWITCH_CHORD_PAD:
      // Got to Chord Pad mode.  If already there then toggle the enable state
      if (hmiSettings.toeSwitchMode == TOE_SWITCH_CHORD) {

         if (ARP_GetArpMode() != ARP_MODE_CHORD_PAD) {
            ARP_SetArpMode(ARP_MODE_CHORD_PAD);
            ARP_SetEnabled(1);
         }
         else {
            // Toggle the ARP on/off
            ARP_SetEnabled(!ARP_GetEnabled());
         }
      }
      hmiSettings.toeSwitchMode = TOE_SWITCH_CHORD;
      // Set to home page
      pCurrentPage = &homePage;
      break;
   default:
      // Invalid.  Just return;
      DEBUG_MSG("HMI_NotifyStompToggle:  Invalid setting %d for stompNum %d", hmiSettings.stompSwitchSetting[stompNum - 1], stompNum);
      return;
   }
   // Update the toe switch indicators and the display in case the mode changed.
   HMI_UpdateIndicators();

   // And persist the data in case anything changed
   HMI_PersistData();

   // update the current page display
   pCurrentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Helper to process a toe switch preset when in voice presets mode.
/////////////////////////////////////////////////////////////////////////////
void HMI_HandleVoicePresetsToeToggle(u8 toeNum) {
   switch (toeNum) {
   case 1:
      // Toe 1 decrements octave
      PEDALS_SetOctave(PEDALS_GetOctave() - 1);
      break;
   case 8:
      // Toenum 8 always increments
      PEDALS_SetOctave(PEDALS_GetOctave() + 1);
      break;
   default:
      // Toe switch numbers 2 through 7 are presets

      // Get the current preset and check if it is already selected or a change
      const midi_preset_num_t presetNum = {
         .bankNumber = hmiSettings.currentToeSwitchGenMIDIPresetBank,
         .presetBankIndex = toeNum - 1 };
      const midi_preset_num_t* presetNumPtr = MIDI_PRESETS_GetLastActivatedMIDIPreset();
      if ((presetNum.bankNumber == presetNumPtr->bankNumber) && (presetNum.presetBankIndex == presetNumPtr->presetBankIndex)) {
         // This is the same one so just update the preset with the current octave and volume setting
         midi_preset_t curPreset;
         if (MIDI_PRESETS_CopyPreset(presetNumPtr, &curPreset) == NULL) {
            DEBUG_MSG("HMI_NotifyToeToggle:  Invalid presetNum in call to MIDI_PRESETS_CopyPreset");
         }
         else {
            // Update the preset with current octave
            curPreset.octave = PEDALS_GetOctave();
            MIDI_PRESETS_SetMIDIPreset(presetNumPtr, &curPreset);
            // Temporarily flash the updated one
            IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST, IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON, 100);
         }
      }
      else {
         // Preset is a different one so activate it
         const midi_preset_num_t* ptr = MIDI_PRESETS_ActivateMIDIPreset(&presetNum);

         if (ptr != NULL) {
            // Update the indicators
            HMI_UpdateIndicators();
            // And update the current page display
            pCurrentPage->pUpdateDisplayCallback();
            // Set the lastSelectedMIDIProgNumber o a rotation of the encoder starts at this one
            // unless we are in the MIDIPresetPage in which case this would overwrite the program to set
            if (pCurrentPage->pageID != PAGE_MIDI_PROGRAM_SELECT) {
               hmiSettings.lastSelectedMIDIProgNumber = MIDI_PRESETS_GetMidiPreset(ptr)->programNumber;
            }
         }
         else {
            // Invalid preset.  Flash and then off
            IND_SetTempIndicatorState(toeNum, IND_FLASH_FAST,
               IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_OFF, 100);
         }
      }
      break;
   }
}
/////////////////////////////////////////////////////////////////////////////
// Helper to restore the indicators based on the toe switch mode.
/////////////////////////////////////////////////////////////////////////////
void HMI_UpdateIndicators() {
   IND_ClearAll();

   //  process according to toeSwitchMode
   s8 octave = PEDALS_GetOctave();
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_VOLUME:
      IND_SetIndicatorState(HMI_GetToeVolumeIndex(), IND_ON, 100, IND_RAMP_NONE);
      // Now set the stomp indicator to Green
      IND_SetIndicatorColor(IND_STOMP_5, IND_COLOR_GREEN);
      IND_SetIndicatorState(IND_STOMP_5, IND_ON, 100, IND_RAMP_NONE);
      break;
   case TOE_SWITCH_OCTAVE:
      // For Octave, pedals 1 and 8 increment/decrement and indicators 2-7 are fixed starting
      // at MIN_DIRECT_OCTAVE_NUMBER.  On the first click below this, indicator 1 is solid,
      // one more click down it blips.  Same behavior on the top side with indicator 8.
      if (octave < MIN_DIRECT_OCTAVE_NUMBER - 1) {
         IND_SetBlipIndicator(1, 0, 2.0, 100);
      }
      else if (octave > MIN_DIRECT_OCTAVE_NUMBER + 6) {
         IND_SetBlipIndicator(8, 0, 2.0, 100);
      }
      else {
         // Indicators 2 through 7 show direct octave.
         IND_SetIndicatorState(octave - MIN_DIRECT_OCTAVE_NUMBER + 2, IND_ON, 100, IND_RAMP_NONE);
      }
      // Now set the stomp indicator to RED
      IND_SetIndicatorColor(IND_STOMP_5, IND_COLOR_RED);
      IND_SetIndicatorState(IND_STOMP_5, IND_ON, 100, IND_RAMP_NONE);
      break;
   case TOE_SWITCH_ARP:
      if (octave < MIN_DIRECT_OCTAVE_NUMBER - 1) {
         IND_SetBlipIndicator(1, 0, 2.0, 100);
      }
      else if (octave > MIN_DIRECT_OCTAVE_NUMBER + 6) {
         IND_SetBlipIndicator(8, 0, 2.0, 100);
      }
      else {
         // Call separate function in ARP_HMI to handle toe indicators 2-7 and stomp indicator
         ARP_HMI_UpdateArpIndicators();
      }
      break;
   case TOE_SWITCH_CHORD:
      ARP_HMI_UpdateChordIndicators();
      break;
   case TOE_SWITCH_VOICE_PRESETS:
      // Set the indicator for the bank index + 1 of the last activated presetNum if we 
      // are currently on that bank.
      const midi_preset_num_t* presetNum = MIDI_PRESETS_GetLastActivatedMIDIPreset();
      if (presetNum->bankNumber == hmiSettings.currentToeSwitchGenMIDIPresetBank) {
         IND_SetIndicatorState(presetNum->presetBankIndex + 1, IND_ON, 100, IND_RAMP_NONE);
      }
      // And set the color of the stomp indicator according to the bank for additional visibility
      // Flash the 4th bank since we only have 3 colors
      indicator_color_t color = IND_COLOR_RED;
      indicator_states_t state = IND_ON;
      switch (hmiSettings.currentToeSwitchGenMIDIPresetBank) {
      case 1:
         color = IND_COLOR_RED;
         break;
      case 2:
         color = IND_COLOR_YELLOW;
         break;
      case 3:
         color = IND_COLOR_GREEN;
         break;
      case 4:
         color = IND_COLOR_GREEN;
         state = IND_FLASH_BLIP;
         break;
      }
      IND_SetIndicatorColor(IND_STOMP_2, color);
      IND_SetIndicatorState(IND_STOMP_2, state, 100, IND_RAMP_NONE);
      break;
   case TOE_SWITCH_PATTERN_PRESETS:
      IND_SetIndicatorColor(IND_STOMP_1, IND_COLOR_YELLOW);
      IND_SetIndicatorState(IND_STOMP_1, IND_ON, 100, IND_RAMP_NONE);
      break;
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

   // Get the current Bank and Preset names
   const midi_preset_num_t* presetNum = MIDI_PRESETS_GetLastActivatedMIDIPreset();
   const midi_preset_t* preset = MIDI_PRESETS_GetMidiPreset(presetNum);
   const char* presetName = MIDI_PRESETS_GetMIDIVoiceName(preset->programNumber);
   const char* bankNamePtr = MIDI_PRESETS_GetGenMidiBankName(hmiSettings.currentToeSwitchGenMIDIPresetBank);

   char lineBuffer[DISPLAY_CHAR_WIDTH + 1];
   char scratchBuffer[DISPLAY_CHAR_WIDTH + 1];

   ///////////////////////////////////////
   // line 0 based oin toeSwitchMode
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
   case TOE_SWITCH_VOICE_PRESETS:
      // Render line 0 with the mode and bank name.
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s:%s",
         pToeSwitchModeTitles[hmiSettings.toeSwitchMode], bankNamePtr);
      HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);
      break;
   case TOE_SWITCH_ARP:
   case TOE_SWITCH_CHORD:
      // Toeswitchmode and preset name since preset can't be displayed on line 2
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s:%s",
         pToeSwitchModeTitles[hmiSettings.toeSwitchMode], presetName);
      HMI_RenderLine(0, lineBuffer, RENDER_LINE_CENTER);
      break;
   }


   // Spacer on line 1
   HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);
   // Current octave, arp state, and volume always go on line 3
   snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "Oct:%d V:%d Arp:%s",
      PEDALS_GetOctave(), HMI_GetToeVolumeIndex(), ARP_GetArpStateText());
   HMI_RenderLine(3, lineBuffer, RENDER_LINE_LEFT);

   ///////////////////////////////////////
   // Render line 2 based on the current toe switch mode
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
   case TOE_SWITCH_VOICE_PRESETS:
      // Preset name on line 2
      HMI_RenderLine(2, presetName, RENDER_LINE_LEFT);
      break;
   case TOE_SWITCH_PATTERN_PRESETS:
      // TODO - Render the pattern preset line
      HMI_ClearLine(2);
      break;
   case TOE_SWITCH_ARP:
      // Arpeggiator details in Line 2 insted of preset name
      u16 bpm = ARP_GetBPM();
      // Truncate scale/mode name.  Had problems with snprintf for some reason so
      // went to memcpy instead.  
      // TODO: Figure out why snprintf didn't work.
      const char* scaleName = SEQ_SCALE_NameGet(ARP_GetModeScale());
      u8 truncLength = 9;
      u8 nameLength = strlen(scaleName);
      if (nameLength < truncLength) {
         truncLength = nameLength;
      }
      memcpy(scratchBuffer, scaleName, truncLength);
      scratchBuffer[truncLength] = '\0';

      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s %s %s %d",
         ARP_MODES_GetNoteName(ARP_GetRootKey()), scratchBuffer, ARP_PAT_GetCurrentPatternShortName(), bpm);
      HMI_RenderLine(2, lineBuffer, RENDER_LINE_LEFT);
      break;
   case TOE_SWITCH_CHORD:
      // Show Root and ModeScale 
      snprintf(lineBuffer, DISPLAY_CHAR_WIDTH + 1, "%s %s",
         ARP_MODES_GetNoteName(ARP_GetRootKey()), SEQ_SCALE_NameGet(ARP_GetModeScale()));
      HMI_RenderLine(2, lineBuffer, RENDER_LINE_LEFT);
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
   case TOE_SWITCH_VOICE_PRESETS:
      // Go MIDI program number page
      midiProgramSelectPage.pBackPage = pCurrentPage;
      pCurrentPage = &midiProgramSelectPage;
      break;
   case TOE_SWITCH_ARP:
      arpSettingsPage.pBackPage = pCurrentPage;
      pCurrentPage = &arpSettingsPage;
      return;
   case TOE_SWITCH_PATTERN_PRESETS:
      // Goto Pattern select page
      // TODO
      return;
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

}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on home page
/////////////////////////////////////////////////////////////////////////////
void HMI_HomePage_RotaryEncoderSelect() {
   switch (hmiSettings.toeSwitchMode) {
   case TOE_SWITCH_VOICE_PRESETS:
      // Go MIDI program number page
      midiProgramSelectPage.pBackPage = pCurrentPage;
      pCurrentPage = &midiProgramSelectPage;
      break;
   case TOE_SWITCH_ARP:
      // TODO - Got to the next pattern
      return;
   case TOE_SWITCH_PATTERN_PRESETS:
      // Goto Pattern select page
      // TODO
      return;
   case TOE_SWITCH_OCTAVE:
   case TOE_SWITCH_VOLUME:
      if (pCurrentPage != &dialogPage) {
         snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "About M3-SuperPedal");
         snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", M3_SUPERPEDAL_VERSION);
         snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", M3_SUPERPEDAL_VERSION_DATE);

         dialogPage.pBackPage = pCurrentPage;
         pCurrentPage = &dialogPage;
      }
      else{
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


////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the Voice Preset Edit Page
////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_UpdateDisplay() {

   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(pCurrentPage->pPageTitle);

   // Selected entry on line 2
   HMI_RenderLine(2, editVoicePageEntries[lastSelectedEditVoicePresetPageEntry], RENDER_LINE_SELECT);
   if (lastSelectedEditVoicePresetPageEntry == 0) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, editVoicePageEntries[lastSelectedEditVoicePresetPageEntry - 1], RENDER_LINE_CENTER);
   }
   if (lastSelectedEditVoicePresetPageEntry == NUM_EDIT_VOICE_PRESET_PAGE_ENTRIES - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, editVoicePageEntries[lastSelectedEditVoicePresetPageEntry + 1], RENDER_LINE_CENTER);
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
   pCurrentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on Edit Voice Preset Page
/////////////////////////////////////////////////////////////////////////////
void HMI_EditVoicePresetPage_RotaryEncoderSelected() {
   switch (lastSelectedEditVoicePresetPageEntry) {
   case EDIT_VOICE_PRESET_PAGE_ENTRY_PROGRAM_NUMBER:
      editVoicePresetPage.pBackPage = pCurrentPage;
      pCurrentPage = &midiProgramSelectPage;
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
   pCurrentPage->pUpdateDisplayCallback();
}
////////////////////////////////////////////////////////////////////////////
// Callback to update the display on the MIDIProgramSelectPage
////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_UpdateDisplay() {
   MIOS32_LCD_CursorSet(0, 0);
   MIOS32_LCD_PrintString(pCurrentPage->pPageTitle);

   // Print 3 presets centered around the current selection.

   // Selected entry on line 2
   u8 lastProgNumber = hmiSettings.lastSelectedMIDIProgNumber;
   HMI_RenderLine(2, MIDI_PRESETS_GetMIDIVoiceName(lastProgNumber), RENDER_LINE_SELECT);
   if (lastProgNumber == 1) {
      // At the beginning. Clear line 1
      HMI_ClearLine(1);
   }
   else {
      HMI_RenderLine(1, MIDI_PRESETS_GetMIDIVoiceName(lastProgNumber - 1), RENDER_LINE_CENTER);
   }
   if (lastProgNumber == MIDI_PRESETS_GetNumMIDIVoices() - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      HMI_RenderLine(3, MIDI_PRESETS_GetMIDIVoiceName(lastProgNumber + 1), RENDER_LINE_CENTER);
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on general MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderChanged(s8 increment) {
   s32 progNumber = hmiSettings.lastSelectedMIDIProgNumber;
   progNumber += increment;
   if (progNumber < 0) {
      progNumber = 0;
   }
   if (progNumber >= MIDI_PRESETS_GetNumMIDIVoices()) {
      progNumber = MIDI_PRESETS_GetNumMIDIVoices() - 1;
   }
   if (hmiSettings.lastSelectedMIDIProgNumber != progNumber) {
      hmiSettings.lastSelectedMIDIProgNumber = progNumber;
      // Activate the MIDI voice temporarily using midi config from current preset
      const midi_preset_num_t* presetNum = MIDI_PRESETS_GetLastActivatedMIDIPreset();
      const midi_preset_t* preset = MIDI_PRESETS_GetMidiPreset(presetNum);
      // TODO handle bankNumber too
      MIDI_PRESETS_ActivateMIDIVoice(progNumber, 0, preset->midiPorts, preset->midiChannel);
      // And force an update to the current page display
      pCurrentPage->pUpdateDisplayCallback();
   }
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_RotaryEncoderSelected() {
   // Replace the last activated preset with the new program number
   const midi_preset_num_t* presetNum = MIDI_PRESETS_GetLastActivatedMIDIPreset();
   const midi_preset_t setPreset = {
      .programNumber = hmiSettings.lastSelectedMIDIProgNumber,
      .midiBankNumber = 0,
      .midiPorts = hmiSettings.lastSelectedMidiOutput,
      .midiChannel = hmiSettings.lastSelectedMidiChannel,
      .octave = PEDALS_GetOctave() };
   const midi_preset_t* presetPtr = MIDI_PRESETS_SetMIDIPreset(presetNum, &setPreset);

   if (presetPtr == NULL) {
      DEBUG_MSG("HMI_MIDIProgramSelectPage_RotaryEncoderSelected:  Error editing preset");
   }
   else {
      // Success, set the toe switch mode to VOICE_PRESET
      hmiSettings.toeSwitchMode = TOE_SWITCH_VOICE_PRESETS;
      // Force the current bank to the bank number of this preset
      hmiSettings.currentToeSwitchGenMIDIPresetBank = presetNum->bankNumber;
      // Update the indicators
      HMI_UpdateIndicators();
      // And then flash the newly replaced preset
      IND_SetTempIndicatorState(presetNum->presetBankIndex + 1, IND_FLASH_FAST,
         IND_TEMP_FLASH_STATE_DEFAULT_DURATION, IND_ON, 100);
   }

   // Go back to the home page 
   pCurrentPage = &homePage;
   // And force an update to the current page display
   pCurrentPage->pUpdateDisplayCallback();
   // And send out the preset change
}

/////////////////////////////////////////////////////////////////////////////
// Callback for back button pressed on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void HMI_MIDIProgramSelectPage_BackButtonCallback() {
   // On a back button press, user did not select a new preset so re-activate the
   // last one.
   const midi_preset_num_t* preset = MIDI_PRESETS_GetLastActivatedMIDIPreset();
   MIDI_PRESETS_ActivateMIDIPreset(preset);
   // And restore the pointer in the program number to the current preset so that the
   // next time into the menu we start at that one.
   hmiSettings.lastSelectedMIDIProgNumber = MIDI_PRESETS_GetMidiPreset(preset)->programNumber;
}

/////////////////////////////////////////////////////////////////////////////
// Helper to store persisted data 
/////////////////////////////////////////////////////////////////////////////
s32 HMI_PersistData() {
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