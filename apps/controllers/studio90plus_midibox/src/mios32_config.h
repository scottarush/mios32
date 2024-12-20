// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// This disables the boot message to the LCD display (still goes to the terminal)
// Otherwise it overwrites the one in the call trace from APP_Init() since APP_Init is
// called in programming_models/traditional/main.c before the MIOS32 boot messages
#define MIOS32_LCD_BOOT_MSG_DELAY 0

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "Studio 90 Plus Midibox"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2024 T.Klose, S. Rush"


// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "jsrglobal.net" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "Studio90Plus MidiboxKB"   // you will see this in the MIDI device list

#define MIOS32_USB_MIDI_NUM_PORTS 1           // 1 MIDI Outs

// Don't use IIC or SPI
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_DONT_USE_SPI_MIDI

// Need two UARTS:  MO1 (UART2) and MO2 (UART3)
#define MIOS32_UART_NUM 4                 

// disables the default SRIO scan routine in programming_models/traditional/main.c
// allows to implement an own handler
// -> see app.c, , APP_SRIO_*
#define MIOS32_DONT_SERVICE_SRIO_SCAN 1

// Enables different (and working) velocity processing in keyboard.c
#define STUDIO90_PLUS_KEYBOARD_VARIANT

// EEPROM emulation
// SIZE == 2048 halfwords -> 4096 bytes
#define EEPROM_EMULATED_SIZE 2048

// AIN configuration:
// Pin mapping on MBHP_CORE_LPC17 module:
//   7        6       5      4      3      2      1       0
// J5B.A7  J5B.A6  J5B.A5 J5B.A4 J5A.A3 J5A.A2 J5A.A1  J5A.A0

// A0=PitchBend
// A1=Mod
// A2=Sustain
// A3=Expression
#define MIOS32_AIN_CHANNEL_MASK 0x000F

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 8 bit, it's set to (2^(12-8)-1) = (2^4 - 1) = 15
#define MIOS32_AIN_DEADBAND 15

// Hook the keyboard send callback into the app.
//#define KEYBOARD_NOTIFY_TOGGLE_HOOK

#endif /* _MIOS32_CONFIG_H */
