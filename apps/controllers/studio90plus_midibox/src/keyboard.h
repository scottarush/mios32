// $Id$
/*
 * Header file for Keyboard handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H


 /////////////////////////////////////////////////////////////////////////////
 // Global definitions
 /////////////////////////////////////////////////////////////////////////////


 // AIN components
#define KEYBOARD_AIN_NUM        4
#define KEYBOARD_AIN_PITCHWHEEL 0
#define KEYBOARD_AIN_MODWHEEL   1
#define KEYBOARD_AIN_SUSTAIN    2
#define KEYBOARD_AIN_EXPRESSION 3

#define MAX_SPLIT_ZONES 4

// for calibration feature: max keys per keyboard
#ifndef KEYBOARD_MAX_KEYS
#define KEYBOARD_MAX_KEYS 128
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum zone_preset_ids_e {
   SINGLE_ZONE = 0,
   DUAL_ZONE = 1,
   DUAL_ZONE_BASS = 2,
   TRIPLE_ZONE = 3,
   TRIPLE_ZONE_BASS = 4
} zone_preset_ids_t;

#define NUM_ZONE_PRESETS 5

typedef struct {
   u16 midiPorts;
   u8 midiChannel;
   s16 startNoteNum;
   s16 transposeOffset;  
} zone_params_t;

typedef struct {
   zone_preset_ids_t presetID;
   u8 numZones;
   zone_params_t zoneParams[MAX_SPLIT_ZONES];
 } zone_preset_t;

typedef struct {

   u8  note_offset;

   u8  num_rows;
   u8  selected_row;
   u8  prev_row;
   u8  verbose_level;

   u8  dout_sr1;
   u8  dout_sr2;
   u8  din_sr1;
   u8  din_sr2;
   u8  din_key_offset;

   u8  din_inverted : 1;
   u8  break_inverted : 1;
   u8  scan_velocity : 1;
   u8  scan_optimized : 1;
   u8  scan_release_velocity : 1;
   u8  make_debounced : 1;
   u8  key_calibration : 1;

   u16 delay_fastest;
   u16 delay_fastest_black_keys;
   u16 delay_fastest_release;
   u16 delay_fastest_release_black_keys;
   u16 delay_slowest;
   u16 delay_slowest_release;

   u16 delay_key[KEYBOARD_MAX_KEYS];

   u32 ain_timestamp[KEYBOARD_AIN_NUM];
   u8  ain_pin[KEYBOARD_AIN_NUM];
   u8  ain_ctrl[KEYBOARD_AIN_NUM];
   u8  ain_min[KEYBOARD_AIN_NUM];
   u8  ain_max[KEYBOARD_AIN_NUM];
   u8  ain_last_value7[KEYBOARD_AIN_NUM];
   u8  ain_inverted[KEYBOARD_AIN_NUM];
   u8  ain_sustain_switch;
   u8  ain_bandwidth_ms;

   zone_preset_t current_zone_preset;

} keyboard_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 KEYBOARD_Init(u32 mode);

extern s32 KEYBOARD_ConnectedNumSet(u8 num);
extern u8  KEYBOARD_ConnectedNumGet(void);

extern void KEYBOARD_SRIO_ServicePrepare(void);
extern void KEYBOARD_SRIO_ServiceFinish(void);
extern void KEYBOARD_Periodic_1mS(void);

extern void KEYBOARD_AIN_NotifyChange(u32 pin, u32 pin_value);

extern s32 KEYBOARD_TerminalHelp(void* _output_function);
extern s32 KEYBOARD_TerminalParseLine(char* input, void* _output_function);
extern s32 KEYBOARD_TerminalPrintConfig(void* _output_function);
extern s32 KEYBOARD_TerminalPrintDelays(void* _output_function);

extern void KEYBOARD_SetCurrentZonePreset(zone_preset_t * pPreset);
extern void KEYBOARD_SetKeyLearningCallback(void (*pCallback)(u8 noteNumber));

extern zone_preset_t * KEYBOARD_GetCurrentZonePreset();
extern void KEYBOARD_CopyZonePreset(zone_preset_t * pSource, zone_preset_t * pDest);


extern char* KEYBOARD_GetNoteName(u8 note, char str[4]);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern keyboard_config_t keyboard_config;


#endif /* _KEYBOARD_H */
