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
#include "arp_hmi.h"
#include "arp_pattern.h"
#include "arp.h"
#include "pedals.h"
#include "indicators.h"
#include "persist.h"

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG
#undef HW_DEBUG

//----------------------------------------------------------------------------
//  defines and macros
//----------------------------------------------------------------------------

#define TEMPO_CHANGE_STEP 5

//----------------------------------------------------------------------------
// ARP-HMI specific Switch types and non-persisted variables
//----------------------------------------------------------------------------

// Total list of functions availabe in ARP Live mode.  Note that toe switches 1 and 8 are
// dedicated to Octave decrement/increment respectively
typedef enum arp_live_toe_functions_e {
   ARP_TOE_SELECT_KEY = 2,
   ARP_TOE_SELECT_MODE_SCALE = 3,
   ARP_TOE_DECREMENT_TEMPO = 4,
   ARP_TOE_INCREMENT_TEMPO = 5,
   ARP_TOE_SET_ARP_MODE = 6
   // 7 is unused
} arp_live_toe_functions_t;


typedef enum arp_settings_entries_e {
   ARP_SETTINGS_MIDI_CHANNEL = 0,
   ARP_SETTINGS_CLOCK_MODE = 1
} arp_settings_entries_t;

const char* arpSettingsPageEntryTitles[] = { "Set Midi Out Channel","Set Clock Mode" };
#define ARP_SETTINGS_PAGE_NUM_ENTRIES 2

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------

void ARP_HMI_SelectRootKeyCallback(u8 pedalNum);
void ARP_HMI_SelectModeScaleCallback(u8 pedalNum);
const char* ARP_HMI_GetClockModeText(arp_clock_mode_t mode);
void ARP_HMI_ARPSettingsValues_RotaryEncoderChanged(s8 increment);


//----------------------------------------------------------------------------
// Local variables
//----------------------------------------------------------------------------
persisted_arp_hmi_data_t arpHMISettings;

static s16 lastARPPatternIndex = 0;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 ARP_HMI_Init()
{
   // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
   arpHMISettings.serializationID = 0x41484D31;   // 'AHM1'

   s32 valid = PERSIST_ReadBlock(PERSIST_ARP_HMI_BLOCK, (unsigned char*)&arpHMISettings, sizeof(persisted_arp_hmi_data_t));
   if (valid < 0) {
      DEBUG_MSG("ARP_HMI_Init:  PERSIST_ReadBlock return invalid. Re-initing persisted settings to defaults");

      arpHMISettings.lastArpSettingsPageIndex = 0;

      ARP_HMI_PersistData();
   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Global function to store persisted arpeggiator data
/////////////////////////////////////////////////////////////////////////////
s32 ARP_HMI_PersistData() {
#ifdef DEBUG
   DEBUG_MSG("ARP_HMI_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(persisted_arp_hmi_data_t));
#endif
   s32 valid = PERSIST_StoreBlock(PERSIST_ARP_HMI_BLOCK, (unsigned char*)&arpHMISettings, sizeof(persisted_arp_hmi_data_t));
   if (valid < 0) {
      DEBUG_MSG("ARP_HMI_PersistData:  Error persisting setting to EEPROM");
   }
   return valid;
}
////////////////////////////////////////////////////////////////////////////
// Callback to update the arp pattern select display
////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPPatternPage_UpdateDisplay() {
   HMI_RenderLine(0, arpPatternPage.pPageTitle, RENDER_LINE_CENTER);
   // Spacer on line 1
   HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);

   // Print up to 2 entries with current selection on line 2

   // Selected entry on line 2
   HMI_RenderLine(2, ARP_PAT_GetPatternName(lastARPPatternIndex), RENDER_LINE_SELECT);
   if (lastARPPatternIndex >= NUM_PATTERNS - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      // Print next item on line 3
      HMI_RenderLine(3, ARP_PAT_GetPatternName(lastARPPatternIndex + 1), RENDER_LINE_CENTER);
   }

}
////////////////////////////////////////////////////////////////////////////
// Callback to update the arp settings display
////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPSettings_UpdateDisplay() {
   HMI_RenderLine(0, arpSettingsPage.pPageTitle, RENDER_LINE_CENTER);
   // Spacer on line 1
   HMI_RenderLine(1, "--------------------", RENDER_LINE_LEFT);

   // Print up to 2 entries with current selection on line 2

   // Selected entry on line 2
   HMI_RenderLine(2, arpSettingsPageEntryTitles[arpHMISettings.lastArpSettingsPageIndex], RENDER_LINE_SELECT);
   if (arpHMISettings.lastArpSettingsPageIndex == ARP_SETTINGS_PAGE_NUM_ENTRIES - 1) {
      // At the end, clear line 3
      HMI_ClearLine(3);
   }
   else {
      // Print next item on line 3
      HMI_RenderLine(3, arpSettingsPageEntryTitles[arpHMISettings.lastArpSettingsPageIndex + 1], RENDER_LINE_CENTER);
   }
}

