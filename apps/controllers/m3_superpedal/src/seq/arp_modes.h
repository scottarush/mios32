/*
 * Mode representation
 */

#ifndef _ARP_MODES_H
#define _ARP_MODES_H

#include "seq_chord.h"
#include "seq_scale.h"
/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum key_e {
   KEY_C = 0,
   KEY_CSHARP = 1,
   KEY_D = 2,
   KEY_EFLAT = 3,
   KEY_E = 4,
   KEY_F = 5,
   KEY_FSHARP = 6,
   KEY_G = 7,
   KEY_AFLAT = 8,
   KEY_A = 9,
   KEY_BFLAT = 10,
   KEY_B = 11
} key_t;

typedef enum mode_groups_e {
   MODE_GROUP_BASE = 0,
   MODE_GROUP_SEVENTH = 1,
   MODE_GROUP_NINTH = 2,
   MODE_GROUP_FIFTH = 3
}  mode_groups_t;
#define NUM_MODE_GROUPS 4


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern const char * ARP_MODES_GetNoteName(u8 note);
extern const chord_type_t ARP_MODES_GetModeChord(scale_t scale,mode_groups_t modeGroup,u8 keySig,u8 note);
extern const char * ARP_MODES_GetChordExtensionText(mode_groups_t extension);

extern const char * ARP_MODES_GetModeGroupName(mode_groups_t group);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_H */
