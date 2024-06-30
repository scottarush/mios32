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

// Defines the natural chords in the mode by key.
typedef struct mode_chords_entry_s {
   chord_type_t chords[7];
   char name[20];
} mode_chords_entry_t;

static const mode_chords_entry_t mode_chords_table[] = {
   {{CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_DOM7,CHORD_MINOR_I,CHORD_MIN7B5},"Ionian"},
   {{CHORD_MINOR_I,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_DOM7},"Dorian"},
   {{CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_DOM7,CHORD_MINOR_I,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I},"Phyrgian"},
   {{CHORD_MAJOR_I,CHORD_DOM7,CHORD_MINOR_I,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I},"Lydian"},
   {{CHORD_DOM7,CHORD_MINOR_I,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I,CHORD_MAJOR_I},"Mixolydian"},
   {{CHORD_MINOR_I,CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_DOM7},"Aeolian"},
   {{CHORD_MIN7B5,CHORD_MAJOR_I,CHORD_MINOR_I,CHORD_MINOR_I,CHORD_MAJOR_I,CHORD_MAJOR_I,CHORD_MINOR_I},"Locrian"},
};


/////////////////////////////////////////////////////////////////////////////
// Returns chord type for mode and key
/////////////////////////////////////////////////////////////////////////////
const chord_type_t ARP_MODES_GetChordType(modes_t mode,key_t key){
   if (mode > MAX_MODE_TYPE){
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid mode=%d",mode);
      return CHORD_INVALID_ERROR;  // invalid
   }
   else if (key > 11){
      DEBUG_MSG("ARP_MODES_GetChordType:  Invalid key=%d",key);
      return CHORD_INVALID_ERROR;  // invalid
   }
   const mode_chords_entry_t * ptr = &mode_chords_table[mode];
   return ptr->chords[key];
}
