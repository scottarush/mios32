/*
 * Header for Arpeggiator pattern data model
 *
 * ==========================================================================
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _ARP_PAT_H
#define _ARP_PAT_H

#include "notestack.h"
#include "arp.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define MAX_NUM_STEPS 16
#define MAX_NUM_NOTES_PER_STEP 4

#define NUM_PATTERNS 6

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// The step type for each step event within the pattern buffer.  With the exception of
/// RNDM, these are the same as those in the BlueARP.
typedef enum step_type_e {
   // Random notes pinned off the root key
   STEP_TYPE_RNDM = 0,
   
   // Chord formed from the root key using the current chord setting
   STEP_TYPE_CHORD = 1,
   
   // Extends the notes in the previous step with this one.
   // TODO:  Update the doc here with better description from BlueArp docs
   STEP_TYPE_TIE = 2,
   
   // Normal mode means whatever notes have been added to the note stack
   // will be added as 16th notes sequentially to the pattern buffer in 
   // their order within the notestack.
   STEP_TYPE_NORM = 3,

   // Extend the note playing from the previous step. Used to make longer notes
   STEP_TYPE_REST = 4,

   // An Off step event doesn't play a note.
   STEP_TYPE_OFF = 5
} step_type_t;

/**
 * There is one event per step
 */
typedef struct step_event_s {
   step_type_t stepType;
   // The order of the active key from 1(first) up to MAX_NUM_KEYS_PER_STEP total keys pressed  
   // Not valid, and set to 0 in the patterns, for STEP_TYPE_CHORD where all keys are to be
   // played.
   u8 keySelect;
   // The octave offset up or down
   s8 octaveOffset;
   // The scale offset up or down in steps
   s8 scaleStepOffset;
} step_event_t;

/**
 * Type for patterns defined in arp_pattern_data.  
 */
typedef struct arp_pattern_s {
   u8 numSteps;
   step_event_t events[MAX_NUM_STEPS];
} arp_pattern_t;




/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_PAT_Init();

extern s32 ARP_PAT_KeyPressed(u8 note, u8 velocity);
extern s32 ARP_PAT_KeyReleased(u8 note, u8 velocity);

extern s32 ARP_PAT_SetCurrentPattern(u8 _patternIndex);
extern s32 ARP_PAT_ActivatePattern(u8 _patternIndex);

extern s32 ARP_PAT_FillChordNotestack(notestack_t* pNotestack, u8 rootNote, u8 velocity);


extern const char * ARP_PAT_GetCurrentPatternShortName();
u8 ARP_PAT_GetCurrentPatternIndex();
extern const char * ARP_PAT_GetPatternName(u8 patternIndex);

extern s32 ARP_PAT_Tick(u32 bpm_tick);
extern void ARP_PAT_Reset();

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_PAT_H */
