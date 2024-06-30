/*
 * Contains required dependencies from the MidiboxNG_SeqV4 copied from the 
 * seq_core.c file.   
 *
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */
#include "seq_core.h"

/////////////////////////////////////////////////////////////////////////////
// This function ensures, that a (transposed) note is within
// the <lower>..<upper> range.
//
// If the note is outside the range, it will be "trimmed" in the semitone
// range, and the octave will be kept.
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_CORE_TrimNote(s32 note, u8 lower, u8 upper)
{
  // negative note (e.g. after transpose?)
  // shift it to the positive range
  if( note < 0 )
    note = 11 - ((-note - 1) % 12);

  // check for lower boundary
  if( note < (s32)lower ) {
    note = 12*(lower/12) + (note % 12);
  }

  // check for upper boundary
  if( note > (s32)upper ) {
    note = 12*(upper/12) + (note % 12);

    // if note still > upper value (e.g. upper is set to >= 120)
    // an if (instead of while) should work in all cases, because note will be (12*int(127/12)) + 11 = 131 in worst case!
    if( note > upper )
      note -= 12;
  }

  return note;
}
