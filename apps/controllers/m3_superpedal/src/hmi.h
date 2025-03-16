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

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define NUM_STOMP_SWITCHES 5
#define NUM_TOE_SWITCHES 8
#define NUM_TOE_PRESETS_PER_BANK 6
#define DISPLAY_CHAR_WIDTH 20

#define NUM_TOE_GEN_MIDI_PRESET_BANKS 4

#define DEBOUNCE_TIME_MS 40

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum pageID_e {
   PAGE_HOME = 0,
   PAGE_ARP_SETTINGS = 1,
   PAGE_ARP_PATTERN_SELECT = 2,
   PAGE_DIALOG = 3,
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


// Following type defines stomp switch functions from left=1 to right=5.
// These are also indexes and must be consecutive.
typedef enum {
   STOMP_SWITCH_VOLUME = 5,
   STOMP_SWITCH_CHORD = 2,
   STOMP_SWITCH_MIDI_CHANNEL = 3,
   STOMP_SWITCH_ARPEGGIATOR = 4,
   STOMP_SWITCH_OCTAVE = 1,
} stomp_switch_setting_t;

typedef enum toeSwitchMode_e {
   TOE_SWITCH_OCTAVE = 0,
   TOE_SWITCH_VOLUME = 1,
   TOE_SWITCH_CHORD = 2,
   TOE_SWITCH_ARP = 3,
   TOE_SWITCH_MIDI_CHANNEL = 4
} toeSwitchMode_t;
#define NUM_TOE_SWITCH_MODES 5

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
extern struct page_s arpSettingsPage;
extern struct page_s arpPatternPage;

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

   // The current mode of the toe switches
   toeSwitchMode_t toeSwitchMode;

} persisted_hmi_settings_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void HMI_Init(u8);
extern void HMI_NotifyStompToggle(u8 stompNum,u8 pressed,s32 timestamp);
extern void HMI_NotifyToeToggle(u8 stompNum,u8 pressed,s32 timestamp);
extern void HMI_NotifyBackToggle(u8 pressed,s32 timestamp);
extern void HMI_NotifyEncoderChange(s32 incrementer);
extern void HMI_NotifyEncoderSwitchToggle(u8 pressed,s32 timestamp);
extern void HMI_NotifyOctaveChange(u8 octave);
extern void HMI_DialogPage_UpdateDisplay();
extern void HMI_UpdateIndicators();
extern void HMI_RenderLine(u8, const char*, renderline_justify_t);
extern void HMI_ClearLine(u8 lineNum);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TERMINAL_H */
