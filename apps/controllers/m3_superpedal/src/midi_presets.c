/*
 * General MIDI presets for M3 Superpedal
 * Copyright Scott Rush 2024
 */
 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <mios32_midi.h>

#include "midi_presets.h"
#include "midi_patch_data.h"
#include "pedals.h"
#include "persist.h"

#define DEBUG

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////
u8 defaultGenMIDIPresetProgramNumbers[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK] = {
   // Bank 1 Strings
   {48,49,50,51,52,54},
   // Bank 2 Bass
   {32,33,35,36,38,39},
   // Bank 3 Keys
   {0,11,14,16,18,19},
   // Bank 4 Wind/Pads
   {61,62,63,99,100,101}
};
u8 defaultMIDIPresetOctaveNumbers[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK] = {
   // Bank 1 Strings
   {3,3,3,3,3,3 },
   // Bank 2 Bass
   {3,3,3,3,3,3 },
   // Bank 3 Keys
   {3,3,3,3,3,3 },
   // Bank 4 Wind/Pads
   {3,3,3,3,3,3 }
};

/////////////////////////////////////////////////////////////////////////////
// Persisted data to E^2
/////////////////////////////////////////////////////////////////////////////
persisted_midi_presets_t presets;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
void MIDI_PRESETS_PersistData();

void MIDI_PRESETS_Init() {
   // Restore settings from E^2 if they exist.  If not the initialize to defaults
   s32 valid = 0;
   // Set the expected serializedID in the supplied block.  Update this ID whenever the persisted structure changes.  
   presets.serializationID = 0x4D494401;   // 'MID1'

   valid = PERSIST_ReadBlock(PERSIST_MIDI_PRESETS_BLOCK, (unsigned char*)&presets, sizeof(presets));
   if (valid < 0) {
      DEBUG_MSG("MIDI_PRESETS_Init:  PERSIST_ReadBlock return invalid.   Reinitializing EEPROM Block");
      // Set default presets
      for (int bank = 0;bank < MAX_NUM_GEN_MIDI_PRESET_BANKS;bank++) {
         for (int i = 0;i < MAX_NUM_PRESETS_PER_BANK;i++) {

            midi_preset_t* ptr = &presets.generalMidiPresets[bank][i];
            ptr->programNumber = defaultGenMIDIPresetProgramNumbers[bank][i];
            ptr->midiBankNumber = 0;
            ptr->midiPorts = DEFAULT_PRESET_MIDI_PORTS;
            ptr->midiChannel = DEFAULT_PRESET_MIDI_CHANNEL;
            ptr->octave = defaultMIDIPresetOctaveNumbers[bank][i];
            ptr->volume = 0;
         }
      }
      // Default to preset 1
      presets.lastActivatedGenMIDIPresetNumber.bankNumber = 1;
      presets.lastActivatedGenMIDIPresetNumber.presetBankIndex = 1;
      MIDI_PRESETS_PersistData();
   }

}

////////////////////////////////////////////////////////////////////////////////////
// Returns MIDI Voice Name for the supplied program and bank number for the
// currently selected MIDI Synth Device.
////////////////////////////////////////////////////////////////////////////////////
const char* MIDI_PRESETS_GetMIDIVoiceName(u8 progNum) {
   if (progNum >= MIDI_PRESETS_GetNumMIDIVoices()) {
      DEBUG_MSG("MIDI_PRESETS_GetGenMIDIVoiceName:  Invalid progNum=%d", progNum);
      return NULL;
   }
   return genMIDIVoiceNames[progNum];
}


u8 MIDI_PRESETS_GetNumMIDIVoices() {
   size_t length = sizeof(genMIDIVoiceNames) / sizeof(char*);
   return length;
}

u8 MIDI_PRESETS_GetGenMidiPresetNumBanks() {
   return MAX_NUM_GEN_MIDI_PRESET_BANKS;
}

extern u8 MIDI_PRESETS_GetGenMidiPresetBankSize() {
   return MAX_NUM_PRESETS_PER_BANK;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the name of the bank
// bankNum:  number of the bank from 1 to MAX_NUM_GEN_MIDI_PRESET_BANKS
/////////////////////////////////////////////////////////////////////////////
extern const char* MIDI_PRESETS_GetGenMidiBankName(u8 bankNumber) {
   if ((bankNumber == 0) || (bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_GetGenMidiBankName: Invalid bankNumber: %d", bankNumber);
      return NULL;
   }
   return defaultGenMIDIBankNames[bankNumber - 1];
}


/////////////////////////////////////////////////////////////////////////////
// Activates a general MIDI Preset including setting the PEDALS octave 
// and volume (velocity).
// presetNum:  pointer to presetNumber struct
// returns:  pointer to activated presetNumber on success, NULL on error.
/////////////////////////////////////////////////////////////////////////////
const midi_preset_num_t* MIDI_PRESETS_ActivateMIDIPreset(const midi_preset_num_t* presetNum) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->presetBankIndex == 0) || (presetNum->presetBankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid presetBankIndex: %d", presetNum->presetBankIndex);
      return NULL;
   }

   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->presetBankIndex - 1];
#ifdef DEBUG
   DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset:  Activating preset# %d.%d, progNumber=%d",
      presetNum->bankNumber,presetNum->presetBankIndex,ptr->programNumber);
