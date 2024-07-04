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
#include "pedals.h"
#include "persist.h"

#undef DEBUG

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////
u8 defaultGenMIDIPresetProgramNumbers[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK] = {
   // Bank 1 Strings
   {43,47,49,50,51,52},
   // Bank 2 Bass
   {33,34,35,37,39,40},
   // Bank 3 Keys
   {1,17,18,19,20,23},
   // Bank 4 Bells/Wind
   {10,15,63,65,71,74}
};
u8 defaultMIDIPresetOctaveNumbers[MAX_NUM_GEN_MIDI_PRESET_BANKS][MAX_NUM_PRESETS_PER_BANK] = {
   // Bank 1 Strings
   {3,3,3,3,3,3 },
   // Bank 2 Bass
   {3,3,3,3,3,3 },
   // Bank 3 Keys
   {3,3,3,3,3,3 },
   // Bank 4 Bells/Wind
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

const char* defaultGenMIDIBankNames[] = { "Strings","Bass","Keys","Bells/Wind" };

const char* genMIDIVoiceNames[] = { "Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano","Honky-tonk Piano","Electric Piano 1",
"Electric Piano 2","Harpsichord","Clavi","Celesta","Glockenspiel",
"Music Box","Vibraphone","Marimba","Xylophone","Tubular Bells",
"Dulcimer","Drawbar Organ","Percussive Organ","Rock Organ","Church Organ",
"Reed Organ","Accordion","Harmonica","Tango Accordion","Nylon Acoustic Guitar",
"Acoustic Guitar (steel)","Electric Guitar (jazz)","Clean Electric Guitar","Muted Electric Guitar","Overdriven Guitar",
"Distortion Guitar","Guitar harmonics","Acoustic Bass","Finger Electric Bass","Picked Electric Bass",
"Fretless Bass","Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
"Violin","Viola","Cello","Contrabass","Tremolo Strings",
"Pizzicato Strings","Orchestral Harp","Timpani","String Ensemble 1","String Ensemble 2",
"SynthStrings 1","SynthStrings 2","Choir Aahs","Voice Oohs","Synth Voice",
"Orchestra Hit","Trumpet","Trombone","Tuba","Muted Trumpet",
"French Horn","Brass Section","SynthBrass 1","SynthBrass 2",
"Soprano Sax","Alto Sax","Tenor Sax","Baritone Sax","Oboe","English Horn",
"Bassoon","Clarinet","Piccolo","Flute","Recorder",
"Pan Flute","Blown Bottle","Shakuhachi","Whistle","Ocarina",
"Lead 1 (square)","Lead 2 (sawtooth)","Lead 3 (calliope)","Lead 4 (chiff)","Lead 5 (charang)",
"Lead 6 (voice)","Lead 7 (fifths)","Lead 8 (bass + lead)","Pad 1 (new age)","Pad 2 (warm)",
"Pad 3 (polysynth)","Pad 4 (choir)","Pad 5 (bowed)","Pad 6 (metallic)","Pad 7 (halo)",
"Pad 8 (sweep)","FX 1 (rain)","FX 2 (soundtrack)","FX 3 (crystal)","FX 4 (atmosphere)",
"FX 5 (brightness)","FX 6 (goblins)","FX 7 (echoes)","FX 8 (sci-fi)","Sitar",
"Banjo","Shamisen","Koto","Kalimba","Bag pipe",
"Fiddle","Shanai","Tinkle Bell","Agogo","Steel Drums",
"Woodblock","Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
"Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet","Telephone Ring",
"Helicopter","Applause","Gunshot" };


// Bank number Control Change for JV880 A & B Presets.  See JV880 Manual page 10-32
#define JV880_AB_PRESET_BANK_SELECT_CC0 80
const char* jv880_ABPresets[] = { "Acoustic Piano 1","Acoustic Piano 2","Mellow Piano","Pop Piano 1","Pop Piano 2",
"Pop Piano 3","MIDIed Grand","Country Bar","Glist EPiano","MIDI EPiano",
"SA Rhodes","Dig Rhodes 1","Dig Rhodes 2","Stiky Rhodes","Guitr Rhodes",
"Nylon Rhodes","Clav 1","Clav 2","Marimba","Marimba SW",
"Warm Vibe","Vibe","Wave Bells","Vibrobell","Pipe Organ 1",
"Pipe Organ 2","Pipe Organ 3","Electric Organ 1","Electric Organ 2","Jazz Organ 1",
"Jazz Organ 2","Metal Organ","Nylon Gtr 1","Flanged Nyln","Steel Guitar",
"PickedGuitar","12 strings","Velo Harmnix","Nylon+Steel","SwitchOnMute",
"JC Strat","Stratus","Syn Strat","Pop Strat","Clean Strat",
"Funk Gtr","Syn Guitar","Overdrive","Fretless","St Fretless",
"Woody Bass 1","Woody Bass 2","Analog Bs 1","House Bass","Hip Bass",
"RockOut Bass","Slap Bass","Thumpin Bass","Pick Bass","Wonder Bass",
"Yowza Bass","Rubber Bass 1","Rubber Bass 2","Stereoww Bs","Pizzicato",
"Real Pizz","Harp","SoarinString","Warm Strings","Marcato",
"St Strings","Orch Strings","Slow Strings","Velo Strings","BrightStrngs",
"TremoloStrng","Orch Stab 1","Brite Stab","JP-8 Strings","String Synth",
"Wire Strings","New Age Vox","Arasian Morn","Beauty Vox","Vento Voxx",
"Pvox Oooze","GlassVoices","Space Ahh","Trumpet","Trombone",
"Harmon Mute1","Harmon Mute2","TeaJay Brass","Brass Sect 1","Brass Sect 2",
"Brass Swell","Brass Combo","Stab Brass","Soft Brass","Horn Brass",
"French Horn","AltoLead Sax","Alto Sax","Tenor Sax 1","Tenor Sax 2",
"Sax Section","Sax Tp Tb","FlutePiccolo","Flute mod","Ocarina",
"OverblownPan","Air Lead","Steel Drum","Log Drum","Box Lead",
"Soft Lead","Whistle","Square Lead","Touch Lead","NightShade",
"Pizza Hutt","EP+Exp Pad","JP-8 Pad","Puff","SpaciosSweep",
"Big n Beefy","RevCymBend","Analog Seq" };


// Bank select Control Change number for JV880 Internal Presets.  See JV880 Manual page 10-32
#define JV880_INTERNAL_PRESET_BANK_SELECT_CC0 81
const char* jv880_InternalPresets[] = { "770 Grand 1","MIDI 3 Grand","Electric Grand 1","60s Electric Piano","Dyna Rhodes",
"Pop Rhodes","Beauty Rhodes","Airies Piano","Clav 1 x4","Wah Clav",
"Housey Clavy","Ballad Org.1","Even Bars 1","Stereo Organ","Jazz Organ 3",
"8ft. Stop","Brite Organ 1","Soft Organ","60s Organ x4","Pipe Organ 4",
"Church","Celeste","Toy Piano","Snow Bells","Acoustic Bass 1",
"Acoustic Bass 2","Acoustic Fretless","Fretless 2","Weather Bass","Jazz Bass",
"Power Bass","Power Funk V-Sw","Stick","Delicate Stik","Stick V-Sw",
"Bassic house","Noo Spitbass","Metal Bass","Sync Bass","Bs Slide",
"Bs Harmonix","Super Nylon","Jazz Guitar","Jazz Cascade","Jazzy Scat",
"Pedal Steel","Banjo 1","Electric Sitar","Funk Guitar","Heavy Duty",
"Lead Guitar 1","Lead Guitar 2","Lead Guitar 3","Pocket Rocket","Power Flange",
"Bowed Guitar","Shakupeace","Cimbalon 1","Sanza 1","Shamisentur",
"Praying Monk","Electric Koto","Ravi Sitar","Mystic Mount" };


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
         }
      }
      // Default to preset 1
      presets.lastActivatedGenMIDIPresetNumber.bankNumber = 1;
      presets.lastActivatedGenMIDIPresetNumber.bankIndex = 1;
      MIDI_PRESETS_PersistData();
   }

}

