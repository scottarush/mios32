/*
 * include for M3 super pedal HMI
 *  
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 */

#ifndef _HMI_H
#define _HMI_H

#include "keyboard.h"
#include "switches.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define DISPLAY_CHAR_WIDTH 16

#define NUM_TOE_GEN_MIDI_PRESET_BANKS 4

#define DEBOUNCE_TIME_MS 40

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum pageID_e {
   PAGE_HOME = 0,
   PAGE_DIALOG = 1,
   PAGE_SPLIT_LEARNING = 2,
   PAGE_MIDI_CONFIG = 3
} pageID_t;

typedef enum flash_state_e {
   FLASH_OFF = 0,
   FLASH_STATE_ONE = 1,
   FLASH_STATE_TWO = 2
} flash_state_t;


struct page_s {
   pageID_t pageID;
   void (*pUpdateDisplayCallback)();
   void (*pBackButtonCallback)();
   void (*pEnterButtonCallback)();
   void (*pUpDownButtonCallback)(u8 up);
   void (*pTimerCallback)();
   int timerCounter;
   int flashTimeMS;
   flash_state_t nextFlashState;
   struct page_s* pBackPage;
};

typedef enum renderline_justify_e {
   RENDER_LINE_LEFT = 0,
   RENDER_LINE_CENTER = 1,
   RENDER_LINE_SELECT = 2,
   RENDER_LINE_RIGHT = 3
} renderline_justify_t;



//----------------------------------------------------------------------------
// Export Global Display page variables
//----------------------------------------------------------------------------
extern struct page_s homePage;
extern struct page_s dialogPage;
extern struct page_s* pCurrentPage;

// Buffer for dialog Page Title
extern char dialogPageTitle[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 1
extern char dialogPageMessage1[DISPLAY_CHAR_WIDTH + 1];
// Buffer for dialog Page Message Line 2
extern char dialogPageMessage2[DISPLAY_CHAR_WIDTH + 1];

//----------------------------------------------------------------------------
// Persisted data to E^2 
//----------------------------------------------------------------------------

typedef struct {
   // First 4 bytes must be serialization version ID.  Big-ended order
   u32 serializationID;

   // Zone presets
   zone_preset_t zone_presets[NUM_ZONE_PRESETS];

} persisted_hmi_settings_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void HMI_Init(u8);
extern void HMI_1msTick();
extern void HMI_NotifyDownToggle(switch_state_t state);
extern void HMI_NotifyUpToggle(switch_state_t state);
extern void HMI_NotifyBackToggle(switch_state_t state);
extern void HMI_NotifyEnterToggle(switch_state_t state);

extern void HMI_DialogPage_UpdateDisplay();
extern void HMI_RenderLine(u8, const char*, renderline_justify_t);
extern void HMI_ClearLine(u8 lineNum);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TERMINAL_H */
