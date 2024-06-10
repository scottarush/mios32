/*
 * Header file for pedal handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */
#ifndef _PEDALS_H
#define _PEDALS_H

typedef struct {
   u16 midi_ports;
   u8 midi_chn;

   u8 num_pedals;

   u8 note_offset;
   u8 verbose_level;

   u8 velocity_enabled;
   u16 delay_fastest;
   u16 delay_fastest_black_pedals;
   u16 delay_slowest;

   u8 left_pedal_note_number;

   // octave = 0, transpose = 0 means leftmost pedal is MIDI(left_pedal_note_number)
   u8 octave;

   // transpose 1/2 steps. tranpose = 0 is key of MIDI(left_pedal_note_number)
   u8 transpose;
} pedal_config_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern void PEDALS_Init();
extern void PEDALS_NotifyChange(u8 pedalNum, u8 value, u32 timestamp);
extern void PEDALS_NotifyMakeChange(u8 pressed, u32 timestamp);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
extern pedal_config_t pedal_config;

#endif // _PEDALS_H
