// $Id$
//! \defgroup KEYBOARD
//!
//! Keyboard Handler
//!
//! \{
/*
 * ==========================================================================
 *
 *  Adapted by Scott Rush from Thosten Kalz's keyboard.c driver for the Studio90.
 * Key changes:
 * 1.  Simplified to a single keyboard.
 * 2.  Velocity DIN logic was replaced since Scott could not get the original
 *     code to work with the Studio 90.
 * 3.  Removed supported for multiple keyboards to reduce complex
 * 4.  Removed all conditional compilations.
 *
 * ==========================================================================
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

#include "keyboard.h"

#include "keyboard_presets.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// scan 16 rows for up to 8*16 contacts
#define MATRIX_NUM_ROWS 16

// maximum number of supported contacts
#define KEYBOARD_NUM_PINS (16*MATRIX_NUM_ROWS)


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

keyboard_config_t keyboard_config;


/////////////////////////////////////////////////////////////////////////////
// Local structures
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


static u16 din_value[MATRIX_NUM_ROWS];
static u16 din_value_changed[MATRIX_NUM_ROWS];

// for velocity
static u16 timestamp;
static u16 din_activated_timestamp[KEYBOARD_NUM_PINS];

static u8 ain_cali_mode_pin;

// Callback for key learning
void (*pKeyLearningCallback)(u8 noteNumber);

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifndef KEYBOARD_NOTIFY_TOGGLE_HOOK
static s32 KEYBOARD_MIDI_SendNote(u8 note_number, u8 velocity, u8 depressed);
#else
extern s32 KEYBOARD_NOTIFY_TOGGLE_HOOK(u8 kb, u8 note_number, u8 velocity);
#endif

static s32 KEYBOARD_MIDI_SendCtrl(u8 ctrl_number, u8 value);
static int KEYBOARD_GetVelocity(u16 delay, u16 delay_slowest, u16 delay_fastest);


/////////////////////////////////////////////////////////////////////////////
//! Initialize the keyboard handler
//! \param mode == 0: init configuration + runtime variables
//! \param mode > 0: init only runtime variables
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_Init(u32 mode) {
   u8 init_configuration = mode == 0;

   pKeyLearningCallback = NULL;

   ain_cali_mode_pin = 0;

       int i;

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;
   if (init_configuration) {
      kc->note_offset = 21;  // 21 for 88 keys (a-1); 28 for 76 keys (E-0); 36 for 61 keys & 49 keys (C-1); 48 for 25 keys (C-2)

      // Defaults have been set to studio 90 measured
      kc->delay_fastest = 175;
      kc->delay_fastest_black_keys = 120; // if 0, we take delay_fastest, otherwise we take this value for the black keys
      kc->delay_fastest_release = 250;   // if 0, we take delay_fastest, otherwise we take this value for releasing keys
      kc->delay_fastest_release_black_keys = 160; // if 0, we take delay_fastest_release, otherwise we take this value for releasing black keys
      kc->delay_slowest = 500;
      kc->delay_slowest_release = 1500;

      for (i = 0; i < KEYBOARD_MAX_KEYS; ++i) {
         kc->delay_key[i] = 0;
      }


      // set low verbose level by default (only warnings)
      kc->verbose_level = 1;

      kc->scan_velocity = 1;
      kc->scan_optimized = 0;
      kc->break_inverted = 0;
      kc->scan_release_velocity = 1;
      kc->make_debounced = 0;
      kc->key_calibration = 0;

      kc->num_rows = 12;
      kc->dout_sr1 = 1;
      kc->dout_sr2 = 2; // with num_rows<=8 the SR2 will mirror SR1
      kc->din_sr1 = 1;
      kc->din_sr2 = 2;
      kc->din_key_offset = 40;	// 32 for 61 keys and 76 keys; 40 for 88 keys
      kc->din_inverted = 0;


      // Initialize QINs
      for (i = 0; i < KEYBOARD_AIN_NUM; ++i) {
         kc->ain_pin[i] = 0;
         // Do device specific inits
         switch (i) {
         case KEYBOARD_AIN_PITCHWHEEL:
            kc->ain_ctrl[i] = 0x80;
            break;
         case KEYBOARD_AIN_MODWHEEL:
            kc->ain_ctrl[i] = 1;
            break;
         case KEYBOARD_AIN_SUSTAIN:
            kc->ain_ctrl[i] = 64;
            kc->ain_inverted[i] = 1;   // Inverted sustain pedal switch
            kc->ain_pin[i] = 3;        // A2 for Studio 90
            break;
         case KEYBOARD_AIN_EXPRESSION:
            kc->ain_ctrl[i] = 11;
            break;
         default:
            kc->ain_ctrl[i] = 16;
            kc->ain_inverted[i] = 0;
            break;
         }
         // Now global defaults
         kc->ain_min[i] = 1;
         kc->ain_max[i] = 254;
         kc->ain_last_value7[i] = 0xff; // will force to send the value
         kc->ain_timestamp[i] = 0;
      }
      kc->ain_sustain_switch = 1;   // enable sustain switch
      kc->ain_bandwidth_ms = 0;


      // runtime variables:
      kc->selected_row = 0;
      kc->prev_row = 0xff;

      // initialize DIN arrays
      int row;
      u16 inversion = kc->din_inverted ? 0xffff : 0x0000;
      for (row = 0; row < MATRIX_NUM_ROWS; ++row) {
         din_value[row] = 0xffff ^ inversion; // default state: buttons depressed
         din_value_changed[row] = 0x0000;
      }

      // initialize timestamps
      int i;
      timestamp = 0;
      for (i = 0; i < KEYBOARD_NUM_PINS; ++i) {
         din_activated_timestamp[i] = 0;
      }

      kc->current_zone_preset.numZones = 1;
      kc->current_zone_preset.zoneParams[0].midiPorts = 0x3033;  // USB and both UARTS -> MIDI
      kc->current_zone_preset.zoneParams[0].midiChannel = 1;
      kc->current_zone_preset.zoneParams[0].startNoteNum = 21;
      kc->current_zone_preset.zoneParams[0].transposeOffset = 0;

   }

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SRIO_ServicePrepare(void) {
   // increment timestamp for velocity delay measurements
   // but skip 0, which is used as reset of ts_make and ts_break values
   if (!(++timestamp))
      ++timestamp;

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   // optional scan optimization for break/make: if break not active, we don't need to scan make
   if (kc->scan_velocity && !kc->break_inverted && kc->scan_optimized) {
      u8 skip_make = 0;

      if (kc->prev_row & 1) { // last scan was break contact
         if (kc->prev_row != 0xff && // if 0xff, no row has been scanned yet
            ((!kc->din_inverted && din_value[kc->prev_row] == 0xffff) ||
               (kc->din_inverted && din_value[kc->prev_row] == 0x0000))) {
            // skip scan of make contacts
            skip_make = 1;
         }
      }

      // if scan optimization active: scan break before make
      if ((kc->selected_row & 1)) { // break
         if (skip_make) {
            if ((kc->selected_row += 2) >= kc->num_rows) // switch to next break
               kc->selected_row = 1; // restart at first break
         }
         else {
            kc->selected_row -= 1; // switch to make
         }
      }
      else {
         if ((kc->selected_row += 3) >= kc->num_rows) {
            kc->selected_row = 1; // restart at first break
         }
      }
   }
   else {
      // select next column, wrap at num_rows
      if (++kc->selected_row >= kc->num_rows) {
         kc->selected_row = 0;
      }
   }

   // selection pattern (active selection is 0, all other outputs 1)
   u16 selection_mask = ~(1 << (u16)kc->selected_row);
   if (kc->din_inverted)
      selection_mask ^= 0xffff;

   if (kc->dout_sr1)
      MIOS32_DOUT_SRSet(kc->dout_sr1 - 1, selection_mask >> 0);

   // mirror selection if <= 8 rows for second SR, otherwise output remaining 8 selection lines
   if (kc->dout_sr2)
      MIOS32_DOUT_SRSet(kc->dout_sr2 - 1, selection_mask >> ((kc->num_rows <= 8) ? 0 : 8));
}

/////////////////////////////////////////////////////////////////////////////
//! This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SRIO_ServiceFinish(void) {
   // check DINs
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;
   u16 sr_value = 0;

   // the DIN scan was done with previous row selection, not the current one:
   int prev_row = kc->prev_row;

   // store new previous row for next scan
   kc->prev_row = kc->selected_row;

   if (kc->din_sr1) {
      MIOS32_DIN_SRChangedGetAndClear(kc->din_sr1 - 1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= MIOS32_DIN_SRGet(kc->din_sr1 - 1);
   }
   else {
      sr_value |= 0x00ff;
   }

   if (kc->din_sr2) {
      MIOS32_DIN_SRChangedGetAndClear(kc->din_sr2 - 1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= (u16)MIOS32_DIN_SRGet(kc->din_sr2 - 1) << 8;
   }
   else {
      sr_value |= 0xff00;
   }

   if (kc->din_inverted)
      sr_value ^= 0xffff;

   // determine pin changes
   u16 changed = sr_value ^ din_value[prev_row];


   if (changed) {
      //	DEBUG_MSG("changed=%d sr_value=%x timestamp=%d",changed,sr_value,timestamp);
      // add them to existing notifications
      din_value_changed[prev_row] |= changed;

      // store new value
      din_value[prev_row] = sr_value;

      // number of pins per row depends on assigned DINs:
      int pins_per_row = kc->din_sr2 ? 16 : 8;
      u8 sr_pin;

      // store timestamp for first break and first make (i.e. when corresponding 
      //* *tsptr == 0
      u16 mask = 0x0001;
      u16* ts_ptr = (u16*)&din_activated_timestamp[prev_row * MATRIX_NUM_ROWS];


      if (!kc->scan_release_velocity) {
         // store timestamp for changed pin on 1->0 transition
         for (sr_pin = 0; sr_pin < pins_per_row; ++sr_pin, mask <<= 1, ++ts_ptr) {
            if ((changed & mask) && !(sr_value & mask) && !(*ts_ptr)) {
               *ts_ptr = timestamp;
               //	    				DEBUG_MSG("Scanned TS: pin %d & row %d = %d \n", sr_pin, prev_row, *ts_ptr);
            }
         }
      }
      else {
         // Handle timestamps for release velocity

         // get related contact row: MKx = BRx - 1; BRx = MK + 1 to use in detecting
         // bounces.
         u8 rel_row = prev_row + ((prev_row & 1) ? (-1) : 1);
         u16 rel_changed = din_value_changed[rel_row];
         u16 rel_sr_value = din_value[rel_row];

         // Check Make and Brake pins.  
         // Scott Rush:  I have rewritten this logic. I do not know how the code in the
         // original module could possibly have worked.
         //
         for (sr_pin = 0; sr_pin < pins_per_row; ++sr_pin, mask <<= 1, ++ts_ptr) {
            // Handle timestamps.
            if (changed & mask) {
               if (prev_row & 1) {
                  // This is a break row.  
                  if (!(rel_changed & mask) && (rel_sr_value & mask)) {
                     // Make hasn't changed and is still released so this is either an initial Key Press
                     // or the beginnig of a Key Release so save the timestamp.
                     *ts_ptr = timestamp;
                     if (kc->verbose_level >= 2)
                        DEBUG_MSG("Scanned Break TS: pin %d & row %d = %d \n", sr_pin, prev_row, *ts_ptr);
                  }
                  // Otherwise, this was a bounce on the break after the initial press or release so
                  // don't overwrite the last timestamp
               }
               else {
                  // This is a make row
                  if (!(rel_changed & mask) && (rel_sr_value & mask)) {
                     // Break hasn't changed and is still released so this is either an initial Key Press
                     // or the beginning of Key Release so save the make timestamp.
                     *ts_ptr = timestamp;
                     if (kc->verbose_level >= 2)
                        DEBUG_MSG("Scanned Make TS: pin %d & row %d = %d \n", sr_pin, prev_row, *ts_ptr);
                  }
                  // Otherwise, this was bounce on the make after the initial reelease so just ignore it.
               }
            }  // if (changed & makse
         }  // for(sr_pin)
      }  // else
   } // if(changed)						
} // for(kb )


/////////////////////////////////////////////////////////////////////////////
//! will be called on pin changes
/////////////////////////////////////////////////////////////////////////////
static void KEYBOARD_NotifyToggle(u8 kb, u8 row, u8 column, u8 depressed) {
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;
   char note_str[4];

   // Display status of all rows
   if (kc->verbose_level >= 2) {
      DEBUG_MSG("---\n");
      int i;
      for (i = 0; i < kc->num_rows; ++i) {
         int v = ~din_value[i];
         DEBUG_MSG("DOUT SR%d.%d:  %c%c%c%c%c%c%c%c  %c%c%c%c%c%c%c%c\n",
            (i / 8) + 1,
            7 - (i % 8),
            (v & 0x0001) ? '1' : '0',
            (v & 0x0002) ? '1' : '0',
            (v & 0x0004) ? '1' : '0',
            (v & 0x0008) ? '1' : '0',
            (v & 0x0010) ? '1' : '0',
            (v & 0x0020) ? '1' : '0',
            (v & 0x0040) ? '1' : '0',
            (v & 0x0080) ? '1' : '0',
            (v & 0x0100) ? '1' : '0',
            (v & 0x0200) ? '1' : '0',
            (v & 0x0400) ? '1' : '0',
            (v & 0x0800) ? '1' : '0',
            (v & 0x1000) ? '1' : '0',
            (v & 0x2000) ? '1' : '0',
            (v & 0x4000) ? '1' : '0',
            (v & 0x8000) ? '1' : '0'
         );
      }
   }

   // check if key is assigned to an "break contact"
   // and determine reference to break and make pin (valid when make pin active)
   // the make contacts are at row 0, 2, 4, 6, 8, 10, 12, 14
   // the break contacts are at row 1, 3, 5, 7, 9, 11, 13, 15

   u8 break_contact = (row & 1); // odd numbers
   u8 row_make = (row & 1) ? row - 1 : row;
   u8 row_break = (row & 1) ? row : row + 1;

   int pin_make, pin_break;
   if (kc->scan_velocity) {
      pin_make = (row_make)*MATRIX_NUM_ROWS + column;
      pin_break = (row_break)*MATRIX_NUM_ROWS + column;
   }
   else {
      break_contact = 0;
      pin_make = (row)*MATRIX_NUM_ROWS + column;
      pin_break = pin_make; // just use the same pin...
   }
   // determine key number based on row/column
   int key = ((column >= 8) ? kc->din_key_offset : 0) + 8 * ((kc->scan_velocity) ? row / 2 : row) + (column % 8);

   // ensure valid range
   if (key >= KEYBOARD_MAX_KEYS)
      key = KEYBOARD_MAX_KEYS - 1;

   // determine note number (here we could insert an octave shift)
   int note_number = key + kc->note_offset;

#if 0
   DEBUG_MSG("key:%d %c note:%d break:%d pin_make:%d pin_break:%d\n", key, depressed ? 'o' : '*', note_number, break_contact, pin_make, pin_break);
#endif

   // break pin inverted?
   if (break_contact && kc->scan_velocity && kc->break_inverted)
      depressed = depressed ? 0 : 1;

   // ensure valid note range
   if (note_number > 127)
      note_number = 127;
   else if (note_number < 0)
      note_number = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
   DEBUG_MSG("row=%d, column=%d, depressed=%d  -->  key=%d, break_contact:%d, note_number=%d\n",
      row, column, depressed, key, break_contact, note_number);
#endif

   //--------------------------------------------------------------------------
   // Scott Rush.  I hand copied and unwound the rest of this function from
   // the obfuscated logic in the stock module because I could not get it to work.
   //--------------------------------------------------------------------------

   // determine timestamps pointers between break and make contact
   u16* ts_break_ptr = (u16*)&din_activated_timestamp[pin_break];
   u16* ts_make_ptr = (u16*)&din_activated_timestamp[pin_make];

   // optionally we differ the delay_xxxx values between black and white keys.
   // IMPORTANT: we should determine the black key based on the key value (=matching with HW), and not on the MIDI note value
   // to ensure that an evtl. transposed note doesn't affect the selected value.
   u8 black_key = 0;
   if (kc->scan_velocity || kc->scan_release_velocity) {
      u8 normalized_key = (key + kc->note_offset) % 12;
      //           C#                     D#                     F#                     G#                      A#
      black_key = (1 == normalized_key || 3 == normalized_key || 6 == normalized_key || 8 == normalized_key || 10 == normalized_key);
   }

   // debounce processing
   if (kc->scan_velocity) {
      if (!kc->scan_release_velocity) {
         // break contacts don't send MIDI notes if release velocity is not enabled, but they are used for delay measurements,
         // and they release the Note On/Off debouncing mechanism
         if (break_contact) {
            if (depressed) {
               if (kc->make_debounced) {
                  // for debouncing we have to play Note Off when Break is released, because make is bouncing
                  MIOS32_IRQ_Disable();
                  *ts_break_ptr = 0;
                  MIOS32_IRQ_Enable();
                  KEYBOARD_MIDI_SendNote(note_number, 0x00, 1);
               }
               // Clear both timestamps (WHY????)
               MIOS32_IRQ_Disable();
               *ts_make_ptr = 0;
               *ts_break_ptr = 0;
               MIOS32_IRQ_Enable();
            }
            // Wait for make
            return;
         }
      }
      else {
         // debouncing with release velocity processing.  Decide if need to
      // skip this contact and return.
         u8 skipflag = 0;
         if (break_contact) {
            if (!(*ts_make_ptr)) {
               // Break has changed, but corresponding make timestamp is zero.  We are
               // either waiting for a make on press or this is a bounce after the note off has been sent.
               skipflag = 1;
            }
         }
         else {
            // make contact
            if (!(*ts_break_ptr)) {
               // Make has changed, but corresponding break timestamp is zero.  We are
               // either waiting for a break off press or this is a bounce of the make after the note On has already been sent.
               skipflag = 1;
            }
         }
         if (skipflag) {
            if (kc->verbose_level >= 2)
               DEBUG_MSG("Skipped: %s contact %s %s (currrent ts=%d; ts_br=%d, ts_mk=%d)\n",
                  break_contact ? "Break" : "Make",
                  depressed ? "released without" : "pressed with remaining",
                  break_contact ? "ts_make" : "ts_break",
                  timestamp, *ts_break_ptr, *ts_make_ptr);

            return;
         }
         // Otherwise, drop through to perform delay processing
      }
   }
   else {
      // no velocity: no debouncing yet, we could use the timestamp...
   }

   //--------------------------------------------------------------------------------
   // At this point if we didn't skip the pin transition, then this is a valid
   // debounce-checked transition.  Compute velocity and send either the NoteOn or NoteOff, 
   // and zero out the timestamps for the next Break/Make transitions.

   int velocity = 127;

   if (break_contact) {
      if (kc->scan_release_velocity) {
         if (!depressed && *ts_make_ptr) {
            // Break contact has just pressed and we have a make time stamp. This must
            // be a note off.
            MIOS32_IRQ_Disable();
            // Compute the Press delay
            u16 delay = *ts_break_ptr - *ts_make_ptr;
            // And clear the timestamps for next key press and to ignore break bounces
            *ts_make_ptr = 0;
            *ts_break_ptr = 0;
            MIOS32_IRQ_Enable();

            // Pull the fastest and slowest delays from the keyboard config to use for the velocity
            u16 delay_fastest = (black_key && kc->delay_fastest_release_black_keys) ? kc->delay_fastest_release_black_keys
               : kc->delay_fastest_release;
            u16 delay_slowest = kc->delay_slowest_release;

            velocity = KEYBOARD_GetVelocity(delay, delay_slowest, delay_fastest);
            if (kc->verbose_level >= 2)
               DEBUG_MSG("Released note=%s, delay=%d, velocity=%d (played from a %s key)\n",
                  KEYBOARD_GetNoteName(note_number, note_str), delay, velocity,
                  black_key ? "black" : "white");

            // send NoteOff with velocity
            KEYBOARD_MIDI_SendNote(note_number, velocity, 1);
         }
      }
      else {
         // No release velocity processing.  
         if (!kc->scan_velocity || !(*ts_make_ptr)) {
            // Either no velocity scan or the make ts has already been cleared
            if (!kc->make_debounced) {
               // We aren't debouncing the make so just send the Note Off now (w/ zero velocity)
               // after clearing the break timestamp for next time
               MIOS32_IRQ_Disable();
               *ts_break_ptr = 0;
               MIOS32_IRQ_Enable();
               KEYBOARD_MIDI_SendNote(note_number, 0, 1);
            }
         }

      }
   }
   else {
      //---------------------------------------------------
      // This is a make pin.  Process On velocity upon press

      // Flag to determine if we send
      u8 sendOnFlag = 0;
      if (!depressed && !kc->scan_velocity) {
         // Always send on make press if we aren't scanning velocity.
         sendOnFlag = 1;
      }
      // Otherwise check if we have a press & make timestamp yet
      else if (!depressed && *ts_make_ptr) {
         sendOnFlag = 1;
      }
      if (sendOnFlag) {
         // Compute the delay and clear the timestamps for computing the Off times (if release velocity enable)
         MIOS32_IRQ_Disable();
         u16 delay = *ts_make_ptr - *ts_break_ptr;
         *ts_break_ptr = 0;
         *ts_make_ptr = 0;
         MIOS32_IRQ_Enable();

         // Compute the velocity and send the note on
         if (kc->scan_velocity) {
            // Get fastest and slowest delays from keyboard config to compute On velocity
            u16 delay_fastest = (black_key && kc->delay_fastest_black_keys) ? kc->delay_fastest_black_keys : kc->delay_fastest;
            u16 delay_slowest = kc->delay_slowest;

            velocity = KEYBOARD_GetVelocity(delay, delay_slowest, delay_fastest);
            if (kc->verbose_level >= 2)
               DEBUG_MSG("PRESSED note=%s, delay=%d, velocity=%d (played from a %s key)\n",
                  KEYBOARD_GetNoteName(note_number, note_str), delay, velocity,
                  black_key ? "black" : "white");
         }
         else {
            // No velocity scanning so just send debug message without computing velocity
            if (kc->verbose_level >= 2)
               DEBUG_MSG("PRESSED note=%s, velocity=%d\n", KEYBOARD_GetNoteName(note_number, note_str), velocity);
         }
         // And send the note
         KEYBOARD_MIDI_SendNote(note_number, velocity, 0);
      }    // if sendOnFlag
   }   // else of if(break_contact)
}

/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically (each mS) to check for pin changes
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_Periodic_1mS(void) {
   int kb;
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;
   // number of pins per row depends on assigned DINs:
   int pins_per_row = kc->din_sr2 ? 16 : 8;

   int row;
   for (row = 0; row < kc->num_rows; ++row) {
      // check if there are pin changes - must be atomic!
      MIOS32_IRQ_Disable();
      u16 changed = din_value_changed[row];
      din_value_changed[row] = 0;
      MIOS32_IRQ_Enable();

      // any pin change at this SR?
      if (!changed)
         continue;

      // check the 16 captured pins of the two SRs
      int sr_pin;
      u16 mask = 0x01;
      for (sr_pin = 0; sr_pin < pins_per_row; ++sr_pin, mask <<= 1)
         if (changed & mask)
            KEYBOARD_NotifyToggle(kb, row, sr_pin, (din_value[row] & mask) ? 1 : 0);
   }
}



/////////////////////////////////////////////////////////////////////////////
//! This function should be called from AIN_NotifyChange in app.c
//!
//! In addition it should be called periodically from a task to handle
//! the reduced bandwidth function.\n
//! See also apps/controllers/midibox_kb_v1/src/app.c
//! 
//! \code
//!     // update AINs with current value
//!     // the keyboard driver will only send events on value changes
//!     {
//!       int pin;
//! 
//!       for(pin=0; pin<8; ++pin) {
//! 	KEYBOARD_AIN_NotifyChange(pin, MIOS32_AIN_PinGet(pin));
//!       }
//!     }
//! \endcode
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_AIN_NotifyChange(u32 pin, u32 pin_value) {
   u32 timestamp = MIOS32_TIMESTAMP_Get();

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   //DEBUG_MSG("KEYBOARD_AIN_NotifyChange:  pin=%d value=%d timestamp=%d",pin, pin_value, timestamp);


   int i;
   for (i = 0; i < KEYBOARD_AIN_NUM; ++i) {
      u8 expected_cali_mode_pin = 1 + i;

      if (ain_cali_mode_pin == expected_cali_mode_pin) {
         u8 notification = 0;

         u8 value8bit = pin_value >> 4;
         if (value8bit <= kc->ain_min[i]) {
            kc->ain_min[i] = value8bit;
            notification = 1;
         }

         if (value8bit >= kc->ain_max[i]) {
            kc->ain_max[i] = value8bit;
            notification = 1;
         }

         if (notification) {
            const char src_name[KEYBOARD_AIN_NUM][17] = {
               "PitchWheel",
               "ModWheel",
               "Sustain Pedal",
               "Expression Pedal"
            };

            DEBUG_MSG("AIN Calibration %s: min=%3d, max=%3d\n", src_name[i], kc->ain_min[i], kc->ain_max[i]);
         }
      }
      else {
         if (pin == ((int)kc->ain_pin[i] - 1)) {
            int value16bit = (pin_value << 4) - ((int)kc->ain_min[i] << 8);
            if (value16bit < 0)
               value16bit = 0;

            int range8bit = (int)kc->ain_max[i] - (int)kc->ain_min[i] + 1;

            // fixed point arithmetic
            int value7bit = (value16bit / range8bit) >> (9 - 8);

            if (value7bit < 0)
               value7bit = 0;
            if (value7bit > 127)
               value7bit = 127;

#if 0
            // for debugging the fixed point arithmetic formula
            DEBUG_MSG("value16bit=%d, range8bit=%d, value7bit=%d\n", value16bit, range8bit, value7bit);
#endif

            if (value7bit != kc->ain_last_value7[i] &&
               (!kc->ain_bandwidth_ms || ((timestamp - kc->ain_timestamp[i]) >= kc->ain_bandwidth_ms))) {
               u8 last_value7 = kc->ain_last_value7[i];
               kc->ain_last_value7[i] = value7bit;
               kc->ain_timestamp[i] = timestamp;

               int sent_value = kc->ain_inverted[i] ? (127 - value7bit) : value7bit;

               if (i == KEYBOARD_AIN_SUSTAIN && kc->ain_sustain_switch) {
                  if ((value7bit >= 0x40 && last_value7 >= 0x40) ||
                     (value7bit < 0x40 && last_value7 < 0x40)) {
                     sent_value = -1; // don't send
                  }
                  else {
                     sent_value = (sent_value >= 0x40) ? 0x7f : 0x00;
                  }
               }

               if (sent_value >= 0) {
                  KEYBOARD_MIDI_SendCtrl(kc->ain_ctrl[i], sent_value);
               }
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////
// API to set the callback for key learning.  Until called again with NULL
// the callback will be called with each Note On.
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SetKeyLearningCallback(void (*pCallback)(u8 noteNumber)) {
   DEBUG_MSG("KEYBOARD_SetKeyLearningCallback:  callback registered");
   pKeyLearningCallback = pCallback;
}

/////////////////////////////////////////////////////////////////////////////
// Utility copies one zone preset to another
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_CopyZonePreset(zone_preset_t * pSource, zone_preset_t * pDest){
   pDest->numZones = pSource->numZones;
   pDest->presetID = pSource->presetID;
   for (int i = 0;i < pSource->numZones;i++) {
      zone_params_t * pDestZoneParams = &pDest->zoneParams[i];
      zone_params_t * pSourceZoneParams = &pSource->zoneParams[i];
      pDestZoneParams->midiChannel = pSourceZoneParams->midiChannel;
      pDestZoneParams->midiPorts = pSourceZoneParams->midiPorts;
      pDestZoneParams->startNoteNum = pSourceZoneParams->startNoteNum;
      pDestZoneParams->transposeOffset = pSourceZoneParams->transposeOffset;
   }
}


/////////////////////////////////////////////////////////////////////////////
// API to set the current zone preset.  Values from the supplied preset will
// be copied into the persisted keyboard currentzone preset
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SetCurrentZonePreset(zone_preset_t* pPreset) {
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   // Copy over the supply preset to the current preset
   zone_preset_t* pCurrentPreset = &kc->current_zone_preset;

   KEYBOARD_CopyZonePreset(pPreset,pCurrentPreset);

   // Update EEPROM
   PRESETS_StoreAll();

   // TODO:  send all note offs to avoid a hung not if zone changed while key pressed
}


/////////////////////////////////////////////////////////////////////////////
// Help function to get MIDI velocity from measured delay
/////////////////////////////////////////////////////////////////////////////
static int KEYBOARD_GetVelocity(u16 delay, u16 delay_slowest, u16 delay_fastest) {
   int velocity = 127;

#if 0
   // see http://midibox.org/forums/topic/20693-midibox_ng-event-noteon-lost/?do=findComment&comment=180231
   u16 prev_delay = delay;
   if (delay < 2 * delay_fastest) {
      delay = ((delay_slowest - delay_fastest) / (delay_fastest * delay_fastest)) * (delay * delay) - 2 * ((delay_slowest - delay_fastest) / delay_fastest) * delay + delay_slowest;
   }
   else {
      delay = delay_slowest;
   }
   DEBUG_MSG("KB Delay %d -> %d\n", prev_delay, delay);
#endif

   if (delay > delay_fastest) {
      // determine velocity depending on delay
      // lineary scaling - here we could also apply a curve table
      velocity = 127 - (((delay - delay_fastest) * 127) / (delay_slowest - delay_fastest));

      // saturate to ensure that range 1..127 won't be exceeded
      if (velocity < 1)
         velocity = 1;
      if (velocity > 127)
         velocity = 127;
   }

   return velocity;
}

/////////////////////////////////////////////////////////////////////////////
//! Help function to send a MIDI note over given ports\n
//! Optionally this function can be provided from external by defining the
//! function name in KEYBOARD_NOTIFY_TOGGLE_HOOK
/////////////////////////////////////////////////////////////////////////////
static s32 KEYBOARD_MIDI_SendNote(u8 note_number, u8 velocity, u8 depressed) {
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   s32 sent_note = note_number;

   // if key learning callback is set and this is an on note, then just forward the
   // note and return.
   // TODO:  Find a way to sound a note without leaving stuck notes. This happens because on a zone change
   // that 'hides' the selected note, the Release comes out on another note number
   if (pKeyLearningCallback != NULL){
      if (!depressed){
         pKeyLearningCallback(sent_note);
         return 0;
      }
   }
   // Otherwise decode the zone and send the note
   u16 midiPort = 0;
   u8 midiChannel = 1;
   if (kc->current_zone_preset.numZones == 1) {
      midiPort = kc->current_zone_preset.zoneParams[0].midiPorts;
      midiChannel = kc->current_zone_preset.zoneParams[0].midiChannel;
   }
   else {
      // Iterate through the zones to find the correct midiChannel and port on which
      // to send the note
      for (int i = 0;i < kc->current_zone_preset.numZones;i++) {
         zone_params_t* pZoneParams = &kc->current_zone_preset.zoneParams[i];
         zone_params_t* pNextZoneParams = NULL;
         if (i < kc->current_zone_preset.numZones - 1) {
            pNextZoneParams = &kc->current_zone_preset.zoneParams[i + 1];
         }
         if (note_number >= pZoneParams->startNoteNum) {
            if ((pNextZoneParams == NULL) || (note_number < pNextZoneParams->startNoteNum)) {
               // Either the last zone or the note is in this one
               midiPort = pZoneParams->midiPorts;
               midiChannel = pZoneParams->midiChannel;
               // And add the transpose offset
               sent_note = sent_note + pZoneParams->transposeOffset;
               if (sent_note < 0){
                  sent_note = 0;  // Shouldn't happen
               }
               else if (sent_note > 127){
                  sent_note = 127;  // Shouldn't happen
               }
               break;
            }
         }
         // Otherwise check the next zone.
      }
   }
   // Now send on the found ports and channel
   int i;
   u16 mask = 1;
   for (i = 0; i < 16; ++i, mask <<= 1) {
      if (midiPort & mask) {
         // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
         mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

         if (depressed && kc->scan_release_velocity)
            MIOS32_MIDI_SendNoteOff(port, midiChannel - 1, sent_note, velocity);
         else
            MIOS32_MIDI_SendNoteOn(port, midiChannel - 1, sent_note, velocity);
      }
   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Help function to send a MIDI controller over given ports
/////////////////////////////////////////////////////////////////////////////
static s32 KEYBOARD_MIDI_SendCtrl(u8 ctrl_number, u8 value) {
   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   for (int i = 0;i < kc->current_zone_preset.numZones;i++) {
      zone_params_t* pZoneParams = &kc->current_zone_preset.zoneParams[i];

      int i;
      u16 mask = 1;
      for (i = 0; i < 16; ++i, mask <<= 1) {
         if (pZoneParams->midiPorts & mask) {
            // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
            mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

            u8 midi_chn = pZoneParams->midiChannel;

            if (ctrl_number < 128) {
               MIOS32_MIDI_SendCC(port, midi_chn - 1, ctrl_number, value);
            }
            else if (ctrl_number == 128) {
#if 0
               u16 pb_value = (value == 0x40) ? 0x2000 : ((value << 7) | value);se
#else
               // allow some headroom for the middle position
               u16 pb_value = (value >= 0x3f && value <= 0x41) ? 0x2000 : ((value << 7) | value);
#endif
               MIOS32_MIDI_SendPitchBend(port, midi_chn - 1, pb_value);
            }
            else if (ctrl_number == 129) {
               MIOS32_MIDI_SendAftertouch(port, midi_chn - 1, value);
            }
         }
      }
   }

   return 0; // no error
}
////////////////////////////////////////////////////////////////////////////
//! Returns the current zone preset
/////////////////////////////////////////////////////////////////////////////
zone_preset_t * KEYBOARD_GetCurrentZonePreset(){
   return &keyboard_config.current_zone_preset;
}

/////////////////////////////////////////////////////////////////////////////
//! Global helper function to put the note name into a string (buffer has 3 chars + terminator)
/////////////////////////////////////////////////////////////////////////////
char* KEYBOARD_GetNoteName(u8 note, char str[4]) {
   const char note_tab[12][3] = { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };

   // determine octave, note contains semitone number thereafter
   int octave = note / 12;
   note %= 12;

   str[0] = octave >= 2 ? (note_tab[note][0] + 'A' - 'a') : note_tab[note][0];
   str[1] = note_tab[note][1];

   if (str[1] == '-'){
      // overwrite octave here
      str[1] = '0' + (octave - 2); // 0..7
      str[2] = '\0';
   }
   else{
      // keep sharp and put octave at end
      str[2] = '0' + (octave - 2); // 0..7
      str[3] = '\0';
   }

   return str;
}

/////////////////////////////////////////////////////////////////////////////
//! help function which parses a decimal or hex value
//! \retval >= 0 if value is valid
//! \retval -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char* word) {
   if (word == NULL)
      return -1;

   char* next;
   long l = strtol(word, &next, 0);

   if (word == next)
      return -1;

   return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses for on or off
//! \retval 0 if 'off'
//! \retval 1 if 'on'
//! \retval -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char* word) {
   if (strcmp(word, "on") == 0 || strcmp(word, "1") == 0)
      return 1;

   if (strcmp(word, "off") == 0 || strcmp(word, "0") == 0)
      return 0;

   return -1;
}

/////////////////////////////////////////////////////////////////////////////
//! help function which returns the current calibration mode
/////////////////////////////////////////////////////////////////////////////
static s32 KEYBOARD_TerminalCaliMode(void* _output_function) {
   void (*out)(char* format, ...) = _output_function;

   switch (ain_cali_mode_pin) {
   case 1: out("AIN Calibration Mode enabled for kb 1 pitchwheel"); break;
   case 2: out("AIN Calibration Mode enabled for kb 1 modwheel"); break;
   case 3: out("AIN Calibration Mode enabled for kb 1 sustain"); break;
   case 4: out("AIN Calibration Mode enabled for kb 1 expression"); break;
   case 5: out("AIN Calibration Mode enabled for kb 2 pitchwheel"); break;
   case 6: out("AIN Calibration Mode enabled for kb 2 modwheel"); break;
   case 7: out("AIN Calibration Mode enabled for kb 2 sustain"); break;
   case 8: out("AIN Calibration Mode enabled for kb 2 expression"); break;
   default:
      out("AIN Calibration Mode disabled.");
   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalHelp(void* _output_function) {
   void (*out)(char* format, ...) = _output_function;

   out("  keyboard or kb:     print current configuration");
   out("  set debug <on|off>:      enables/disables debug mode (not stored in EEPROM)");

   out("  set midi_ports <ports>:  selects the MIDI ports (values: see documentation)");
   out("  set midi_chn <0-16>:     selects the MIDI channel (0=off)");
#
   out("  set note_offset <0-127>: selects the note offset (transpose)");
   out("  set rows <0-%d>:         how many rows should be scanned? (0=off)", MATRIX_NUM_ROWS);
   out("  set velocity <on|off>:   keyboard supports break and make contacts");
   out("  set release_velocity <on|off>: keyboard supports NoteOff velocity");
   out("  set optimized <on|off>:        make contacts only scanned if break contacts activated");
   out("  set dout_sr1 <0-%d>:            selects first DOUT shift register (0=off)", MIOS32_SRIO_ScanNumGet());
   out("  set dout_sr2 <0-%d>:            selects second DOUT shift register (0=off)", MIOS32_SRIO_ScanNumGet());
   out("  set din_sr1 <0-%d>:             selects first DIN shift register (0=off)", MIOS32_SRIO_ScanNumGet());
   out("  set din_sr2 <0-%d>:             selects second DIN shift register (0=off)", MIOS32_SRIO_ScanNumGet());
   out("  set din_key_offset <0-127>:    selects the key offset between DIN1 and DIN2");
   out("  set din_inverted <on|off>:     DINs inverted?");
   out("  set break_inverted <on|off>:   Only break contacts inverted?");
   out("  set make_debounced <on|off>:   Make contacts will be debounced");
   out("  set break_is_make <on|off>:    Break contact will act like Make and trigger a note");
   out("  set delay_fastest <0-65535>:   fastest delay for velocity calculation");
   out("  set delay_fastest_black_keys <0-65535>: optional fastest delay for black keys");
   out("  set delay_fastest_release <0-65535>: opt. fastest release delay for velocity calculation");
   out("  set delay_fastest_release_black_keys <0-65535>: opt.fastest release delay for black keys");
   out("  set delay_slowest <0-65535>:   slowest delay for velocity calculation");
   out("  set delay_slowest_release <0-65535>: slowest release delay for velocity calculation");
   out("  set split_mode: 0=Off, 1=Two split mode, 2=Three split mode");
   out("  set split2_middle: MIDI note number for two-split center from 1 to 126");
   out("  set split2_right_shift: note offset +/- for split two right side");
   out("  set split3_left: MIDI note number for three-split left from 1 to 126");
   out("  set split3_left_shift: note offset +/- for split three left side");
   out("  set split3_right: MIDI note number for three-split right from 1 to 126");
   out("  set split3_right_shift: note offset +/- for split three right side");

   out("  set ain_pitchwheel <0..7/128..135> or off: assigns pitchwheel to given analog pin");
   out("  set ctrl_pitchwheel <0-129>:               assigns CC/PB(=128)/AT(=129) to PitchWheel");
   out("  set ain_pitchwheel_inverted <on|off>:      inverts the pitchwheel controller");
   out("  set ain_modwheel <0..7/128..135> or off:   assigns ModWheel to given analog pin");
   out("  set ctrl_modwheel <0-129>:                 assigns CC/PB(=128)/AT(=129) to ModWheel");
   out("  set ain_modwheel_inverted <on|off>:        inverts the modwheel controller");
   out("  set ain_expression <0..7/128..135> or off: assigns Expression Pedal to given analog pin");
   out("  set ctrl_expression <0-129>:               assigns CC/PB(=128)/AT(=129) to Expression");
   out("  set ain_expression_inverted <on|off>:      inverts the expression controller");
   out("  set ain_sustain <0..7/128..135> or off:    assigns Sustain Pedal to given analog pin");
   out("  set ctrl_sustain <0-129>:                  assigns CC/PB(=128)/AT(=129) to Sustain Pedal");
   out("  set ain_sustain_inverted <on|off>:         inverts the sustain controller");
   out("  set ain_sustain_switch <on|off>:      set to on if the pedal should behave like a switch");
   out("  set ain_bandwidth_ms <delay>:         defines the bandwidth of AIN scans in milliseconds");
   out("  set ain_calibration <off|pitchwheel|modwheel|expression|sustain>: starts AIN calibration");
   out("  set key_calibration <on|off>               enables/disables key calibration");
   out("  set key_calibration clean                  clears calibration data");
   out("  set key_calibration_value <key> <delay>    directly sets delay value");

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Parser for a complete line
//! \return > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalParseLine(char* input, void* _output_function) {
   void (*out)(char* format, ...) = _output_function;
   char* separators = " \t";
   char* brkt;
   char* parameter;

   // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
   // on an unsuccessful call (whenever this function returns < 1)
   u8 input_line_parsed = 1;
   int input_len = strlen(input);

   if (!(parameter = strtok_r(input, separators, &brkt))) {
      input_line_parsed = 0; // input line has to be restored
   }
   else {
      if (strcmp(parameter, "kb") == 0 || strcmp(parameter, "keyboard") == 0) {

         if ((parameter = strtok_r(NULL, separators, &brkt))) {
            if (strcmp(parameter, "delays") == 0) {
               KEYBOARD_TerminalPrintDelays(_output_function);
            }
            else {
               out("Unknown command after %s!", parameter);
            }
            return 1; // command taken
         }

         KEYBOARD_TerminalPrintConfig(_output_function);
      }
      else if (strcmp(parameter, "set") == 0) {
         if (!(parameter = strtok_r(NULL, separators, &brkt))) {
            out("Missing parameter after 'set'!");
            return 1; // command taken
         }

         if (strcmp(parameter, "kb") != 0 && strcmp(parameter, "keyboard") != 0) {
            input_line_parsed = 0; // input line has to be restored
         }
         else {
            keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

            if (!(parameter = strtok_r(NULL, separators, &brkt))) {
               out("Missing parameter name and value after 'set '!");
               return 1; // command taken
            }

            /////////////////////////////////////////////////////////////////////
            if (strcmp(parameter, "note_offset") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the Note offset!");
                  return 1; // command taken
               }

               int offset = get_dec(parameter);

               if (offset < 0 || offset > 127) {
                  out("Note Offset should be in the range between 0 and 127!");
                  return 1; // command taken
               }
               else {
                  kc->note_offset = offset;
                  out("Keyboard #%d: Note Offset %d", kc->note_offset);
               }

            }
            else if (strcmp(parameter, "din_key_offset") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the key offset!");
                  return 1; // command taken
               }

               int offset = get_dec(parameter);

               if (offset < 0 || offset > 127) {
                  out("Key Offset should be in the range between 0 and 127!");
                  return 1; // command taken
               }
               else {
                  kc->din_key_offset = offset;
                  out("Keyboard #%d: DIN Key Offset %d", kc->din_key_offset);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "debug") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->verbose_level = on_off ? 2 : 1;

                  out("Keyboard #%d: debug mode %s", on_off ? "enabled" : "disabled");
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "rows") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the number of rows (0..%d)", MATRIX_NUM_ROWS);
                  return 1; // command taken
               }

               int rows = get_dec(parameter);

               if (rows < 0 || rows > MATRIX_NUM_ROWS) {
                  out("Rows should be in the range between 0 (off) and %d", MATRIX_NUM_ROWS);
                  return 1; // command taken
               }
               else {
                  kc->num_rows = rows;
                  out("Keyboard #%d: %d rows will be scanned!", kc->num_rows);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "dout_sr1") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the DOUT number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }

               int sr = get_dec(parameter);

               if (sr < 0 || sr > MATRIX_NUM_ROWS) {
                  out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }
               else {
                  kc->dout_sr1 = sr;
                  out("Keyboard #%d: dout_sr1 assigned to %d!", kc->dout_sr1);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "dout_sr2") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the DOUT number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }

               int sr = get_dec(parameter);

               if (sr < 0 || sr > MATRIX_NUM_ROWS) {
                  out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }
               else {
                  kc->dout_sr2 = sr;
                  out("Keyboard #%d: dout_sr2 assigned to %d!", kc->dout_sr2);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "din_sr1") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the DIN number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }

               int sr = get_dec(parameter);

               if (sr < 0 || sr > MATRIX_NUM_ROWS) {
                  out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }
               else {
                  kc->din_sr1 = sr;
                  out("Keyboard #%d: din_sr1 assigned to %d!", kc->din_sr1);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "din_sr2") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the DIN number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }

               int sr = get_dec(parameter);

               if (sr < 0 || sr > MATRIX_NUM_ROWS) {
                  out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
                  return 1; // command taken
               }
               else {
                  kc->din_sr2 = sr;
                  out("Keyboard #%d: din_sr2 assigned to %d!", kc->din_sr2);
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "velocity") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->scan_velocity = on_off;

                  out("Keyboard #%d: velocity mode %s", on_off ? "enabled" : "disabled");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "release_velocity") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->scan_release_velocity = on_off;

                  out("Keyboard #%d: release velocity mode %s", on_off ? "enabled" : "disabled");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "optimized") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->scan_optimized = on_off;

                  out("Keyboard #%d: optimized scan %s", on_off ? "enabled" : "disabled");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "din_inverted") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->din_inverted = on_off;

                  out("Keyboard #%d: DIN values are %sinverted", kc->din_inverted ? "" : "not ");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "break_inverted") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->break_inverted = on_off;

                  out("Keyboard #%d: Break contacts are %sinverted", kc->break_inverted ? "" : "not ");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "make_debounced") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off (alternatively 1 or 0)");
                  return 1; // command taken
               }

               int on_off = get_on_off(parameter);

               if (on_off < 0) {
                  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
               }
               else {
                  kc->make_debounced = on_off;

                  out("Keyboard #%d: Make contact debouncing %s", kc->make_debounced ? "on" : "off");
                  KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_fastest") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the fastest delay for velocity calculation!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_fastest = delay;
                  out("Keyboard #%d: delay_fastest set to %d!", kc->delay_fastest);
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_fastest_black_keys") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the fastest delay for the black keys!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_fastest_black_keys = delay;
                  out("Keyboard #%d: delay_fastest_black_keys set to %d!", kc->delay_fastest_black_keys);
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_fastest_release") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the fastest delay for release velocity calculation!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_fastest_release = delay;
                  out("Keyboard #%d: delay_fastest_release set to %d!", kc->delay_fastest_release);
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_fastest_release_black_keys") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the fastest delay for release of black keys!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_fastest_release_black_keys = delay;
                  out("Keyboard #%d: delay_fastest_release_black_keys set to %d!", kc->delay_fastest_release_black_keys);
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_slowest") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the slowest delay for velocity calculation!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_slowest = delay;
                  out("Keyboard #%d: delay_slowest set to %d!", kc->delay_slowest);
               }
               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "delay_slowest_release") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the slowest release delay for velocity calculation!");
                  return 1; // command taken
               }

               int delay = get_dec(parameter);

               if (delay < 0 || delay > 65535) {
                  out("Delay should be in the range between 0 and 65535");
                  return 1; // command taken
               }
               else {
                  kc->delay_slowest_release = delay;
                  out("Keyboard #%d: delay_slowest_release set to %d!", kc->delay_slowest_release);
               }
            }
            else if (strcmp(parameter, "key_calibration_value") == 0) {
               int key;
               int value;

               if (!(parameter = strtok_r(NULL, separators, &brkt)) ||
                  ((key = get_dec(parameter)) < 0 || key >= KEYBOARD_MAX_KEYS)) {
                  out("Invalid <key> value, expect 0..%d!", KEYBOARD_MAX_KEYS - 1);
                  return 1; // command taken
               }

               if (!(parameter = strtok_r(NULL, separators, &brkt)) ||
                  ((value = get_dec(parameter)) < 0 || value >= 65535)) {
                  out("Invalid <delay> value, expect 0..65535!");
                  return 1; // command taken
               }

               kc->delay_key[key] = value;
               out("Delay of key #%d set to %d", key, value);

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "key_calibration") == 0 || strcmp(parameter, "key_calibrate") == 0) {
               int value;
               int clean = -1;
               if (!(parameter = strtok_r(NULL, separators, &brkt)) ||
                  ((value = get_on_off(parameter)) < 0 && (clean = strcmp(parameter, "clean")) != 0)) {
                  out("Please specify on, off or clean!");
                  return 1; // command taken
               }

               if (clean == 0) { // matching string
                  int i;

                  for (i = 0; i < KEYBOARD_MAX_KEYS; ++i) {
                     kc->delay_key[i] = 0;
                  }

                  out("Cleaned calibration data.");
               }
               else {
                  kc->key_calibration = value;

                  if (kc->key_calibration) {
                     out("Key calibration enabled.");
                     out("Press all keys with slowest velocity now.");
                     out("Enter 'set key_calibration clean' to clean previous data");
                     out("Enter 'set key_calibration off' to finish calibration");
                     out("Enter 'delays' to display current measurement results");
                  }
                  else {
                     out("Key calibration disabled.");
                     out("Enter 'delays' to display measured delays.");
                  }
               }
               return 1; // command taken

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "ain_pitchwheel") == 0 ||
               strcmp(parameter, "ain_modwheel") == 0 ||
               strcmp(parameter, "ain_expression") == 0 ||
               strcmp(parameter, "ain_sustain") == 0) {
               u8 pitchwheel = strcmp(parameter, "ain_pitchwheel") == 0;
               u8 modwheel = strcmp(parameter, "ain_modwheel") == 0;
               u8 expression = strcmp(parameter, "ain_expression") == 0;

               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify J5.Ax number (0..7), AINSER number (128..135) or off!");
                  return 1; // command taken
               }

               int ain = 0;
               if (strcmp(parameter, "off") != 0) {
                  ain = get_dec(parameter);

                  if (ain < 0 || ain > 255) {
                     out("AIN pin should be in the range of 0..255");
                     return 1; // command taken
                  }
                  ain += 1;
               }

               char wheel_name[20];
               if (pitchwheel) {
                  strcpy(wheel_name, "PitchWheel");
                  kc->ain_pin[KEYBOARD_AIN_PITCHWHEEL] = ain;
               }
               else if (modwheel) {
                  strcpy(wheel_name, "ModWheel");
                  kc->ain_pin[KEYBOARD_AIN_MODWHEEL] = ain;
               }
               else if (expression) {
                  strcpy(wheel_name, "Expression");
                  kc->ain_pin[KEYBOARD_AIN_EXPRESSION] = ain;
               }
               else {
                  strcpy(wheel_name, "Sustain Pedal");
                  kc->ain_pin[KEYBOARD_AIN_SUSTAIN] = ain;
               }

               if (ain) {
                  if (ain >= 128) {
                     out("Keyboard #%d: %s assigned to AINSER pin A%d!", wheel_name, ain - 1 - 128);
                  }
                  else {
                     out("Keyboard #%d: %s assigned to J5.A%d!", wheel_name, ain - 1);
                  }
               }
               else {
                  out("Keyboard #%d: %s disabled!", wheel_name);
               }

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "ctrl_pitchwheel") == 0 ||
               strcmp(parameter, "ctrl_modwheel") == 0 ||
               strcmp(parameter, "ctrl_expression") == 0 ||
               strcmp(parameter, "ctrl_sustain") == 0) {
               u8 pitchwheel = strcmp(parameter, "ctrl_pitchwheel") == 0;
               u8 modwheel = strcmp(parameter, "ctrl_modwheel") == 0;
               u8 expression = strcmp(parameter, "ctrl_expression") == 0;

               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the CC number (or 128 for PitchBend or 129 for Aftertouch)!");
                  return 1; // command taken
               }

               int ctrl = get_dec(parameter);

               if (ctrl < 0 || ctrl > 129) {
                  out("Controller Number should be in the range between 0 and 129!");
                  return 1; // command taken
               }
               else {
                  char wheel_name[20];
                  if (pitchwheel) {
                     strcpy(wheel_name, "PitchWheel");
                     kc->ain_ctrl[KEYBOARD_AIN_PITCHWHEEL] = ctrl;
                  }
                  else if (modwheel) {
                     strcpy(wheel_name, "ModWheel");
                     kc->ain_ctrl[KEYBOARD_AIN_MODWHEEL] = ctrl;
                  }
                  else if (expression) {
                     strcpy(wheel_name, "Expression");
                     kc->ain_ctrl[KEYBOARD_AIN_EXPRESSION] = ctrl;
                  }
                  else {
                     strcpy(wheel_name, "Sustain Pedal");
                     kc->ain_ctrl[KEYBOARD_AIN_SUSTAIN] = ctrl;
                  }

                  if (ctrl < 128)
                     out("Keyboard #%d: %s sends CC#%d", wheel_name, ctrl);
                  else if (ctrl == 128)
                     out("Keyboard #%d: %s sends PitchBend", wheel_name);
                  else
                     out("Keyboard #%d: %s sends Aftertouch", wheel_name);
               }

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "ain_pitchwheel_inverted") == 0 ||
               strcmp(parameter, "ain_modwheel_inverted") == 0 ||
               strcmp(parameter, "ain_expression_inverted") == 0 ||
               strcmp(parameter, "ain_sustain_inverted") == 0) {
               u8 pitchwheel = strcmp(parameter, "ain_pitchwheel_inverted") == 0;
               u8 modwheel = strcmp(parameter, "ain_modwheel_inverted") == 0;
               u8 expression = strcmp(parameter, "ain_expression_inverted") == 0;

               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off!");
                  return 1; // command taken
               }

               int value;
               if ((value = get_on_off(parameter)) < 0) {
                  out("Invalid value, please specify on or off!");
                  return 1; // command taken
               }

               char wheel_name[20];
               if (pitchwheel) {
                  strcpy(wheel_name, "PitchWheel");
                  kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] = value;
               }
               else if (modwheel) {
                  strcpy(wheel_name, "ModWheel");
                  kc->ain_inverted[KEYBOARD_AIN_MODWHEEL] = value;
               }
               else if (expression) {
                  strcpy(wheel_name, "Expression");
                  kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] = value;
               }
               else {
                  strcpy(wheel_name, "Sustain Pedal");
                  kc->ain_inverted[KEYBOARD_AIN_SUSTAIN] = value;
               }

               out("Keyboard #%d: %s controller inversion %s!", wheel_name, value ? "on" : "off");

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "calibration") == 0 || strcmp(parameter, "calibrate") == 0 ||
               strcmp(parameter, "ain_calibration") == 0 || strcmp(parameter, "ain_calibrate") == 0) {

               int pin = -1;
               u8 invalid_mode = 0;
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  invalid_mode = 1;
               }
               else {
                  if (strcmp(parameter, "off") == 0)
                     pin = -1;
                  else if (strcmp(parameter, "pitchwheel") == 0)
                     pin = KEYBOARD_AIN_PITCHWHEEL;
                  else if (strcmp(parameter, "modwheel") == 0)
                     pin = KEYBOARD_AIN_MODWHEEL;
                  else if (strcmp(parameter, "expression") == 0)
                     pin = KEYBOARD_AIN_EXPRESSION;
                  else if (strcmp(parameter, "sustain") == 0)
                     pin = KEYBOARD_AIN_SUSTAIN;
                  else
                     invalid_mode = 1;
               }

               if (invalid_mode) {
                  out("Please specify off, pitchwheel, modwheel, expression or sustain to disable/enable calibration mode!");
                  return 1; // command taken
               }

               if (pin < 0) {
                  ain_cali_mode_pin = 0; // off
                  KEYBOARD_TerminalCaliMode(_output_function);
               }
               else {
                  ain_cali_mode_pin = 1 + pin;
                  KEYBOARD_TerminalCaliMode(_output_function);

                  kc->ain_min[pin] = 0xff;
                  kc->ain_max[pin] = 0x00;
                  out("Please move the potentiomenter into both directions now!");
                  out("The calibration will be finished by selection a new source, or with 'set calibration off'");
                  out("Enter 'store' to save the calibration values");
               }

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "ain_bandwidth_ms") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify the AIN bandwidth in milliseconds!");
                  return 1; // command taken
               }

               int bandwidth_ms = get_dec(parameter);

               if (bandwidth_ms < 0 || bandwidth_ms > 255) {
                  out("Bandwidth delay should be in the range between 0..255");
                  return 1; // command taken
               }
               else {
                  kc->ain_bandwidth_ms = bandwidth_ms;
                  out("Keyboard #%d: ain_bandwidth_ms set to %d!", kc->ain_bandwidth_ms);
               }

               /////////////////////////////////////////////////////////////////////
            }
            else if (strcmp(parameter, "ain_sustain_switch") == 0) {
               if (!(parameter = strtok_r(NULL, separators, &brkt))) {
                  out("Please specify on or off!");
                  return 1; // command taken
               }

               int value;
               if ((value = get_on_off(parameter)) < 0) {
                  out("Please specify on or off!");
                  return 1; // command taken
               }

               kc->ain_sustain_switch = value;

               out("Sustain controller behaves like a %s", kc->ain_sustain_switch ? "switch" : "pot");

               /////////////////////////////////////////////////////////////////////
            }
            else {
               out("Unknown parameter for keyboard configuration - type 'help' to list available parameters!");
               return 1; // command taken
            }
         }
      }
      else {
         // out("Unknown command - type 'help' to list available commands!");
         input_line_parsed = 0; // input line has to be restored
      }
   }

   if (!input_line_parsed) {
      // restore input line (replace NUL characters by spaces)
      int i;
      char* input_ptr = input;
      for (i = 0; i < input_len; ++i, ++input_ptr)
         if (!*input_ptr)
            *input_ptr = ' ';

      return 0; // command not taken
   }

   return 1; // command taken
}


/////////////////////////////////////////////////////////////////////////////
//! Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalPrintConfig(void* _output_function) {
   void (*out)(char* format, ...) = _output_function;

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   out("debug %s", (kc->verbose_level >= 2) ? "on" : "off");

   out("note_offset %d", kc->note_offset);
   out("rows %d", kc->num_rows);
   out("velocity %s", kc->scan_velocity ? "on" : "off");
   out("release_velocity %s", kc->scan_release_velocity ? "on" : "off");
   out("optimized %s", kc->scan_optimized ? "on" : "off");
   out("dout_sr1 %d", kc->dout_sr1);
   out("dout_sr2 %d", kc->dout_sr2);
   out("din_sr1 %d", kc->din_sr1);
   out("din_sr2 %d", kc->din_sr2);
   out("din_key_offset %d", kc->din_key_offset);
   out("din_inverted %s", kc->din_inverted ? "on" : "off");
   out("break_inverted %s", kc->break_inverted ? "on" : "off");
   out("make_debounced %s", kc->make_debounced ? "on" : "off");
   out("delay_fastest %d", kc->delay_fastest);
   out("delay_fastest_black_keys %d", kc->delay_fastest_black_keys);
   out("delay_fastest_release %d", kc->delay_fastest_release);
   out("delay_fastest_release_black_keys %d", kc->delay_fastest_release_black_keys);
   out("delay_slowest %d", kc->delay_slowest);
   out("delay_slowest_release %d", kc->delay_slowest_release);

   if (kc->ain_pin[KEYBOARD_AIN_PITCHWHEEL])
      out("ain_pitchwheel %d", kc->ain_pin[KEYBOARD_AIN_PITCHWHEEL] - 1);
   else
      out("ain_pitchwheel off");
   out("ctrl_pitchwheel %d (%s)", kc->ain_ctrl[KEYBOARD_AIN_PITCHWHEEL],
      (kc->ain_ctrl[KEYBOARD_AIN_PITCHWHEEL] < 128) ? "CC" : (kc->ain_ctrl[KEYBOARD_AIN_PITCHWHEEL] == 128 ? "PitchBend" : "Aftertouch"));
   out("ain_pitchwheel_inverted %s", kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] ? "on" : "off");

   if (kc->ain_pin[KEYBOARD_AIN_MODWHEEL])
      out("ain_modwheel %d", kc->ain_pin[KEYBOARD_AIN_MODWHEEL] - 1);
   else
      out("ain_modwheel off");
   out("ctrl_modwheel %d (%s)", kc->ain_ctrl[KEYBOARD_AIN_MODWHEEL],
      (kc->ain_ctrl[KEYBOARD_AIN_MODWHEEL] < 128) ? "CC" : (kc->ain_ctrl[KEYBOARD_AIN_MODWHEEL] == 128 ? "PitchBend" : "Aftertouch"));
   out("ain_modwheel_inverted %s", kc->ain_inverted[KEYBOARD_AIN_MODWHEEL] ? "on" : "off");

   if (kc->ain_pin[KEYBOARD_AIN_EXPRESSION])
      out("ain_expression %d", kc->ain_pin[KEYBOARD_AIN_EXPRESSION] - 1);
   else
      out("ain_expression off");
   out("ctrl_expression %d (%s)", kc->ain_ctrl[KEYBOARD_AIN_EXPRESSION],
      (kc->ain_ctrl[KEYBOARD_AIN_EXPRESSION] < 128) ? "CC" : (kc->ain_ctrl[KEYBOARD_AIN_EXPRESSION] == 128 ? "PitchBend" : "Aftertouch"));
   out("ain_expression_inverted %s", kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] ? "on" : "off");

   if (kc->ain_pin[KEYBOARD_AIN_SUSTAIN])
      out("ain_sustain %d", kc->ain_pin[KEYBOARD_AIN_SUSTAIN] - 1);
   else
      out("ain_sustain off");
   out("ctrl_sustain %d (%s)", kc->ain_ctrl[KEYBOARD_AIN_SUSTAIN],
      (kc->ain_ctrl[KEYBOARD_AIN_SUSTAIN] < 128) ? "CC" : (kc->ain_ctrl[KEYBOARD_AIN_SUSTAIN] == 128 ? "PitchBend" : "Aftertouch"));
   out("ain_sustain_inverted %s", kc->ain_inverted[KEYBOARD_AIN_SUSTAIN] ? "on" : "off");

   out("ain_sustain_switch %s", kc->ain_sustain_switch ? "on" : "off");
   out("ain_bandwidth_ms %d", kc->ain_bandwidth_ms);

   KEYBOARD_TerminalCaliMode(_output_function);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalPrintDelays(void* _output_function) {
   void (*out)(char* format, ...) = _output_function;


   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config;

   int last_key;
   for (last_key = 127; last_key >= 0; --last_key) {
      if (kc->delay_key[last_key] > 0)
         break;
   }

   if (last_key < 0) {
      out("No delays measured yet; please enable key_calibration and press the keys");
   }
   else {
      int i;
      for (i = 0; i <= last_key; ++i) {
         out("Key#%3d: %d\n", i, kc->delay_key[i]);
      }
   }

   return 0; // no error
}


//! \}
