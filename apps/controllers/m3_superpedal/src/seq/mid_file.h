// $Id$
/*
 * Header for MIDI file access routines.  Adapted from mid_file.h by
 * Scott Rush to remove recording routines not needed for Arpeggiator sequence
 * loading in M3 SuperPedal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MID_FILE_H
#define _MID_FILE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MID_FILE_Init(u32 mode);

extern char *MID_FILE_UI_NameGet(void);
extern s32 MID_FILE_UI_NameClear(void);
extern s32 MID_FILE_FindNext(char *filename, char *next_file);
extern s32 MID_FILE_FindPrev(char *filename, char *prev_file);

extern s32 MID_FILE_open(char *filename);
extern u32 MID_FILE_read(void *buffer, u32 len);
extern s32 MID_FILE_eof(void);
extern s32 MID_FILE_seek(u32 pos);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MID_FILE_H */