/////////////////////////////////////////////////////////////////////////////
// Callback for select arp pattern page rotary encoder select
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPPatternPage_RotaryEncoderSelected() {
   ARP_PAT_SetCurrentPattern(lastARPPatternIndex);
   // Go back to the previous page
   pCurrentPage = pCurrentPage->pBackPage;
   // And force an update to the current page display
   pCurrentPage->pUpdateDisplayCallback();
}

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder change on arp pattern page
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPPatternPage_RotaryEncoderChanged(s8 increment) {
   s16 index = lastARPPatternIndex;
   index += increment;
   if (index < 0) {
      index = 0;
   }
   else if (index >= NUM_PATTERNS) {
      index = NUM_PATTERNS - 1;
   }
   if (index == lastARPPatternIndex) {
      return;
   }
   lastARPPatternIndex = index;
   // Activate the pattern, but don't set it as current until/unless user selects encoder.  
   ARP_PAT_ActivatePattern(lastARPPatternIndex);

   // force an update to the current page display
   pCurrentPage->pUpdateDisplayCallback();
}

/////////////////////////////////////////////////////////////////////////////
// Callback for back button pressed on arp pattern select page.
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPPatternPage_BackButtonCallback() {
   // On a back button press, user did not select a new pattern so restore the
   // current one
   lastARPPatternIndex = ARP_GetARPSettings()->arpPatternIndex;
   ARP_PAT_ActivatePattern(lastARPPatternIndex);
}

/////////////////////////////////////////////////////////////////////////////
// Callback for select arp settings page rotary encoder change
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPSettingsPage_RotaryEncoderChanged(s8 increment) {
   s32 index = arpHMISettings.lastArpSettingsPageIndex + increment;
   if (index < 0) {
      index = 0;
   }
   else if (index >= ARP_SETTINGS_PAGE_NUM_ENTRIES) {
      index = ARP_SETTINGS_PAGE_NUM_ENTRIES - 1;
   }
   if (index == arpHMISettings.lastArpSettingsPageIndex)
      return;
   arpHMISettings.lastArpSettingsPageIndex = index;
   // force an update to the current page display
   pCurrentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder select on gen MIDI select page
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPSettingsPage_RotaryEncoderSelected() {
   switch (arpHMISettings.lastArpSettingsPageIndex) {
   case ARP_SETTINGS_MIDI_CHANNEL:
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH, "SELECT MIDI CHANNEL");

      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH, "Channel: %d", ARP_GetMIDIChannel());
      dialogPageMessage2[0] = '\0';
      dialogPage.pBackPage = pCurrentPage;
      dialogPage.pRotaryEncoderChangedCallback = ARP_HMI_ARPSettingsValues_RotaryEncoderChanged;
      dialogPage.pBackButtonCallback = NULL;
      break;
   case ARP_SETTINGS_CLOCK_MODE:
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH, "SELECT CLOCK MODE");
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH, "Clock Mode: %s", ARP_HMI_GetClockModeText(ARP_GetClockMode()));
      dialogPageMessage2[0] = '\0';
      dialogPage.pRotaryEncoderChangedCallback = ARP_HMI_ARPSettingsValues_RotaryEncoderChanged;
      dialogPage.pBackPage = pCurrentPage;
      dialogPage.pBackButtonCallback = NULL;
      break;
   }
   pCurrentPage = &dialogPage;
   // And force an update to the current page display
   pCurrentPage->pUpdateDisplayCallback();
}

