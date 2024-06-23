/*
 * General MIDI presets for M3 Superpedal
 * Copyright Scott Rush 2024
 */

#ifndef _MIDI_PRESETS_H
#define _MIDI_PRESETS_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Both of these defines are used in PERSIST.C to calculate the current 
// size and make sure it is under the max
#define NUM_GEN_MIDI_PRESETS 8
#define NUM_PATTERN_PRESETS 8

// OUT1 and USB
#define DEFAULT_PRESET_MIDI_PORTS 0x0031

#define DEFAULT_PRESET_MIDI_CHANNEL Chn1

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   GENERAL_MIDI_PRESET = 0,
   MIDI_PRESET = 1,
   PATTERN_PRESET = 2
} midi_preset_bank_t;

typedef struct {   
   // Preset #s start at 1
   u8 presetNumber;
   // Per Gen MIDI 1 spec programNumbers 1-128
   u8 programNumber;
   // bankNumber is device specific.  For GenMIDI, set to 0.
   u8 bankNumber;
   // midiPorts uses standard MIOS32 format
   u8 midiPorts;
   // midiChannel number common for all ports
   u8 midiChannel;
   // octave for pedals: 0 to 7
   u8 octave;
   // volume scaling fraction from 
} midi_preset_t;


typedef struct {
   // First 4 bytes must be serialization version ID. Little-endian order
   u32 serializationID;

   midi_preset_t generalMidiPresets[NUM_GEN_MIDI_PRESETS];
   u8 lastActivatedVoicePreset;
   //  midi_preset_t patternPresets[NUM_PATTERN_PRESETS];   
} persisted_midi_presets_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void MIDI_PRESETS_Init();

extern const char * MIDI_PRESETS_GetGenMIDIVoiceName(u8 progNum);
extern u8 MIDI_PRESETS_GetNumGenMIDIVoices();

extern const midi_preset_t* MIDI_PRESETS_SetMIDIPreset(u8 presetNumber, u8 programNumber, u8 bankNumber, u8 octave,u8 midiPorts, u8 midiChannel);

extern u8 MIDI_PRESETS_ActivateMIDIPreset(u8 presetNumber);
extern u8 MIDI_PRESETS_ActivateMIDIVoice(u8 programNumber, u8 bankNumber, u8 midiPorts, u8 midiChannel);

extern const midi_preset_t * MIDI_PRESETS_GetMidiPreset(u8 presetNumber);
extern const midi_preset_t* MIDI_PRESETS_GetLastActivatedPreset();

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _PRESETS_H */
