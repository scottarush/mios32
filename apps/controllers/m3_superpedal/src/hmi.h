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

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
// Following type is used as an array index so must be 0 based and consecutive
typedef enum {
   STOMP_SWITCH_OCTAVE = 0,
   STOMP_SWITCH_VOLUME = 1,
   STOMP_SWITCH_VOICE_PRESETS = 2,
   STOMP_SWITCH_PATTERN_PRESETS = 3,
   STOMP_SWITCH_ARPEGGIATOR = 4
} stomp_switch_setting_t;

typedef enum {
   TOE_SWITCH_OCTAVE = 0,
   TOE_SWITCH_VOLUME = 1,
   TOE_SWITCH_VOICE_PRESETS = 2,
   TOE_SWITCH_PATTERN_PRESETS = 3,
   TOE_SWITCH_ARPEGGIATOR = 4
} toeSwitchMode_t;
#define NUM_TOE_SWITCH_MODES 5

typedef struct {
   // First 4 bytes must be serialization version ID.  Big-ended order
   u32 serializationID;

   // The current mode of the toe switches
   toeSwitchMode_t toeSwitchMode;

   // Last selected toe switch in each mode
   u8 selectedToeIndicator[NUM_TOE_SWITCH_MODES];

   // presetNumbers for toe switch gen MIDI presets. 0 is unset
   u8 toeSwitchVoicePresetNumbers[NUM_TOE_SWITCHES];

   // Last selected MIDI preset program number in the assignment dialog
   u8 lastSelectedMIDIProgNumber;

   // Midi output to be used for preset editing
   u8 lastSelectedMidiOutput;

   // Midi channel to be used for preset editing
   u8 lastSelectedMidiChannel;

   // stomp switch settings
   stomp_switch_setting_t stompSwitchSetting[NUM_STOMP_SWITCHES];

} persisted_hmi_settings_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void HMI_Init(void);
extern void HMI_NotifyStompToggle(u8 stompNum,u8 pressed,s32 timestamp);
extern void HMI_NotifyToeToggle(u8 stompNum,u8 pressed,s32 timestamp);
extern void HMI_NotifyBackToggle(u8 pressed,s32 timestamp);
extern void HMI_NotifyEncoderChange(s32 incrementer);
extern void HMI_NotifyEncoderSwitchToggle(u8 pressed,s32 timestamp);
extern void HMI_NotifyOctaveChange(u8);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TERMINAL_H */
