// $Id$
/*
 * Header file for chord routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_CHORD_H
#define _SEQ_CHORD_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// Offset for set 2 enums in the chord_set_type_e
#define CHORD_SET2_ENUM_OFFSET 32
/*
* Enum type added by Scott Rush.  The enum values are indexes into the
* rows of the seq_chord_table within seq_chord.c for chord set 1.
*/
typedef enum chord_type_e {
   CHORD_MAJOR_I = 0,
   CHORD_MAJOR_II = 1,
   CHORD_MAJOR_III  = 2,
   CHORD_ROOT = 3,
   CHORD_THIRD = 4,
   CHORD_FIFTH = 5,
   CHORD_ROOT_THIRD = 6,
   CHORD_ROOT_FIFTH = 7,
   CHORD_MAJ6 = 8,
   CHORD_MAJ7 = 9,
   CHORD_MAJ8 = 10,
   CHORD_MAJ9 = 11,
   CHORD_MAJ10 = 12,
   CHORD_MAJ12 = 13,
   CHORD_SUS4= 14,
   CHORD_MAJ_PLUS = 15,
   CHORD_MINOR_I = 16,
   CHORD_MINOR_II = 17,
   CHORD_MINOR_III = 18,
   CHORD_MINOR_THIRD = 20,
   CHORD_ROOT_MINOR_THIRD = 22,
   CHORD_MIN6 = 24,
   CHORD_MIN8 = 26,
   CHORD_MIN10 = 28,
   CHORD_MIN12 = 29,
   // Not sure what 'Co7' is.
   CHORD_MINOR_PLUS = 31,
   // The following addition chords are from chord set 2.  The indexes
   // are +32 (the length of the chord table)
   // Note that only the members not already part of chord_set1 are included here
   CHORD_POWER5 = CHORD_SET2_ENUM_OFFSET,
   CHORD_POWER8 = CHORD_SET2_ENUM_OFFSET+1,
   CHORD_ADD9 = CHORD_SET2_ENUM_OFFSET+9,
   CHORD_MAJ11 = CHORD_SET2_ENUM_OFFSET+11,
   CHORD_MAJ13 = CHORD_SET2_ENUM_OFFSET+12,
   CHORD_MIN7 = CHORD_SET2_ENUM_OFFSET + 15,
   CHORD_MIN_ADD9 = CHORD_SET2_ENUM_OFFSET+16,
   CHORD_MIN9 = CHORD_SET2_ENUM_OFFSET + 17,
   CHORD_MIN11 = CHORD_SET2_ENUM_OFFSET+18,
   CHORD_MIN13 = CHORD_SET2_ENUM_OFFSET+19,
   CHORD_DOM7 = CHORD_SET2_ENUM_OFFSET+20,
   CHORD_DOM7SUS4 = CHORD_SET2_ENUM_OFFSET+21,
   CHORD_DOM9  = CHORD_SET2_ENUM_OFFSET+22,
   CHORD_DOM11 = CHORD_SET2_ENUM_OFFSET+23,
   CHORD_DOM13 = CHORD_SET2_ENUM_OFFSET+24,
   CHORD_DOM7B5 = CHORD_SET2_ENUM_OFFSET+25,
   CHORD_DOM7_SHARP5 = CHORD_SET2_ENUM_OFFSET+26, 
   CHORD_DOM7B9 = CHORD_SET2_ENUM_OFFSET+27,
   CHORD_DOM7_SHARP9 = CHORD_SET2_ENUM_OFFSET+28,
   CHORD_DIM3 = CHORD_SET2_ENUM_OFFSET+29,
   CHORD_DIM = CHORD_SET2_ENUM_OFFSET+30,
   CHORD_MIN7B5 = CHORD_SET2_ENUM_OFFSET+31,
   CHORD_INVALID = 0xFE,
   CHORD_ERROR = 0xFF
} chord_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CHORD_Init(u32 mode);

extern s32 SEQ_CHORD_NumGet(u8 chord_set);
extern char *SEQ_CHORD_NameGet(u8 chord_set, u8 chord_ix);
extern s32 SEQ_CHORD_NoteGet(u8 key_num, u8 chord_set, u8 chord);

extern s32 SEQ_CHORD_NoteGetByEnum(u8 key_num, chord_type_t chord, s8 oct_transpose);
extern const char * SEQ_CHORD_NameGetByEnum(chord_type_t chord);
extern u8 SEQ_CHORD_GetNumNotesByEnum(chord_type_t chord);
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_CHORD_H */
