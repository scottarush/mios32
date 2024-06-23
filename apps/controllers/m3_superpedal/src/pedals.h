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

/////////////////////////////////////////////////////////////////////////////
// Global define data.
/////////////////////////////////////////////////////////////////////////////
#define PEDALS_DEFAULT_OCTAVE_NUMBER 3
#define PEDALS_MAX_VOLUME 127
/////////////////////////////////////////////////////////////////////////////
// Persisted data.
/////////////////////////////////////////////////////////////////////////////
typedef struct {
   // First 4 bytes must be serialization version ID.  Big-ended order
   u32 serializationID;

   u16 midi_ports;
   u8 midi_chn;

   u8 num_pedals;

   u8 note_offset;
   u8 verbose_level;

   u16 delay_fastest;
   u16 delay_fastest_black_pedals;
   u16 delay_slowest;
   u16 delay_slowest_black_pedals;
   u16 delay_release_fastest;
   u16 delay_release_slowest;

   u16 minimum_press_velocity;
   u16 minimum_release_velocity;

   u8 left_pedal_note_number;

   // octave = 0, transpose = 0 means leftmost pedal is MIDI(left_pedal_note_number)
   u8 octave;

   // transpose 1/2 steps. tranpose = 0 is key of MIDI(left_pedal_note_number)
   u8 transpose;

   // volume from 1 to PEDALS_MAX_VOLUME
   u8 volumeLevel;
} persisted_pedal_confg_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern void PEDALS_Init();
extern void PEDALS_NotifyChange(u8 pedalNum, u8 value, u32 timestamp);
extern void PEDALS_NotifyMakeChange(u8 pressed, u32 timestamp);
extern void PEDALS_SetOctave(u8 octave);
extern u8 PEDALS_GetOctave();
extern u8 PEDALS_GetVolume();
extern void PEDALS_SetVolume(u8 volumeLevel);

extern float PEDALS_ComputeVolumeScaling(u8 volumeLevel);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
extern persisted_pedal_confg_t pedal_config;

#endif // _PEDALS_H
