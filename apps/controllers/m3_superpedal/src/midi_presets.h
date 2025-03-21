/*
 * General MIDI presets for M3 Superpedal
 * Copyright Scott Rush 2024
 */

#ifndef _MIDI_PRESETS_H
#define _MIDI_PRESETS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Defines for maximum preset banks and presets / bank.  Used to set the
// maximum size of the persisted_midi_presets_t structure
#define MAX_NUM_GEN_MIDI_PRESET_BANKS 4
#define MAX_NUM_PATTERN_PRESET_BANKS 1
#define MAX_NUM_PRESETS_PER_BANK 6
#define MAX_BANK_NAME_SIZE 10


#define MAX_NUM_GEN_MIDI_PRESET_NUMBER (MAX_NUM_GEN_MIDI_PRESET_BANKS * MAX_NUM_PRESETS_PER_BANK)

// OUT1 and USB
#define DEFAULT_PRESET_MIDI_PORTS 0x0031

#define DEFAULT_PRESET_MIDI_CHANNEL Chn1

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   PRESET_GENERAL_MIDI = 0,
   PRESET_DISKLAVIER_XG = 1,
   PRESET_JV880 = 2
} midi_preset_type_t;

typedef struct midi_preset_num_s {
   // from 1..Number of Banks
   u8 bankNumber;
   // from 1..Number of Presets per Bank
   u8 presetBankIndex;
} midi_preset_num_t;

typedef struct {   
   // Per Gen MIDI 1 spec programNumbers 0..127
   u8 programNumber;
   // bankNumber is device specific.  For GenMIDI, set to 0.
   u8 midiBankNumber;
   // midiPorts uses standard MIOS32 format
   u8 midiPorts;
   // midiChannel number common for all ports
   u8 midiChannel;
   // octave for pedals: 0 to 7
   u8 octave;
   // volume expressed as midi velocity.  0 is default and follows current system volume
   u8 volume;
} midi_preset_t;


typedef struct persisted_midi_presets_s {
   // First 4 bytes must be serialization version ID. Little-endian order
   u32 serializationID;

   char generalMIDIBankNames[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_BANK_NAME_SIZE];

   midi_preset_t generalMidiPresets[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK];
   
   midi_preset_num_t lastActivatedGenMIDIPresetNumber;

   midi_preset_t pattern[MAX_NUM_PATTERN_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK];
   u8 lastActivatedPatternPresetNum;

} persisted_midi_presets_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void MIDI_PRESETS_Init(u8);

extern const char * MIDI_PRESETS_GetMIDIVoiceName(u8 progNum);
extern u8 MIDI_PRESETS_GetNumMIDIVoices();

extern const midi_preset_t * MIDI_PRESETS_SetMIDIPreset(const midi_preset_num_t * presetNum,const midi_preset_t* setPresetPtr);
extern midi_preset_t * MIDI_PRESETS_CopyPreset(const midi_preset_num_t* presetNum, midi_preset_t * ptr);

extern const midi_preset_num_t * MIDI_PRESETS_ActivateMIDIPreset(const midi_preset_num_t * presetNum);
extern u8 MIDI_PRESETS_ActivateMIDIVoice(u8 programNumber, u8 bankNumber, u8 midiPorts, u8 midiChannel);

extern const midi_preset_t* MIDI_PRESETS_GetMidiPreset(const midi_preset_num_t * presetNum);
extern const midi_preset_num_t* MIDI_PRESETS_GetLastActivatedMIDIPreset();

extern u8 MIDI_PRESETS_GetGenMidiPresetNumBanks();
extern u8 MIDI_PRESETS_GetGenMidiPresetBankSize();
extern const char * MIDI_PRESETS_GetGenMidiBankName(u8 bankNum);
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _PRESETS_H */