const char* MIDI_PRESETS_GetGenMIDIVoiceName(u8 progNum) {
   if ((progNum == 0) || (progNum > MIDI_PRESETS_GetNumGenMIDIVoices())) {
      DEBUG_MSG("MIDI_PRESETS_GetGenMIDIVoiceName:  Invalid progNum=%d", progNum);
      return NULL;
   }
   return genMIDIVoiceNames[progNum - 1];
}


u8 MIDI_PRESETS_GetNumGenMIDIVoices() {
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
extern const char * MIDI_PRESETS_GetGenMidiBankName(u8 bankNumber){
      if ((bankNumber == 0) || (bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_GetGenMidiBankName: Invalid bankNumber: %d", bankNumber);
      return NULL;
   }
   return defaultGenMIDIBankNames[bankNumber-1];
}


/////////////////////////////////////////////////////////////////////////////
// Activates a general MIDI Preset.
// presetNum:  pointer to presetNumber struct
// returns:  pointer to activated presetNumber on success, NULL on error.
/////////////////////////////////////////////////////////////////////////////
const midi_preset_num_t* MIDI_PRESETS_ActivateGenMIDIPreset(const midi_preset_num_t* presetNum) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->bankIndex == 0) || (presetNum->bankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankIndex: %d", presetNum->bankIndex);
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->bankIndex - 1];

   MIDI_PRESETS_ActivateMIDIVoice(ptr->programNumber, ptr->midiBankNumber, ptr->midiPorts, ptr->midiChannel);

   // Set the octave
   PEDALS_SetOctave(ptr->octave);

   // Save last activate preset number
   presets.lastActivatedGenMIDIPresetNumber.bankNumber = presetNum->bankNumber;
   presets.lastActivatedGenMIDIPresetNumber.bankIndex = presetNum->bankIndex;
   // And persist
   MIDI_PRESETS_PersistData();

   return presetNum; // for success
}
/////////////////////////////////////////////////////////////////////////////
// Direct voice activation without preset
// programNumber:  The General MIDI program number
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
// bankIndex:  index of preset from 1 to MAX_NUM_PRESETS_PER_BANK
// returns: pointer to updated preset on success, NULL on invalid preset data
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_SetGenMIDIPreset(const midi_preset_num_t* presetNum, const midi_preset_t* setPresetPtr) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber);
      return NULL;
   }
   if ((presetNum->bankIndex == 0) || (presetNum->bankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankIndex: %d", presetNum->bankIndex);
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->bankIndex - 1];
   // TODO:  Validate programNumber, midiBankNumber, etc.
   ptr->programNumber = setPresetPtr->programNumber;
   ptr->midiBankNumber = setPresetPtr->midiBankNumber;
   ptr->octave = setPresetPtr->octave;
   ptr->midiPorts = setPresetPtr->midiPorts;
   ptr->midiChannel = setPresetPtr->midiChannel;

   // Update presets to E2
   MIDI_PRESETS_PersistData();
   return ptr;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a General MIDI Preset.
// presetNum:  pointer to presetNumber struct
// returns: pointer to updated preset on success, NULL on invalid presetNumber
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_GetGenMidiPreset(const midi_preset_num_t* presetNum) {
   if ((presetNum->bankNumber == 0) || (presetNum->bankNumber > MAX_NUM_GEN_MIDI_PRESET_BANKS)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankNumber: %d", presetNum->bankNumber );
      return NULL;
   }
   if ((presetNum->bankIndex == 0) || (presetNum->bankIndex > MAX_NUM_PRESETS_PER_BANK)) {
      DEBUG_MSG("MIDI_PRESETS_ActivateGenMIDIPreset: Invalid bankIndex: %d", presetNum->bankIndex );
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNum->bankNumber - 1][presetNum->bankIndex - 1];
#ifdef DEBUG
   DEBUG_MSG("MIDI_PRESETS_GetMidiPreset: bank%d index%d progNumber=%d", presetNum->bankNumber, presetNum->bankIndex , ptr->programNumber);
#endif
   return ptr;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the number of the last activated MIDI preset
// returns: pointer to last activated preset.  Must be valid
/////////////////////////////////////////////////////////////////////////////
const midi_preset_num_t* MIDI_PRESETS_GetLastActivatedGenMIDIPreset() {
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

