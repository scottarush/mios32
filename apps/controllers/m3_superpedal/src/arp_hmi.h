/*
 * include for M3 super pedal HMI
 *  
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 */

#ifndef _ARP_HMI_H
#define _ARP_HMI_H


#include "indicators.h"
#include "arp_modes.h"
/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define NUM_STOMP_SWITCHES 5
#define NUM_TOE_SWITCHES 8

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Persisted ARP HMI data to E2 .
/////////////////////////////////////////////////////////////////////////////

typedef struct persisted_arp_hmi_data_s {
   // First 4 bytes must be serialization version ID.  Big-ended order
   u32 serializationID;

   u8 lastArpSettingsPageIndex;

   mode_groups_t currentModeGroup;

} persisted_arp_hmi_data_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ARP_HMI_Init(u8);

extern void ARP_HMI_UpdateArpStompIndicator(indicator_id_t indicator);
extern void ARP_HMI_UpdateChordStompIndicator(indicator_id_t);

extern void ARP_HMI_ARPSettingsPage_UpdateDisplay();
extern void ARP_HMI_HandleArpToeToggle(u8, u8);
extern void ARP_HMI_HandleChordToeToggle(u8 toeNum, u8 pressed);
extern s32 ARP_HMI_PersistData();
extern void ARP_HMI_ARPSettingsPage_RotaryEncoderChanged(s8 increment);
extern void ARP_HMI_ARPSettingsPage_RotaryEncoderSelected();
extern void ARP_HMI_ARPPatternPage_RotaryEncoderChanged(s8 increment);

extern void ARP_HMI_ARPPatternPage_UpdateDisplay();

extern void ARP_HMI_ModeGroupPage_UpdateDisplay();
extern void ARP_HMI_ModeGroupPage_RotaryEncoderChanged(s8 increment);

///////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _ARP_HMI_H */
