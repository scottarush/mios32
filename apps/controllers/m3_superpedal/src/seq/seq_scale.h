// $Id$
/*
 * Header file for scale routines with extensions for M3 SuperPedal by Scott Rush
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_SCALE_H
#define _SEQ_SCALE_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Enum of scales added by Scott Rush.
// Enum constants are indexes into seq_scale_table so that they 
// can be used in SEQ_SCALE_NameGet, NoteValueGet, etc.
/////////////////////////////////////////////////////////////////////////////
typedef enum scale_e {
   SCALE_MAJOR = 0,
   SCALE_HARMONIC_MINOR =1,
   SCALE_MELODIC_MINOR = 2,
   SCALE_NATURAL_MINOR = 3,
   SCALE_CHROMATIC = 4,
   SCALE_WHOLE_TONE = 5,
   SCALE_PENT_MAJOR = 6,
   SCALE_PENT_MINOR = 7,
   SCALE_PENT_BLUES =8,
   SCALE_PENT_NEUTRAL = 9,
   SCALE_OCTATONIC_HW = 10,
   SCALE_OCTATONIC_WH = 11,
   SCALE_IONIAN = 12,
   SCALE_DORIAN = 13,
   SCALE_PHRYGIAN = 14,
   SCALE_LYDIAN = 15,
   SCALE_MIXOLYDIAN = 16,
   SCALE_AEOLIAN = 17,
   SCALE_LOCRIAN = 18,
   SCALE_ALGERIAN = 19,
   SCALE_ARABIAN_A = 20,
   SCALE_ARABIAN_B = 21,
   SCALE_AUGMENTED = 22,
   SCALE_AUX_DIM = 23,
   SCALE_AUX_AUG = 24,
   // TODO:  Scale 25 is different with same name as 23.  Figure this out and add
   SCALE_BALINESE = 26,
   SCALE_BLUES = 27,
   SCALE_BYZANTINE = 28,
   SCALE_CHINESE = 29,
   SCALE_CHINESE_MONGOLIAN = 30,
   SCALE_DIATONIC = 31,
   SCALE_DIMINISHED = 32,
   SCALE_DIMINISHED_HALF = 33,
   SCALE_DIMINISHED_WHOLE = 34,
   SCALE_DIMINISHED_WHOLE_TONE = 35,
   SCALE_DOMINANT_7TH = 37,
   SCALE_DOUBLE_HARMONIC = 38,
   SCALE_EGYPTIAN = 39,
   SCALE_EIGHT_TONE_SPANISH = 40,
   // TODO:  Finish all of the others later
   SCALE_INVALID = 0xFF
} scale_t;
#define SCALE_MAX_ENUM_VALUE 40

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_SCALE_Init(u32 mode);

extern s32 SEQ_SCALE_NumGet(void);
extern char * SEQ_SCALE_NameGet(u8 scale);

extern s32 SEQ_SCALE_NoteValueGet(u8 note, u8 scale, u8 root);
extern s32 SEQ_SCALE_NextNoteInScale(u8 current_note, u8 scale, u8 root);

extern s32 SEQ_SCALE_GetScaleLength(u8 scale);
extern s32 SEQ_SCALE_GetScaleIndex(u8 scale,u8 root,u8 note);
extern u8 SEQ_SCALE_IsNoteInScale(u8 scale,u8 rootNote,u8 note);

extern s32 SEQ_SCALE_Note(mios32_midi_package_t *p, u8 scale, u8 root);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_SCALE_H */
