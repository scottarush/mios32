/*
 * General MIDI presets for M3 Superpedal
 * Copyright Scott Rush 2024
 */
 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "midi_presets.h"

#undef DEBUG_ENABLED

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

#define NUM_GEN_MIDI_PRESETS 8
#define NUM_PATTERN_PRESETS 8

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////
const u8 defaultMIDIPresetProgramNumbers[] = {43,49,50,51,17,20,33,92};
const u8 defaultMIDIPresetBankNumbers[] = {0,0,0,0,0,0,0,0};
#define DEFAULT_PRESET_MIDI_OUTPUT 1
#define DEFAULT_PRESET_MIDI_CHANNEL 1

/////////////////////////////////////////////////////////////////////////////
// Persisted data to E^2
/////////////////////////////////////////////////////////////////////////////
typedef struct persistedData_s {
   midi_preset_t generalMidiPresets[NUM_GEN_MIDI_PRESETS];
   //  midi_preset_t patternPresets[NUM_PATTERN_PRESETS];   
} persistedData_t;

persistedData_t presets;

char* genMIDIVoiceNames[] = { " Acoustic Grand Piano"," Bright Acoustic Piano"," Electric Grand Piano"," Honky-tonk Piano"," Electric Piano 1",
" Electric Piano 2"," Harpsichord"," Clavi"," Celesta"," Glockenspiel",
" Music Box"," Vibraphone"," Marimba"," Xylophone"," Tubular Bells",
" Dulcimer"," Drawbar Organ"," Percussive Organ"," Rock Organ"," Church Organ",
" Reed Organ"," Accordion"," Harmonica"," Tango Accordion"," Acoustic Guitar (nylon",
" Acoustic Guitar (steel)"," Electric Guitar (jazz)"," Electric Guitar (clean)"," Electric Guitar (muted"," Overdriven Guitar",
" Distortion Guitar"," Guitar harmonics"," Acoustic Bass"," Electric Bass (finger)"," Electric Bass (pick)",
" Fretless Bass"," Slap Bass 1"," Slap Bass 2"," Synth Bass 1"," Synth Bass 2",
" Violin"," Viola"," Cello"," Contrabass"," Tremolo Strings",
" Pizzicato Strings"," Orchestral Harp"," Timpani"," String Ensemble 1"," String Ensemble 2",
" SynthStrings 1"," SynthStrings 2"," Choir Aahs"," Voice Oohs"," Synth Voice",
" Orchestra Hit"," Trumpet"," Trombone"," Tuba"," Muted Trumpet",
" French Horn"," Brass Section"," SynthBrass 1"," SynthBrass 2"," Soprano Sax",
" Alto Sax"," Tenor Sax"," Baritone Sax"," Oboe"," English Horn",
" Bassoon"," Clarinet"," Piccolo"," Flute"," Recorder",
" Pan Flute"," Blown Bottle"," Shakuhachi"," Whistle"," Ocarina",
" Lead 1 (square)"," Lead 2 (sawtooth)"," Lead 3 (calliope)"," Lead 4 (chiff)"," Lead 5 (charang)",
" Lead 6 (voice)"," Lead 7 (fifths)"," Lead 8 (bass + lead)"," Pad 1 (new age)"," Pad 2 (warm)",
" Pad 3 (polysynth)"," Pad 4 (choir)"," Pad 5 (bowed)"," Pad 6 (metallic)"," Pad 7 (halo)",
" Pad 8 (sweep)"," FX 1 (rain)"," FX 2 (soundtrack)"," FX 3 (crystal)"," FX 4 (atmosphere)",
" FX 5 (brightness)"," FX 6 (goblins)"," FX 7 (echoes)"," FX 8 (sci-fi)"," Sitar",
" Banjo"," Shamisen"," Koto"," Kalimba"," Bag pipe",
" Fiddle"," Shanai"," Tinkle Bell"," Agogo"," Steel Drums",
" Woodblock"," Taiko Drum"," Melodic Tom"," Synth Drum"," Reverse Cymbal",
" Guitar Fret Noise"," Breath Noise"," Seashore"," Bird Tweet"," Telephone Ring",
" Helicopter"," Applause"," Gunshot" };

void MIDI_PRESETS_Init() {
   // Restore settings from E^2 if they exist.  If not the initialize to defaults
  // TODO
   u8 valid = 0;
   // valid = PRESETS_GetHMISettings(&settings);
   if (!valid) {
      // Set default presets
      for (int i = 0;i < NUM_GEN_MIDI_PRESETS;i++) {
         midi_preset_t* ptr = &presets.generalMidiPresets[i];
         ptr->presetNumber = i;
         ptr->programNumber = defaultMIDIPresetProgramNumbers[i];
         ptr->programNumber = defaultMIDIPresetBankNumbers[i];
         ptr->midiOutput = DEFAULT_PRESET_MIDI_OUTPUT;
         ptr->midiChannel = DEFAULT_PRESET_MIDI_CHANNEL;
      }

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

u8 MIDI_PRESET_ActivateMIDIPreset(u8 presetNumber) {
   for (int i = 0;i < NUM_GEN_MIDI_PRESETS;i++) {
      midi_preset_t* gPtr = &presets.generalMidiPresets[i];
      if (gPtr->presetNumber == presetNumber) {
         // Activate this preset on the requested midi output and channel
         // TODO

         return gPtr->presetNumber;  // for success
      }
   }
   return 0; // error
}

/////////////////////////////////////////////////////////////////////////////
// Sets a MIDI Preset.
// presetNumber:  0 to NUM_GEN_MIDI_PRESETS-1
// programNumber:  The General MIDI program number
// bankNumber: General MIDI bank number
// midiOutput: MIDI hardware output
// midiChannel:  MIDI channel
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_SetMIDIPreset(u8 presetNumber, u8 programNumber, u8 bankNumber, u8 midiOutput, u8 midiChannel) {
   if (presetNumber >= NUM_GEN_MIDI_PRESETS) {
      DEBUG_MSG("MIDI_PRESETS_SetMIDIPreset: Invalid presetNumber: %d", presetNumber);
      return NULL;
   }
   midi_preset_t* gPtr = &presets.generalMidiPresets[presetNumber];
   gPtr->programNumber = programNumber;
   gPtr->bankNumber = bankNumber;
   gPtr->midiOutput = midiOutput;
   gPtr->midiChannel = midiChannel;
   return gPtr;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the MIDI Preset.
// presetNumber:  0 to NUM_GEN_MIDI_PRESETS-1
/////////////////////////////////////////////////////////////////////////////
const midi_preset_t* MIDI_PRESETS_GetMidiPreset(u8 presetNumber) {
   if (presetNumber >= NUM_GEN_MIDI_PRESETS) {
      DEBUG_MSG("MIDI_PRESETS_GetMidiPreset: Invalid presetNumber: %d", presetNumber);
      return NULL;
   }
   midi_preset_t* ptr = &presets.generalMidiPresets[presetNumber];
   DEBUG_MSG("MIDI_PRESETS_GetMidiPreset: preset#:%d, progNumber=%d",ptr->presetNumber,ptr->programNumber);

   return ptr;
}

