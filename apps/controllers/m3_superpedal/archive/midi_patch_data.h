/*
 * MIDI Patch data
 * Copyright Scott Rush 2024
 */

#ifndef _MIDI_PATCH_DATA_H
#define _MIDI_PATCH_DATA_H

#include <mios32.h>

// Bank number Control Change for JV880 A & B Presets.  See JV880 Manual page 10-32
#define JV880_AB_PRESET_BANK_SELECT_CC0 80
// Bank select Control Change number for JV880 Internal Presets.  See JV880 Manual page 10-32
#define JV880_INTERNAL_PRESET_BANK_SELECT_CC0 81

// Structure for disklavier patches.
typedef struct disklavier_voice_midi_number_s {
   u8 groupID;
   u8 programNumber;
   u8 bankNumber;
} disklavier_voice_midi_number_t;


//////////////////////////////////////////////////////////////////////
// Global exports
#define NUM_GEN_MIDI_BANK_NAMES 4
extern const char* defaultGenMIDIBankNames[NUM_GEN_MIDI_BANK_NAMES];

#define NUM_GEN_MIDI_VOICE_NAMES 128
extern const char* genMIDIVoiceNames[NUM_GEN_MIDI_VOICE_NAMES];

#define NUM_DISKLAVIER_VOICES 438
extern const disklavier_voice_midi_number_t disklavierVoiceMIDINumbers[NUM_DISKLAVIER_VOICES];
extern const char* disklavierVoiceNames[NUM_DISKLAVIER_VOICES];

#define NUM_DISKLAVIER_GROUPS 16
extern const char * disklavierGroupNames[NUM_DISKLAVIER_GROUPS];
extern const int disklavierGroupIndex[NUM_DISKLAVIER_GROUPS];

#endif /* _MIDI_PATCH_DATA_H */
