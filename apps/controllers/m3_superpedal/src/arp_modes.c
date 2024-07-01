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
#include "seq_scale.h"

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

// Defines the natural chords in the mode by key.
typedef struct mode_chords_entry_s {
   chord_type_t chords[12];
   scales_t scale;
} mode_chords_entry_t;

static const mode_chords_entry_t mode_chords_table[] = {
   {{CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_INVALID,CHORD_DOM7,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5},SCALE_IONIAN},
   {{CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_INVALID,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_INVALID,CHORD_DOM7},SCALE_DORIAN},
   {{CHORD_MINOR_I,CHORD_INVALID,CHORD_MAJOR_I,CHORD_INVALID,CHORD_DOM7,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I},SCALE_PHRYGIAN},
   {{CHORD_MAJOR_I,CHORD_INVALID,CHORD_DOM7,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MINOR_I},SCALE_LYDIAN},
   {{CHORD_DOM7,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MAJOR_I},SCALE_MIXOLYDIAN},
   {{CHORD_MINOR_I,CHORD_INVALID,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_INVALID,CHORD_DOM7,CHORD_INVALID},SCALE_AEOLIAN},
   {{CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_DOM7,CHORD_INVALID,CHORD_MAJOR_I,CHORD_INVALID,CHORD_MINOR_I,CHORD_INVALID},SCALE_LOCRIAN},
};


/////////////////////////////////////////////////////////////////////////////
// Returns chord type for mode and key
// mode:  selected mode.
// key:  normalized key from 0..11 
/////////////////////////////////////////////////////////////////////////////
const chord_type_t ARP_MODES_GetChordType(modes_t mode,key_t key){
   if (mode > MAX_MODE_TYPE){
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid mode=%d",mode);
      return CHORD_ERROR;  // invalid
   }
   else if (key > 11){
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid key=%d",key);
      return CHORD_ERROR;  // invalid
   }
   const mode_chords_entry_t * ptr = &mode_chords_table[mode];
   return ptr->chords[key];
}