/////////////////////////////////////////////////////////////////////////////
// Callback for rotary encoder changes on the ARP Settings subpages.
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPSettingsValues_RotaryEncoderChanged(s8 increment) {
   switch (arpHMISettings.lastArpSettingsPageIndex) {
   case ARP_SETTINGS_MIDI_CHANNEL:
      u8 channel = ARP_GetMIDIChannel();
      channel += increment;
      if (channel > 16) {
         channel = 16;
      }
      else if (channel < 1) {
         channel = 1;
      }
      ARP_SetMIDIChannel(channel);
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH, "Channel: %d", ARP_GetMIDIChannel());
      break;
   case ARP_SETTINGS_CLOCK_MODE:
      arp_clock_mode_t mode = ARP_GetClockMode();
      mode += increment;
      if (mode < 0) {
         mode = 0;
      }
      if (mode > ARP_CLOCK_MODE_SLAVE) {
         mode = ARP_CLOCK_MODE_SLAVE;
      }
      ARP_SetClockMode(mode);
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH, "Clock Mode: %s", ARP_HMI_GetClockModeText(ARP_GetClockMode()));
      break;
   }
   pCurrentPage->pUpdateDisplayCallback();
}
/////////////////////////////////////////////////////////////////////////////
// Helper for Clock Mode text
/////////////////////////////////////////////////////////////////////////////
const char* ARP_HMI_GetClockModeText(arp_clock_mode_t mode) {
   switch (mode) {
   case ARP_CLOCK_MODE_MASTER:
      return "Master";
   case ARP_CLOCK_MODE_SLAVE:
      return "Slave";
   }
   return "ERR!";
}

/////////////////////////////////////////////////////////////////////////////
// Sets/updates the indicators for the Arp mode
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_UpdateArpIndicators() {
   // Update stomp indicator to Red if any mode active, or
   // Green/Yellow depending on the the ARP mode
   indicator_color_t color = IND_COLOR_RED;
   if (!ARP_GetEnabled()) {
      // Arp not running so set indicator to Green or Yellow for ARP vs. PAD
      switch (ARP_GetArpMode()) {
      case ARP_MODE_CHORD_ARP:
         color = IND_COLOR_GREEN;
         break;
      case ARP_MODE_CHORD_PAD:
         color = IND_COLOR_YELLOW;
         break;
      case ARP_MODE_KEYS:
         // TODO - Find another color for Keys
       //  color = IND_COLOR_RED;
         break;
      }
   }
   IND_SetIndicatorColor(IND_STOMP_4, color);
   IND_SetIndicatorState(IND_STOMP_4, IND_ON, 100, IND_RAMP_NONE);

   // TODO the state of the Toe indicators
   
}

