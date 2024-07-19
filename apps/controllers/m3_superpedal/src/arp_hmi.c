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
   ARP_LIVE_TOE_SELECT_KEY = 2,
   ARP_LIVE_TOE_SELECT_MODAL_SCALE = 3,
   ARP_LIVE_TOE_GEN_ORDER = 4,
   // 5 is unused
   ARP_LIVE_TOE_TEMPO_DECREMENT = 6,
   ARP_LIVE_TOE_TEMPO_INCREMENT = 7,
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
// Callback to update the display on the MIDIProgramSelectPage
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
// Callback for rotary encoder change on arp settings page
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_ARPSettingsPage_RotaryEncoderChanged(s8 increment) {
   arpHMISettings.lastArpSettingsPageIndex += increment;
   if (arpHMISettings.lastArpSettingsPageIndex < 0) {
      arpHMISettings.lastArpSettingsPageIndex = 0;
   }
   else if (arpHMISettings.lastArpSettingsPageIndex >= ARP_SETTINGS_PAGE_NUM_ENTRIES) {
      arpHMISettings.lastArpSettingsPageIndex = ARP_SETTINGS_PAGE_NUM_ENTRIES - 1;
   }
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
   // Set the stomp indicator to Red if running, Green if paused
   indicator_color_t color = IND_COLOR_GREEN;
   if (ARP_GetEnabled()) {
      color = IND_COLOR_RED;
   }
   IND_SetIndicatorColor(IND_STOMP_4, color);
   IND_SetIndicatorState(IND_STOMP_4, IND_ON, 100, IND_RAMP_NONE);

   // TODO the rest
   // IND_SetIndicatorState(1, IND_FLASH_BLIP);
}

/////////////////////////////////////////////////////////////////////////////
// Sets/updates the indicators for the Arp Chord mode
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_UpdateChordIndicators() {
   // Update stomp state according to the ARP mode
   indicator_color_t color = IND_COLOR_GREEN;
   if (ARP_GetEnabled()) {
      switch (ARP_GetArpMode()) {
      case ARP_MODE_CHORD_ARP:
         color = IND_COLOR_GREEN;
         break;
      case ARP_MODE_CHORD_PAD:
         color = IND_COLOR_YELLOW;
         break;
      case ARP_MODE_KEYS:
         color = IND_COLOR_RED;
         break;
      }
   }
   IND_SetIndicatorColor(IND_STOMP_3, color);
   IND_SetIndicatorState(IND_STOMP_3, IND_ON, 100, IND_RAMP_NONE);
}

/////////////////////////////////////////////////////////////////////////////
// Helper to handle Arp live toe toggle. 
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_HandleArpLiveToeToggle(u8 toeNum, u8 pressed) {
   u16 bpm;
   switch (toeNum) {
   case ARP_LIVE_TOE_SELECT_KEY:
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
   case ARP_LIVE_TOE_SELECT_MODAL_SCALE:
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
   case ARP_LIVE_TOE_TEMPO_INCREMENT:
      bpm = ARP_GetBPM();
      bpm += TEMPO_CHANGE_STEP;
      ARP_SetBPM(bpm);
      break;
   case ARP_LIVE_TOE_TEMPO_DECREMENT:
      bpm = ARP_GetBPM();
      bpm -= TEMPO_CHANGE_STEP;
      ARP_SetBPM(bpm);
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
