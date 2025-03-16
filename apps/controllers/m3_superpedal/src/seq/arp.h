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

#include "arp_modes.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
typedef enum arp_mode_e {
   // Arp mode off.  Notes will be played
   ARP_MODE_OFF = 0,
   // Arppegiates from multiple press keys.  Not very usable with pedal board
   ARP_MODE_MULTI_KEY = 1,
   // arpeggiates a chord from single root note key/pedal press
   ARP_MODE_ONEKEY_CHORD_ARP = 2,
   // plays modal chord from single root note key/pedal press
   ARP_MODE_CHORD_PAD = 3 
} arp_mode_t;

typedef enum arp_clock_mode_e {
   ARP_CLOCK_MODE_MASTER = 0,
   ARP_CLOCK_MODE_SLAVE = 1
} arp_clock_mode_t;

/////////////////////////////////////////////////////////////////////////////
// Persisted Arpeggiatordata to E2 .
/////////////////////////////////////////////////////////////////////////////
typedef struct persisted_arp_data_s {
   // First 4 bytes must be serialization version ID.  Big-ended order
   u32 serializationID;
   arp_mode_t arpMode;
   u8 arpPatternIndex;
   arp_clock_mode_t clockMode;
   key_t rootKey;
   scale_t modeScale;
   mode_groups_t modeGroup;
   int ppqn;
   double bpm;
} persisted_arp_data_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_Init(u8);


extern s32 ARP_Handler(void);

extern s32 ARP_NotifyNewNote(notestack_t *n);

extern s32 ARP_NotifyNoteOn(u8 note, u8 velocity);
extern s32 ARP_NotifyNoteOff(u8 note,u8 velocity);

extern s32 ARP_PlayOffEvents(void);

extern void ARP_SetArpMode(arp_mode_t mode);
extern const arp_mode_t ARP_GetArpMode();

extern void ARP_SetEnabled(u8 enabled);
extern u8 ARP_GetEnabled();
extern const char * ARP_GetArpStateText();

extern persisted_arp_data_t * ARP_GetARPSettings();

extern float ARP_GetBPM();
extern void ARP_SetBPM(u16 bpm);

extern void ARP_SetRootKey(u8 rootKey);
extern u8 ARP_GetRootKey();

extern void ARP_SetModeScale(scale_t scale);
extern scale_t ARP_GetModeScale();

extern arp_clock_mode_t ARP_GetClockMode();
extern void ARP_SetClockMode(arp_clock_mode_t mode);

extern const mode_groups_t ARP_GetChordExtension();
extern void ARP_SetChordExtension(mode_groups_t extension);


extern s32 ARP_PersistData();
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _ARP_H */
