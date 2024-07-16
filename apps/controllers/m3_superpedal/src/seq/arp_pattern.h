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
#define MAX_NUM_PATTERNS 32

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum step_type_e {
   STEP_TYPE_RNDM = 0,
   STEP_TYPE_CHORD = 1,
   STEP_TYPE_TIE = 2,
   STEP_TYPE_NORM = 3,
   STEP_TYPE_REST = 4,
   STEP_TYPE_OFF = 5
} step_type_t;

typedef struct step_event_s {
   step_type_t stepType;
   u8 keySelect;
   s8 octave;
   s8 scaleStep;
} step_event_t;

typedef struct arp_pattern_s {
   const char * name;
   u8 numSteps;
   step_event_t events[MAX_NUM_STEPS];
} arp_pattern_t;

typedef struct step_note_s {
   u8 note;
   u8 velocity;
   u8 length;
} step_note_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_PAT_Init();

extern s32 ARP_PAT_KeyPressed(u8 note, u8 velocity);
extern void ARP_PAT_KeyReleased(u8 note, u8 velocity);

extern const arp_pattern_t * ARP_PAT_GetCurrentPattern();


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_PAT_H */
