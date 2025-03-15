// $Id$
/*
 * Preset handling
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _PRESETS_H
#define _PRESETS_H

#include <eeprom.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// Keyboard presets always start at 8 so we can reuse Thorsten's original for now
// Other blocks must start at 0x200
#define KEYBOARD_PRESETS_START_ADDR 8
#define KEYBOARD_PRESETS_END_ADDR 0x200

#define PRESETS_ADDR_UIP_USE_DHCP  0x08
#define PRESETS_ADDR_UIP_IP01      0x12
#define PRESETS_ADDR_UIP_IP23      0x13
#define PRESETS_ADDR_UIP_NETMASK01 0x14
#define PRESETS_ADDR_UIP_NETMASK23 0x15
#define PRESETS_ADDR_UIP_GATEWAY01 0x16
#define PRESETS_ADDR_UIP_GATEWAY23 0x17

#define PRESETS_NUM_OSC_RECORDS            4
#define PRESETS_OFFSET_BETWEEN_OSC_RECORDS 8
#define PRESETS_ADDR_OSC0_REMOTE01    (0x20 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x20
#define PRESETS_ADDR_OSC0_REMOTE23    (0x21 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x21
#define PRESETS_ADDR_OSC0_REMOTE_PORT (0x22 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x22
#define PRESETS_ADDR_OSC0_LOCAL_PORT  (0x23 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x23

#define PRESETS_ADDR_OSC1_REMOTE01    (0x20 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x28
#define PRESETS_ADDR_OSC1_REMOTE23    (0x21 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x29
#define PRESETS_ADDR_OSC1_REMOTE_PORT (0x22 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x2a
#define PRESETS_ADDR_OSC1_LOCAL_PORT  (0x23 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x2b

#define PRESETS_ADDR_OSC2_REMOTE01    (0x20 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x30
#define PRESETS_ADDR_OSC2_REMOTE23    (0x21 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x31
#define PRESETS_ADDR_OSC2_REMOTE_PORT (0x22 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x32
#define PRESETS_ADDR_OSC2_LOCAL_PORT  (0x23 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x33

#define PRESETS_ADDR_OSC3_REMOTE01    (0x20 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x38
#define PRESETS_ADDR_OSC3_REMOTE23    (0x21 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x39
#define PRESETS_ADDR_OSC3_REMOTE_PORT (0x22 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x3a
#define PRESETS_ADDR_OSC3_LOCAL_PORT  (0x23 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x3b

#define PRESETS_ADDR_NUM_SRIO          0x3C
#define PRESETS_ADDR_MIDIMON           0x3D

#define PRESETS_ADDR_NOTE_OFFSET     0x42 // 0xc2
#define PRESETS_ADDR_ROWS            0x43 // 0xc3
#define PRESETS_ADDR_DOUT_SR1        0x44 // 0xc4
#define PRESETS_ADDR_DOUT_SR2        0x45 // 0xc5
#define PRESETS_ADDR_DIN_SR1         0x46 // 0xc6
#define PRESETS_ADDR_DIN_SR2         0x47 // 0xc7
#define PRESETS_ADDR_MISC            0x48 // 0xc8
#define PRESETS_ADDR_DELAY_FASTEST   0x49 // 0xc9
#define PRESETS_ADDR_DELAY_SLOWEST   0x4a // 0xca
#define PRESETS_ADDR_AIN_CFG1_1      0x4b // 0xcb
#define PRESETS_ADDR_AIN_CFG1_2      0x4c // 0xcc
#define PRESETS_ADDR_AIN_CFG2_1      0x4d // 0xcd
#define PRESETS_ADDR_AIN_CFG2_2      0x4e // 0xce
#define PRESETS_ADDR_AIN_CFG3_1      0x4f // 0xcf
#define PRESETS_ADDR_AIN_CFG3_2      0x50 // 0xd0
#define PRESETS_ADDR_AIN_CFG4_1      0x51 // 0xd1
#define PRESETS_ADDR_AIN_CFG4_2      0x52 // 0xd2
#define PRESETS_ADDR_AIN_CFG5        0x53 // 0xd3
#define PRESETS_ADDR_DELAY_FASTEST_BLACK_KEYS         0x54 // 0xd4
#define PRESETS_ADDR_DELAY_FASTEST_RELEASE            0x55 // 0xd5
#define PRESETS_ADDR_DELAY_FASTEST_RELEASE_BLACK_KEYS 0x56 // 0xd6
#define PRESETS_ADDR_DELAY_SLOWEST_RELEASE            0x57 // 0xd7

// Current zone preset data.  Maximum # of zones = 4
#define PRESETS_ADDR_NUMZONES       0x58 // 0xd8
#define PRESETS_ADDR_NOTE_NUMBER_ARRAY     0x59 // 0x59 to 0x5c
#define PRESETS_ADDR_OCTAVE_OFFSET_ARRAY       0x5D // 0x5d to 0x0x61
#define PRESETS_ADDR_MIDI_CHANNELS_ARRAY         0x62 // 0x62 to 0x65
#define PRESETS_ADDR_MIDI_PORTS_ARRAY            0x65 // 0x65 to 0x68
#define PRESETS_ADDR_VELOCITY_CURVE_ARRAY        0x69  // 0x69 to 0x7d

#define PRESETS_ADDR_CALIDATA_BEGIN  0x70 // 128 halfwords (=256 bytes)
#define PRESETS_ADDR_CALIDATA_END    0xEF

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 PRESETS_Init(u32 mode);

extern u16 PRESETS_Read16(u16 addr);
extern u32 PRESETS_Read32(u16 addr);

extern s32 PRESETS_Write16(u16 addr, u16 value);
extern s32 PRESETS_Write32(u16 addr, u32 value);

extern s32 PRESETS_StoreAll(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _PRESETS_H */
