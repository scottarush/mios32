// $Id$
/*
 * Keyboard Preset handling
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * Adapted from Thorsten's original for Studio 90 and
 * converted to object-block based Presets by Scott Rush 2024
 *
 * ==========================================================================
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>
#include <keyboard.h>

#include "keyboard_presets.h"
#include "midimon.h"
#include "uip_task.h"
#include "osc_server.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes the keyboard presents.
// if init > 0 then rewrite all of the presets, otherwise restore from E2
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Init(u32 mode) {

   if (mode >= 0) {
      // Rewrite all the defaults.
      // Reinit 
      KEYBOARD_Init(1);
      // and then store default values
      PRESETS_StoreAll();
      return 0;
   }
   // Otherwise, fall through to read the existing keyboard presets from E2

   s32 status = 0;

   u8 midimon_setup = PRESETS_Read16(PRESETS_ADDR_MIDIMON);
   status |= MIDIMON_InitFromPresets((midimon_setup >> 0) & 1, (midimon_setup >> 1) & 1, (midimon_setup >> 2) & 1);

   u8 num_srio = PRESETS_Read16(PRESETS_ADDR_NUM_SRIO);
   if (num_srio)
      MIOS32_SRIO_ScanNumSet(num_srio);


   status |= UIP_TASK_InitFromPresets(PRESETS_Read16(PRESETS_ADDR_UIP_USE_DHCP),
      PRESETS_Read32(PRESETS_ADDR_UIP_IP01),
      PRESETS_Read32(PRESETS_ADDR_UIP_NETMASK01),
      PRESETS_Read32(PRESETS_ADDR_UIP_GATEWAY01));

   u8 con = 0;
   for (con = 0; con < PRESETS_NUM_OSC_RECORDS; ++con) {
      int offset = con * PRESETS_OFFSET_BETWEEN_OSC_RECORDS;
      status |= OSC_SERVER_InitFromPresets(con,
         PRESETS_Read32(PRESETS_ADDR_OSC0_REMOTE01 + offset),
         PRESETS_Read16(PRESETS_ADDR_OSC0_REMOTE_PORT + offset),
         PRESETS_Read16(PRESETS_ADDR_OSC0_LOCAL_PORT + offset));
   }

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   u16 note_offsets = PRESETS_Read16(PRESETS_ADDR_NOTE_OFFSET );
   kc->note_offset = (note_offsets >> 0) & 0xff;
   kc->din_key_offset = (note_offsets >> 8) & 0xff;

   kc->num_rows = PRESETS_Read16(PRESETS_ADDR_ROWS );
   kc->dout_sr1 = PRESETS_Read16(PRESETS_ADDR_DOUT_SR1 );
   kc->dout_sr2 = PRESETS_Read16(PRESETS_ADDR_DOUT_SR2 );
   kc->din_sr1 = PRESETS_Read16(PRESETS_ADDR_DIN_SR1 );
   kc->din_sr2 = PRESETS_Read16(PRESETS_ADDR_DIN_SR2 );

   u16 misc = PRESETS_Read16(PRESETS_ADDR_MISC );
   kc->din_inverted = (misc & (1 << 0)) ? 1 : 0;
   kc->break_inverted = (misc & (1 << 1)) ? 1 : 0;
   kc->scan_velocity = (misc & (1 << 2)) ? 1 : 0;
   kc->scan_optimized = (misc & (1 << 3)) ? 1 : 0;
   kc->scan_release_velocity = (misc & (1 << 4)) ? 1 : 0;
   kc->make_debounced = (misc & (1 << 5)) ? 1 : 0;

   kc->delay_fastest = PRESETS_Read16(PRESETS_ADDR_DELAY_FASTEST );
   kc->delay_slowest = PRESETS_Read16(PRESETS_ADDR_DELAY_SLOWEST );

   kc->delay_fastest_black_keys = PRESETS_Read16(PRESETS_ADDR_DELAY_FASTEST_BLACK_KEYS );
   kc->delay_fastest_release = PRESETS_Read16(PRESETS_ADDR_DELAY_FASTEST_RELEASE );
   kc->delay_fastest_release_black_keys = PRESETS_Read16(PRESETS_ADDR_DELAY_FASTEST_RELEASE_BLACK_KEYS );
   kc->delay_slowest_release = PRESETS_Read16(PRESETS_ADDR_DELAY_SLOWEST_RELEASE );

   // AIN device presets
   int i = 0;
   for (i = 0; i < KEYBOARD_AIN_NUM; ++i) {
      u16 ain_cfg1 = PRESETS_Read16(PRESETS_ADDR_AIN_CFG1_1 + i * 2 );
      kc->ain_pin[i] = (ain_cfg1 >> 0) & 0xff;
      kc->ain_ctrl[i] = (ain_cfg1 >> 8) & 0xff;

      u16 ain_cfg2 = PRESETS_Read16(PRESETS_ADDR_AIN_CFG1_2 + i * 2 );
      kc->ain_min[i] = (ain_cfg2 >> 0) & 0xff;
      kc->ain_max[i] = (ain_cfg2 >> 8) & 0xff;
   }

   u16 ain_cfg5 = PRESETS_Read16(PRESETS_ADDR_AIN_CFG5 );
   kc->ain_bandwidth_ms = (ain_cfg5 >> 0) & 0xff;
   kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] = (ain_cfg5 >> 8) & 1;
   kc->ain_inverted[KEYBOARD_AIN_MODWHEEL] = (ain_cfg5 >> 9) & 1;
   kc->ain_inverted[KEYBOARD_AIN_SUSTAIN] = (ain_cfg5 >> 10) & 1;
   kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] = (ain_cfg5 >> 11) & 1;
   kc->ain_sustain_switch = (ain_cfg5 >> 15) & 1;


   // restore calibration data
   u16 calidata_base = PRESETS_ADDR_CALIDATA_BEGIN;
   for (i = 0; i < 128 && i < KEYBOARD_MAX_KEYS; ++i) { // note: actually KEYBOARD_MAX_KEYS is 128, we just want to avoid memory overwrites for the case that somebody defines a lower number
      u16 delay = PRESETS_Read16(calidata_base + i);
      kc->delay_key[i] = delay;
   }
   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help functions to read a value from EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
u16 PRESETS_Read16(u16 addr) {
   return EEPROM_Read(addr);
}

u32 PRESETS_Read32(u16 addr) {
   return ((u32)EEPROM_Read(addr) << 16) | EEPROM_Read(addr + 1);
}

/////////////////////////////////////////////////////////////////////////////
// Help function to write a value into EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Write16(u16 addr, u16 value) {
   return EEPROM_Write(addr, value);
}

s32 PRESETS_Write32(u16 addr, u32 value) {
   return EEPROM_Write(addr, (value >> 16) & 0xffff) | EEPROM_Write(addr + 1, (value >> 0) & 0xffff);
}


/////////////////////////////////////////////////////////////////////////////
// Stores all presets
// (the EEPROM emulation ensures that a value won't be written if it is already
// equal)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_StoreAll(void) {
   s32 status = 0;

   // write MIDImon data
   status |= PRESETS_Write16(PRESETS_ADDR_MIDIMON,
      (MIDIMON_ActiveGet() ? 1 : 0) |
      (MIDIMON_FilterActiveGet() ? 2 : 0) |
      (MIDIMON_TempoActiveGet() ? 4 : 0));

   // write number of SRIOs
   status |= PRESETS_Write16(PRESETS_ADDR_NUM_SRIO,
      MIOS32_SRIO_ScanNumGet());

   // write uIP data
   status |= PRESETS_Write16(PRESETS_ADDR_UIP_USE_DHCP, UIP_TASK_DHCP_EnableGet());
   status |= PRESETS_Write32(PRESETS_ADDR_UIP_IP01, UIP_TASK_IP_AddressGet());
   status |= PRESETS_Write32(PRESETS_ADDR_UIP_NETMASK01, UIP_TASK_NetmaskGet());
   status |= PRESETS_Write32(PRESETS_ADDR_UIP_GATEWAY01, UIP_TASK_GatewayGet());

   // write OSC data
   u8 con = 0;
   for (con = 0; con < PRESETS_NUM_OSC_RECORDS; ++con) {
      int offset = con * PRESETS_OFFSET_BETWEEN_OSC_RECORDS;
      status |= PRESETS_Write32(PRESETS_ADDR_OSC0_REMOTE01 + offset, OSC_SERVER_RemoteIP_Get(con));
      status |= PRESETS_Write16(PRESETS_ADDR_OSC0_REMOTE_PORT + offset, OSC_SERVER_RemotePortGet(con));
      status |= PRESETS_Write16(PRESETS_ADDR_OSC0_LOCAL_PORT + offset, OSC_SERVER_LocalPortGet(con));
   }


   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;
   status |= PRESETS_Write16(PRESETS_ADDR_NOTE_OFFSET , kc->note_offset | ((u16)kc->din_key_offset << 8));
   status |= PRESETS_Write16(PRESETS_ADDR_ROWS , kc->num_rows);
   status |= PRESETS_Write16(PRESETS_ADDR_DOUT_SR1 , kc->dout_sr1);
   status |= PRESETS_Write16(PRESETS_ADDR_DOUT_SR2 , kc->dout_sr2);
   status |= PRESETS_Write16(PRESETS_ADDR_DIN_SR1 , kc->din_sr1);
   status |= PRESETS_Write16(PRESETS_ADDR_DIN_SR2 , kc->din_sr2);

   u16 misc =
      (kc->din_inverted << 0) |
      (kc->break_inverted << 1) |
      (kc->scan_velocity << 2) |
      (kc->scan_optimized << 3) |
      (kc->scan_release_velocity << 4) |
      (kc->make_debounced << 5);
   status |= PRESETS_Write16(PRESETS_ADDR_MISC , misc);

   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_FASTEST , kc->delay_fastest);
   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_SLOWEST , kc->delay_slowest);
   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_FASTEST_BLACK_KEYS , kc->delay_fastest_black_keys);
   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_FASTEST_RELEASE , kc->delay_fastest_release);
   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_FASTEST_RELEASE_BLACK_KEYS , kc->delay_fastest_release_black_keys);
   status |= PRESETS_Write16(PRESETS_ADDR_DELAY_SLOWEST_RELEASE , kc->delay_slowest_release);

   // Current zone.  Only write out the valid zones
   int i;
   status |= PRESETS_Write16(PRESETS_ADDR_NUMZONES , kc->current_zone_preset.numZones);   
   for (i = 0; i < kc->current_zone_preset.numZones;i++) {
      zone_params_t * zoneParams = &kc->current_zone_preset.zoneParams[i];
      status |= PRESETS_Write16(PRESETS_ADDR_NOTE_NUMBER_ARRAY + i, zoneParams->startNoteNum);
      status |= PRESETS_Write16(PRESETS_ADDR_OCTAVE_OFFSET_ARRAY + i, zoneParams->octaveOffset);
      status |= PRESETS_Write16(PRESETS_ADDR_MIDI_CHANNELS_ARRAY + i, zoneParams->midiChannel);
      status |= PRESETS_Write16(PRESETS_ADDR_MIDI_PORTS_ARRAY + i, zoneParams->midiPorts);
   }

   for (i = 0; i < KEYBOARD_AIN_NUM; ++i) {
      u16 ain_cfg1 =
         (kc->ain_pin[i] << 0) |
         (kc->ain_ctrl[i] << 8);
      status |= PRESETS_Write16(PRESETS_ADDR_AIN_CFG1_1 + i * 2 , ain_cfg1);

      u16 ain_cfg2 =
         (kc->ain_min[i] << 0) |
         (kc->ain_max[i] << 8);
      status |= PRESETS_Write16(PRESETS_ADDR_AIN_CFG1_2 + i * 2 , ain_cfg2);
   }

#if KEYBOARD_AIN_NUM != 4
# error "please adapt the code below"
#endif
   u16 ain_cfg5 =
      (kc->ain_bandwidth_ms << 0) |
      (kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] ? 0x0100 : 0) |
      (kc->ain_inverted[KEYBOARD_AIN_MODWHEEL] ? 0x0200 : 0) |
      (kc->ain_inverted[KEYBOARD_AIN_SUSTAIN] ? 0x0400 : 0) |
      (kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] ? 0x0800 : 0) |
      (kc->ain_sustain_switch ? 0x8000 : 0);
   status |= PRESETS_Write16(PRESETS_ADDR_AIN_CFG5 , ain_cfg5);

   // store calibration data

   u16 calidata_base = PRESETS_ADDR_CALIDATA_BEGIN;
   for (i = 0; i < 128 && i < KEYBOARD_MAX_KEYS; ++i) { // note: actually KEYBOARD_MAX_KEYS is 128, we just want to avoid memory overwrites for the case that somebody defines a lower number
      status |= PRESETS_Write16(calidata_base + i, kc->delay_key[i]);
   }




#if DEBUG_VERBOSE_LEVEL >= 1
   if (status < 0) {
      DEBUG_MSG("[PRESETS] ERROR while writing into EEPROM!");
   }
#endif

   return 0; // no error
}

