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

#include "seq_chord.h"
#include "arp_modes.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
typedef enum arp_gen_ordere {
   ARP_GEN_ORDER_ASCENDING = 0,
   ARP_GEN_ORDER_DESCENDING = 1,
   ARP_GEN_ORDER_ASC_DESC = 2,
   ARP_GEN_ORDER_OCTAVE = 3,
   ARP_GEN_ORDER_RANDOM = 4
} arp_gen_order_t;

typedef enum arp_mode_e {
   // Depressed keys only
   ARP_MODE_KEYS = 0,
   // Single key/pedal press arpeggiates a chord from the root.
   ARP_MODE_ROOT = 1
} arp_mode_t;

/////////////////////////////////////////////////////////////////////////////
// Persisted Arpeggiatordata to E2 .
/////////////////////////////////////////////////////////////////////////////
typedef struct persisted_arp_data_s {
   arp_gen_order_t genOrder;
   arp_mode_t arpMode;
   key_t rootKey;
   modes_t keyMode;
   int ppqn;
   double bpm;
} persisted_arp_data_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_Init();

extern s32 ARP_Reset(void);

extern s32 ARP_Handler(void);

extern s32 ARP_NotifyNewNote(notestack_t *n);

extern s32 ARP_NotifyNoteOn(u8 note, u8 velocity);
extern s32 ARP_NotifyNoteOff(u8 note);

extern arp_gen_order_t ARP_GetArpGenOrder();
extern u8 ARP_SetArpGenOrder(arp_gen_order_t mode);

extern void ARP_SetEnabled(u8 enabled);
extern u8 ARP_GetEnabled();

extern float ARP_GetBPM();
extern void ARP_SetBPM(u16 bpm);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_H */
