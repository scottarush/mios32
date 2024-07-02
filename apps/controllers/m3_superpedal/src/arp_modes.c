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
s32 ARP_MODES_GetChordTableIndex(scale_t scale,chord_extflags_t extFlags);

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char * key_names[] ={"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};

// Defines the natural chords in the mode by key.
typedef struct mode_chords_entry_s {
   chord_type_t chords[12];
   chord_extflags_t chordExtFlags;
   scale_t scale;
} mode_chords_entry_t;
#define MODE_CHORD_TABLE_SIZE 7

static const mode_chords_entry_t mode_chord_table[] = {
   {{CHORD_MAJOR_I,  CHORD_MINOR_I, CHORD_MINOR_I, CHORD_MAJOR_I, CHORD_DOM7,    CHORD_MINOR_I, CHORD_MIN7B5},    CHORD_EXT_NONE,   SCALE_IONIAN},
   {{CHORD_MINOR_I,  CHORD_MINOR_I,  CHORD_MAJOR_I, CHORD_DOM7, CHORD_MINOR_I, CHORD_MIN7B5, CHORD_MAJOR_I},   CHORD_EXT_NONE,   SCALE_DORIAN},
   {{CHORD_MINOR_I,  CHORD_MAJOR_I, CHORD_DOM7,    CHORD_MINOR_I, CHORD_MIN7B5,  CHORD_MAJOR_I, CHORD_MINOR_I},   CHORD_EXT_NONE,   SCALE_PHRYGIAN},
   {{CHORD_MAJOR_I,  CHORD_DOM7,    CHORD_MINOR_I, CHORD_MIN7B5,  CHORD_MAJOR_I, CHORD_MINOR_I, CHORD_MINOR_I},   CHORD_EXT_NONE,   SCALE_LYDIAN},
   {{CHORD_DOM7,     CHORD_MINOR_I, CHORD_MIN7B5,  CHORD_MAJOR_I, CHORD_MINOR_I, CHORD_MINOR_I, CHORD_MAJOR_I},   CHORD_EXT_NONE,   SCALE_MIXOLYDIAN},
   {{CHORD_MINOR_I,  CHORD_MIN7B5,  CHORD_MAJOR_I, CHORD_MINOR_I, CHORD_MINOR_I, CHORD_MAJOR_I, CHORD_DOM7},      CHORD_EXT_NONE,   SCALE_AEOLIAN},
   {{CHORD_MIN7B5,   CHORD_MAJOR_I, CHORD_MINOR_I, CHORD_MINOR_I, CHORD_DOM7,    CHORD_MAJOR_I, CHORD_MINOR_I},   CHORD_EXT_NONE,   SCALE_LOCRIAN},
};


/////////////////////////////////////////////////////////////////////////////
// Returns chord type for mode and note
// scale:  selected scale.
// extFlags:  extension flags
// keySig:  key signature root
// note:  root note of the chord 
/////////////////////////////////////////////////////////////////////////////
const chord_type_t ARP_MODES_GetModeChord(scale_t scale,chord_extflags_t extFlags,u8 keySig,u8 note){
   if (scale > SCALE_MAX_ENUM_VALUE){
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid scale=%d",scale);
      return CHORD_ERROR;  // invalid
   }

   // Get the mode scale from the table or error if not found
   int modeTableIndex = -1;
   for(int i=0;i < MODE_CHORD_TABLE_SIZE;i++){
      scale_t checkScale = mode_chord_table[i].scale;
      if (checkScale == scale){
         modeTableIndex = i;
         break;
      }
   }
   if (modeTableIndex < 0){
      DEBUG_MSG("ARP_MODES_GetChordType:  Scale=%d not found in Mode table.",scale);
      return CHORD_ERROR;  // invalid      
   }
   // Check if extFlags match
   if (mode_chord_table[modeTableIndex].chordExtFlags != extFlags){
      DEBUG_MSG("ARP_MODES_GetChordType:  chordExtFlags=%d not available for modeScale %s",SEQ_SCALE_NameGet(scale));
      return CHORD_INVALID;
   }
   // Check if key is part of mode scale.  
   if (!SEQ_SCALE_IsNoteInScale(mode_chord_table[modeTableIndex].scale,keySig,note)){
#ifdef DEBUG
      DEBUG_MSG("ARP_MODES_GetModeChord:  note: %d in key:%d not in scale %s",note,keySig,SEQ_SCALE_NameGet(scale));
#endif
      // Note is not in the scale
      return CHORD_INVALID;
   }


   // Otherwise return the chord.
   const mode_chords_entry_t * ptr = &mode_chord_table[modeTableIndex];
   u8 index = SEQ_SCALE_GetScaleIndex(ptr->scale,keySig,note);
   return ptr->chords[index];
}

/////////////////////////////////////////////////////////////////////////////
// Utility returns string name for each note.
// input:  notenumber from 0 to 127
/////////////////////////////////////////////////////////////////////////////
const char * ARP_MODES_GetNoteName(u8 note){
   u8 key = note % 12;
   return key_names[key];
}




/////////////////////////////////////////////////////////////////////////////
// helper returns the index of the chord within the chord table for the requested
// scale and extension flags.
// scale:  scale
// extFlags:  flags
// Returns -1 if combination not found in table
/////////////////////////////////////////////////////////////////////////////
s32 ARP_MODES_GetChordTableIndex(scale_t scale,chord_extflags_t extFlags){
   for(int i=0;i < MODE_CHORD_TABLE_SIZE;i++){
      mode_chords_entry_t entry = mode_chord_table[i];
      if ((entry.scale == scale) && (entry.chordExtFlags == extFlags)){
         return i;
      }
   }
   return -1;
}