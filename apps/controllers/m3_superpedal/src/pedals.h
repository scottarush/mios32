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

#include "velocity.h"
/////////////////////////////////////////////////////////////////////////////
// Global define data.
/////////////////////////////////////////////////////////////////////////////
#define PEDALS_DEFAULT_OCTAVE_NUMBER 3
#define PEDALS_MAX_OCTAVE_NUMBER 8
#define PEDALS_MIN_OCTAVE_NUMBER -2
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
   s8 octave;

   // transpose 1/2 steps. tranpose = 0 is key of MIDI(left_pedal_note_number)
   u8 transpose;

   // volume from 1 to PEDALS_MAX_VOLUME
   u8 volumeLevel;

   // velocity curve
   velocity_curve_t velocityCurve;
} persisted_pedal_confg_t;

// Callback function for receiving a selected pedal in pedalSelectMode
typedef void (*GetSelectedPedal)(u8 pedalNum);

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern void PEDALS_Init(u8);
extern void PEDALS_NotifyChange(u8 pedalNum, u8 value, u32 timestamp);
extern void PEDALS_NotifyMakeChange(u8 pressed, u32 timestamp);
extern void PEDALS_SetOctave(s8 octave);
extern s8 PEDALS_GetOctave();
extern u8 PEDALS_GetVolume();
extern void PEDALS_SetVolume(u8 volumeLevel);
extern u8 PEDALS_GetMIDIChannel();
extern void PEDALS_SetMIDIChannel(u8 channel);

extern mios32_midi_port_t PEDALS_GetMIDIPorts();

extern u8 PEDALS_ScaleVelocity(u8 velocity,u8 volumeLevel);

extern void PEDALS_SetScale(u8 enabled,u8 rootkey);
extern void PEDALS_SetSelectPedalCallback(GetSelectedPedal callback);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
extern persisted_pedal_confg_t pedal_config;

#endif // _PEDALS_H
