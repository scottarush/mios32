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


//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------

void ARP_HMI_SelectRootKeyCallback(u8 pedalNum);
void ARP_HMI_SelectModeScaleCallback(u8 pedalNum);


/////////////////////////////////////////////////////////////////////////////
// Sets/updates the indicators for the current ARP_LIVE mode
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_SetArpSettingsIndicators() {
   IND_ClearAll();
   if (ARP_GetEnabled() == 0) {
      return;  // Arp is disabled so leave all the indicators off
   }
   // Else, synch the indicators to the Arp state.

   // First the gen mode indicators
   switch (ARP_GetArpGenOrder()) {
   case ARP_GEN_ORDER_ASCENDING:
      IND_SetBlipIndicator(ARP_LIVE_TOE_GEN_ORDER, 0, ARP_GetBPM() / 60);
      break;
   case ARP_GEN_ORDER_DESCENDING:
      IND_SetBlipIndicator(ARP_LIVE_TOE_GEN_ORDER, 1, ARP_GetBPM() / 60);
      break;
   case ARP_GEN_ORDER_ASC_DESC:
      // TODO
      break;
   case ARP_GEN_ORDER_RANDOM:
      // TODO
      break;
   }
   // TODO the rest
   // IND_SetIndicatorState(1, IND_FLASH_BLIP);
}

/////////////////////////////////////////////////////////////////////////////
// Helper to handle Arp live toe toggle. 
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_HandleArpLiveToeToggle(u8 toeNum, u8 pressed) {
   u16 bpm;
   switch (toeNum) {
   case ARP_LIVE_TOE_GEN_ORDER:
      // Wrap the 3 modes on this toe switch
      switch (ARP_GetArpGenOrder()) {
      case ARP_GEN_ORDER_ASCENDING:
         ARP_SetArpGenOrder(ARP_GEN_ORDER_DESCENDING);
         break;
      case ARP_GEN_ORDER_DESCENDING:
         ARP_SetArpGenOrder(ARP_GEN_ORDER_ASC_DESC);
         break;
      case ARP_GEN_ORDER_ASC_DESC:
         ARP_SetArpGenOrder(ARP_GEN_ORDER_RANDOM);
         break;
      case ARP_GEN_ORDER_RANDOM:
         ARP_SetArpGenOrder(ARP_GEN_ORDER_ASCENDING);
      }
      break;
   case ARP_LIVE_TOE_SELECT_KEY:
      /// Go to the dialog page
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "Set Arp Root Key");
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", "Press Pedal to");
      snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", "Select Key");
      dialogPage.pBackPage = currentPage;
      currentPage = &dialogPage;
      currentPage->pUpdateDisplayCallback();

      // Flash the indicators
      IND_FlashAll(0);
      // Set the pedal callback to get the selected root key
      PEDALS_SetSelectPedalCallback(&ARP_HMI_SelectRootKeyCallback);
      break;
   case ARP_LIVE_TOE_SELECT_MODAL_SCALE:
      // Go to the dialog page
      snprintf(dialogPageTitle, DISPLAY_CHAR_WIDTH + 1, "%s", "Set Arp Modal Scale  ");
      snprintf(dialogPageMessage1, DISPLAY_CHAR_WIDTH + 1, "%s", "Press Brown Pedal to");
      snprintf(dialogPageMessage2, DISPLAY_CHAR_WIDTH + 1, "%s", "Select Mode");
      dialogPage.pBackPage = currentPage;
      currentPage = &dialogPage;
      currentPage->pUpdateDisplayCallback();

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
   currentPage->pUpdateDisplayCallback();
}

/////////////////////////////////////////////////////////////////////////////
// Callback for selecting the arpeggiator key from the pedals.
/////////////////////////////////////////////////////////////////////////////
void ARP_HMI_SelectRootKeyCallback(u8 pedalNum) {
   ARP_SetRootKey(pedalNum - 1);
   // go back to last page and refresh displays
   currentPage = &homePage;
   HMI_UpdateIndicators();
   currentPage->pUpdateDisplayCallback();
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
   currentPage = &homePage;
   HMI_UpdateIndicators();
   currentPage->pUpdateDisplayCallback();
}