/////////////////////////////////////////////////////////////////////////////
// Helper to handle Arp live toe toggle. 
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_HandleArpToeToggle(u8 toeNum, u8 pressed) {
   u16 bpm;
   switch (toeNum) {
   case ARP_TOE_SELECT_KEY:
      /// Go to the dialog page
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "SET ARP ROOT KEY");
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", "Press Pedal to");
      snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", "Select Key");
      dialogPage.pBackPage = pCurrentPage;
      pCurrentPage = &dialogPage;
      pCurrentPage->pUpdateDisplayCallback();

      // Flash the indicators
      IND_FlashAll(0);
      // Set the pedal callback to get the selected root key
      PEDALS_SetSelectPedalCallback(&ARP_HMI_SelectRootKeyCallback);
      break;
   case ARP_TOE_SELECT_MODE_SCALE:
      // Go to the dialog page
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "SET ARP MODAL SCALE");
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", "Press Brown Pedal to");
      snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", "Select Mode");
      dialogPage.pBackPage = pCurrentPage;
      pCurrentPage = &dialogPage;
      pCurrentPage->pUpdateDisplayCallback();

      IND_FlashAll(0);

      // Set the pedal callback to get the selected root key
      PEDALS_SetSelectPedalCallback(&ARP_HMI_SelectModeScaleCallback);
      break;
   case ARP_TOE_INCREMENT_TEMPO:
      bpm = ARP_GetBPM();
      bpm += TEMPO_CHANGE_STEP;
      ARP_SetBPM(bpm);
      // Flash the indicator for confirmation
      IND_SetTempIndicatorState(toeNum,IND_FLASH_FAST,IND_TEMP_FLASH_STATE_DEFAULT_DURATION,IND_OFF,100);      
      break;
   case ARP_TOE_DECREMENT_TEMPO:
      bpm = ARP_GetBPM();
      bpm -= TEMPO_CHANGE_STEP;
      ARP_SetBPM(bpm);
      // Flash the indicator for confirmation
      IND_SetTempIndicatorState(toeNum,IND_FLASH_FAST,IND_TEMP_FLASH_STATE_DEFAULT_DURATION,IND_OFF,100);      
      break;
   case ARP_TOE_SET_ARP_MODE:
      indicator_states_t targetState = IND_ON;
      switch(ARP_GetArpMode()){
         case ARP_MODE_CHORD_ARP:
            ARP_SetArpMode(ARP_MODE_CHORD_PAD);
            break;
         case ARP_MODE_CHORD_PAD:
            ARP_SetArpMode(ARP_MODE_CHORD_ARP);
            targetState = IND_FLASH_SLOW;
            break;
         // TODO - Add in arp mode keys someday
      }
      // Flash the indicator for confirmation.  
      IND_SetTempIndicatorState(toeNum,IND_FLASH_FAST,IND_TEMP_FLASH_STATE_DEFAULT_DURATION,targetState,100);
      break;
   }
   // Update the current display to reflect any content change
   pCurrentPage->pUpdateDisplayCallback();
}

/////////////////////////////////////////////////////////////////////////////
// Callback for selecting the arpeggiator key from the pedals.
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_SelectRootKeyCallback(u8 pedalNum) {
   ARP_SetRootKey(pedalNum - 1);
   // go back to last page and refresh displays
   pCurrentPage = &homePage;
   HMI_UpdateIndicators();
   pCurrentPage->pUpdateDisplayCallback();
}


/////////////////////////////////////////////////////////////////////////////
// Callback for selecting the modal scale from the pedals.  Only the main
// keys are valid.
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_SelectModeScaleCallback(u8 pedalNum) {
#ifdef DEBUG
   DEBUG_MSG("HMI_SelectModeScaleCallback called with pedal #%d", pedalNum);
#endif

   scale_t mode;
   u8 valid = 1;
   switch (pedalNum) {
   case 1:
      mode = SCALE_IONIAN;
      break;
   case 3:
      mode = SCALE_DORIAN;
      break;
   case 5:
      mode = SCALE_PHRYGIAN;
      break;
   case 6:
      mode = SCALE_LYDIAN;
      break;
   case 8:
      mode = SCALE_MIXOLYDIAN;
      break;
   case 10:
      mode = SCALE_AEOLIAN;
      break;
   case 12:
      mode = SCALE_LOCRIAN;
      break;
   default:
      // invalid
      valid = 0;
   }
   if (valid) {
      ARP_SetModeScale(mode);
   }
   // go back to home page and refresh displays
   pCurrentPage = &homePage;
   HMI_UpdateIndicators();
   pCurrentPage->pUpdateDisplayCallback();
}
