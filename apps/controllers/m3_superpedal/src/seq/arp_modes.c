/*
 * Arpeggiator modes
 *
 *  Copyright Scott Rush 2024.
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "arp_modes.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define DEBUG

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char* key_names[] = { "C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B" };

// Defines the natural chords in the mode by key.
typedef struct mode_chords_s {
   chord_type_t chords[12];
   u8 numChords;
   scale_t scale;
} mode_chords_t;
#define NUM_MODES 7

// Table of chords indexed by mode group
static const mode_chords_t mode_group_chord_table[NUM_MODE_GROUPS][NUM_MODES] = {
   // MODE_GROUP_BASE
   {{{CHORD_MAJOR_I, CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJOR_I,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MIN7B5}, 7, SCALE_IONIAN},
   {{CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJOR_I,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I}, 7, SCALE_DORIAN},
   {{CHORD_MINOR_I, CHORD_MAJOR_I,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I,  CHORD_MINOR_I, }, 7, SCALE_PHRYGIAN},
   {{CHORD_MAJOR_I,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MINOR_I}, 7, SCALE_LYDIAN},
   {{CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJOR_I}, 7, SCALE_MIXOLYDIAN},
   {{CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJOR_I,  CHORD_MAJOR_I},    7, SCALE_AEOLIAN},
   {{CHORD_MIN7B5,  CHORD_MAJOR_I,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJOR_I,  CHORD_MAJOR_I,  CHORD_MINOR_I}, 7, SCALE_LOCRIAN}},
   // MODE_GROUP_SEVENTH
   {{{CHORD_MAJ7, CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ7,  CHORD_DOM7,  CHORD_MINOR_I,  CHORD_MIN7B5},  7, SCALE_IONIAN},
   {{CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ7,  CHORD_DOM7,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ7}, 7, SCALE_DORIAN},
   {{CHORD_MINOR_I, CHORD_MAJ7,  CHORD_MAJ7,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ7,  CHORD_MINOR_I, }, 7, SCALE_PHRYGIAN},
   {{CHORD_MAJ7,  CHORD_DOM7,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ7,  CHORD_MINOR_I,  CHORD_MINOR_I}, 7, SCALE_LYDIAN},
   {{CHORD_DOM7,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ7,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ7}, 7, SCALE_MIXOLYDIAN},
   {{CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ7,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ7,  CHORD_DOM7}, 7, SCALE_AEOLIAN},
   {{CHORD_MIN7B5,  CHORD_MAJ7,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ7,  CHORD_DOM7,  CHORD_MINOR_I}, 7, SCALE_LOCRIAN}},
   // MODE_GROUP_NINTH
   {{{CHORD_MAJ9, CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ9,  CHORD_DOM9,  CHORD_MINOR_I,  CHORD_MIN7B5},  7, SCALE_IONIAN},
   {{CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ9,  CHORD_DOM9,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ9}, 7, SCALE_DORIAN},
   {{CHORD_MINOR_I, CHORD_MAJ9,  CHORD_MAJ9,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ9,  CHORD_MINOR_I, }, 7, SCALE_PHRYGIAN},
   {{CHORD_MAJ9,  CHORD_DOM9,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ9,  CHORD_MINOR_I,  CHORD_MINOR_I}, 7, SCALE_LYDIAN},
   {{CHORD_DOM9,  CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ9,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ9}, 7, SCALE_MIXOLYDIAN},
   {{CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJ9,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ9,  CHORD_DOM9}, 7, SCALE_AEOLIAN},
   {{CHORD_MIN7B5,  CHORD_MAJ9,  CHORD_MINOR_I,  CHORD_MINOR_I, CHORD_MAJ9,  CHORD_DOM9,  CHORD_MINOR_I}, 7, SCALE_LOCRIAN}},
   // MODE_GROUP_FIFTH
   {{{CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_MIN7B5},  7, SCALE_IONIAN},
   {{CHORD_FIFTH,  CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_MIN7B5,  CHORD_FIFTH}, 7, SCALE_DORIAN},
   {{CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_MIN7B5,  CHORD_FIFTH,  CHORD_FIFTH, }, 7, SCALE_PHRYGIAN},
   {{CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_MIN7B5,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH}, 7, SCALE_LYDIAN},
   {{CHORD_FIFTH,  CHORD_FIFTH,  CHORD_MIN7B5,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH, CHORD_FIFTH}, 7, SCALE_MIXOLYDIAN},
   {{CHORD_FIFTH,  CHORD_MIN7B5,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH}, 7, SCALE_AEOLIAN},
   {{CHORD_MIN7B5,  CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH, CHORD_FIFTH,  CHORD_FIFTH,  CHORD_FIFTH}, 7, SCALE_LOCRIAN}}
};


typedef struct mode_group_preset_s {
   mode_groups_t group;
   const char* presetName;
} mode_group_preset_t;

const mode_group_preset_t mode_group_presets[NUM_MODE_GROUPS] =
{ {MODE_GROUP_BASE,"Base"},{MODE_GROUP_SEVENTH,"Seventh"},{MODE_GROUP_NINTH,"Ninth"},{MODE_GROUP_FIFTH,"Fifths"} };

/////////////////////////////////////////////////////////////////////////////
// Returns the next name of the mode group
/////////////////////////////////////////////////////////////////////////////
const char* ARP_MODES_GetModeGroupName(enum mode_groups_e group) {

   if ((group < 0) > (group >= NUM_MODE_GROUPS)) {
      return "Invalid Mode Group";
   }
   return (const char*)mode_group_presets[group].presetName;
}

/////////////////////////////////////////////////////////////////////////////
// Returns chord type for mode and note
// scale:  selected scale.
// extFlags:  extension flags
// keySig:  key signature root
// note:  root note of the chord 
/////////////////////////////////////////////////////////////////////////////
const chord_type_t ARP_MODES_GetModeChord(scale_t scale, mode_groups_t modeGroup, u8 keySig, u8 note) {
   if (scale > SCALE_MAX_ENUM_VALUE) {
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid scale=%d", scale);
      return CHORD_ERROR;  // invalid
   }
   if ((modeGroup < 0) || (modeGroup >= NUM_MODE_GROUPS)) {
      return CHORD_ERROR;
   }

   // Get the mode scale from the table or error if not found
   mode_chords_t* pModeChords = NULL;
   int found = 0;
   for (int modeIndex = 0;modeIndex < NUM_MODES;modeIndex++) {
      pModeChords = &mode_group_chord_table[modeGroup][modeIndex];
      if (pModeChords->scale == scale) {
         found = 1;
         break;
      }
   }

   if (!found) {
      DEBUG_MSG("ARP_MODES_GetChordType:  Scale: %s and mode group %d match not found", SEQ_SCALE_NameGet(scale), modeGroup);
      return CHORD_ERROR;  // invalid      
   }

   // Check if key is part of mode scale.  
   if (!SEQ_SCALE_IsNoteInScale(pModeChords->scale, keySig, note)) {
#ifdef DEBUG
      DEBUG_MSG("ARP_MODES_GetModeChord:  note: %d in key:%d not in scale %s", note, keySig, SEQ_SCALE_NameGet(scale));
#endif
      // Note is not in the scale
      return CHORD_INVALID;
   }
   // return the chord by index within the mode group by pulling the index for the scale from the SEQ_SCALE database.
   u8 index = SEQ_SCALE_GetScaleIndex(pModeChords->scale, keySig, note);
   if (index >= pModeChords->numChords){
      return CHORD_ERROR;    // Shouldn't ever happen, but need to check
   }
   else{
      return pModeChords->chords[index];
   }
}

/////////////////////////////////////////////////////////////////////////////
// Utility returns string name for each note.
// input:  notenumber from 0 to 127
/////////////////////////////////////////////////////////////////////////////
const char* ARP_MODES_GetNoteName(u8 note) {
   u8 key = note % 12;
   return key_names[key];
}