#endif

   MIDI_PRESETS_ActivateMIDIVoice(ptr->programNumber, ptr->midiBankNumber, ptr->midiPorts, ptr->midiChannel);

   // Set the octave
   PEDALS_SetOctave(ptr->octave);

   // If the volume is non-zero then change the current PEDALS volume to the preset value
   if (ptr->volume > 0) {
      PEDALS_SetVolume(ptr->volume);
   }
   // Save last activate preset number
   presets.lastActivatedGenMIDIPresetNumber.bankNumber = presetNum->bankNumber;
   presets.lastActivatedGenMIDIPresetNumber.presetBankIndex = presetNum->presetBankIndex;
   // And persist
   MIDI_PRESETS_PersistData();

   return presetNum; // for success
}
/////////////////////////////////////////////////////////////////////////////
// Temporary direct voice activation without change of preset.
// programNumber:  The General MIDI program number from 0..127
// midiBankNumber: General MIDI bank number
// midiPorts: MIDI hardware outputs
// midiChannel:  MIDI channel
// returns: pointer to updated preset on success, NULL on invalid preset data
/////////////////////////////////////////////////////////////////////////////
u8 MIDI_PRESETS_ActivateMIDIVoice(u8 programNumber, u8 midiBankNumber, u8 midiPorts, u8 midiChannel) {
   u16 mask = 1;
   for (int porti = 0; porti < 16; ++porti, mask <<= 1) {
      if (midiPorts & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((porti & 0xc) << 2) + (porti & 3);

         //DEBUG_MSG("midi tx:  port=0x%x",port);
         MIOS32_MIDI_SendProgramChange(port, midiChannel, programNumber);
         // Send midiBankNumber if non-zero
         if (midiBankNumber != 0) {
            // TODO
            DEBUG_MSG("MIDI_PRESETS_ActivateMIDIVoice:  midi bank# != 0 (%d) but bank# Tx not yet implemented.", midiBankNumber);
         }
      }
   }
   return programNumber;
}

/////////////////////////////////////////////////////////////////////////////
// Sets all parameters for a General MIDI Preset.
// presetNum:  pointer to presetNumber struct
// presetPtr:  point to client-side filled preset object which will be copied
//             into persisted storage
// bankNumber:  bank number from 1 to MAX_NUM_GEN_MIDI_PRESET_BANKS
// presetBankIndex:  index of preset from 1 to MAX_NUM_PRESETS_PER_BANK
// returns: pointer to updated preset on success, NULL on invalid preset data
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_SetMIDIPreset(const midi_preset_num_t* presetNum, const midi_preset_t* setPresetPtr) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->presetBankIndex == 0) || (presetNum->presetBankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid presetBankIndex: %d", presetNum->presetBankIndex);
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->presetBankIndex - 1];
   #ifdef DEBUG
   DEBUG_MSG("MIDI_PRESETS_SetGenMIDIPreset: Setting preset# %d.%d, progNumber=%d",
      presetNum->bankNumber,presetNum->presetBankIndex,ptr->programNumber);
   #endif
   // TODO:  Validate programNumber, midiBankNumber, etc.
   ptr->programNumber = setPresetPtr->programNumber;
   ptr->midiBankNumber = setPresetPtr->midiBankNumber;
   ptr->octave = setPresetPtr->octave;
   ptr->midiPorts = setPresetPtr->midiPorts;
   ptr->midiChannel = setPresetPtr->midiChannel;
   ptr->volume = setPresetPtr->volume;

   // Update presets to E2
   MIDI_PRESETS_PersistData();
   return ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Utility copies preset parameters to a supplied pointer.  For use
// in updating preset params on an existing preset before calling SetGenMIDIPreset
// Returns supplied pointer with updated params, NULL if presetNum is invalid
/////////////////////////////////////////////////////////////////////////////
midi_preset_t* MIDI_PRESETS_CopyPreset(const midi_preset_num_t* presetNum, midi_preset_t* ptr) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_CopyPresetParams: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->presetBankIndex == 0) || (presetNum->presetBankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_CopyPresetParams: Invalid presetBankIndex: %d", presetNum->presetBankIndex);
      return NULL;
   }
   midi_preset_t* presetPtr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->presetBankIndex - 1];
   ptr->programNumber = presetPtr->programNumber;
   ptr->midiBankNumber = presetPtr->midiBankNumber;
   ptr->octave = presetPtr->octave;
   ptr->midiPorts = presetPtr->midiPorts;
   ptr->midiChannel = presetPtr->midiChannel;
   ptr->volume = presetPtr->volume;
   return ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a General MIDI Preset.
// presetNum:  pointer to presetNumber struct
// returns: pointer to updated preset on success, NULL on invalid presetNumber
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_GetMidiPreset(const midi_preset_num_t* presetNum) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->presetBankIndex == 0) || (presetNum->presetBankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid presetBankIndex: %d", presetNum->presetBankIndex);
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->presetBankIndex - 1];
#ifdef DEBUG
//   DEBUG_MSG("MIDI_PRESETS_GetMidiPreset: bank%d index%d progNumber=%d", presetNum->bankNumber, presetNum->presetBankIndex, ptr->programNumber);
#endif
   return ptr;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the number of the last activated MIDI preset
// returns: pointer to last activated preset.  Must be valid
/////////////////////////////////////////////////////////////////////////////
const midi_preset_num_t* MIDI_PRESETS_GetLastActivatedMIDIPreset() {
   return &presets.lastActivatedGenMIDIPresetNumber;
}

/////////////////////////////////////////////////////////////////////////////
// Helper to store persisted data to EEPROM
/////////////////////////////////////////////////////////////////////////////
void MIDI_PRESETS_PersistData() {
#ifdef DEBUG
   DEBUG_MSG("MIDI_PRESETS_PersistData: Writing persisted data:  sizeof(presets)=%d bytes", sizeof(presets));
#endif
   s32 valid = PERSIST_StoreBlock(PERSIST_MIDI_PRESETS_BLOCK, (unsigned char*)&presets, sizeof(presets));
   if (valid < 0) {
      DEBUG_MSG("MIDI_PRESETS_Init:  Error persisting setting to EEPROM");
   }
   return;
}

