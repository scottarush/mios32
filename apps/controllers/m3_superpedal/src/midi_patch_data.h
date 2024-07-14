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

// Structure for disklavier patches
typedef struct disklavier_midi_patch_s {
   u8 groupID;
   u8 programNumber;
   u8 bankNumber;
   const char * voiceName[];
} disklavier_midi_patch_t;

//////////////////////////////////////////////////////////////////////
// Global exports
#define NUM_GEN_MIDI_BANK_NAMES 4
extern const char* defaultGenMIDIBankNames[NUM_GEN_MIDI_BANK_NAMES];

#define NUM_GEN_MIDI_VOICE_NAMES 128
extern const char* genMIDIVoiceNames[NUM_GEN_MIDI_VOICE_NAMES];

#define NUM_DISKLAVIER_PATCHES 438
extern const disklavier_midi_patch_t disklavierPatches[NUM_DISKLAVIER_PATCHES];

#endif /* _MIDI_PATCH_DATA_H */
