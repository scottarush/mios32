/*
 * Sequencer and Arpeggiator functions for M3 superpedal
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *  
 *  with extensions by Scott Rush 2024.
 */

#ifndef _ARP_H
#define _ARP_H

#include <notestack.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
typedef enum {
   ARP_GEN_MODE_ASCENDING = 0,
   ARP_GEN_MODE_DESCENDING = 1,
   ARP_GEN_MODE_ASC_DESC = 2,
   ARP_GEN_MODE_OCTAVE = 3,
   ARP_GEN_MODE_RANDOM = 4
} arp_gen_mode_type_t;


/////////////////////////////////////////////////////////////////////////////
// Persisted Arpeggiatordata to E2 .
/////////////////////////////////////////////////////////////////////////////
typedef struct {
   arp_gen_mode_type_t genMode;
   int ppqn;
   double bpm;
} persisted_arp_data_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_Init(u32 mode);

extern s32 ARP_Reset(void);

extern s32 ARP_Handler(void);

extern s32 ARP_NotifyNewNote(notestack_t *n);

extern s32 ARP_NotifyNoteOn(u8 note, u8 velocity);
extern s32 ARP_NotifyNoteOff(u8 note);

extern arp_gen_mode_type_t ARP_GetArpGenMode();
extern u8 ARP_SetArpGenMode(arp_gen_mode_type_t mode);

extern void ARP_SetEnabled(u8 enabled);
extern u8 ARP_GetEnabled();

float ARP_GetBPM();
void ARP_SetBPM(u16 bpm);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_H */
