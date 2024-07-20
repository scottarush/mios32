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
#define NUM_PATTERNS 4

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum step_type_e {
   RNDM = 0,
   CHORD = 1,
   TIE = 2,
   NORM = 3,
   REST = 4,
   OFF = 5
} step_type_t;

typedef struct step_event_s {
   step_type_t stepType;
   // from 1..MAX_NUM_NOTES_PER_STEP
   u8 keySelect;
   s8 octave;
   s8 scaleStep;
} step_event_t;

typedef struct arp_pattern_s {
   u8 numSteps;
   step_event_t events[MAX_NUM_STEPS];
} arp_pattern_t;

typedef struct step_note_s {
   u8 note;
   u8 velocity;
   // duration in bpmticks from starting tickoffset in the step
   u8 length;
   // offset += in bpmticks.  Default is 0 at start of step
   s32 tickOffset;
} step_note_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_PAT_Init();

extern s32 ARP_PAT_KeyPressed(u8 note, u8 velocity);
extern s32 ARP_PAT_KeyReleased(u8 note, u8 velocity);

extern s32 ARP_PAT_SetCurrentPattern(u8 _patternIndex);
extern s32 ARP_PAT_ActivatePattern(u8 _patternIndex);

extern const char * ARP_PAT_GetCurrentPatternShortName();
extern const char * ARP_PAT_GetPatternName(u8 patternIndex);

extern s32 ARP_PAT_Tick(u32 bpm_tick);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_PAT_H */
