// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/docs/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// Version string
#define M3_SUPERPEDAL_VERSION "Version: 1.0.1"
#define M3_SUPERPEDAL_VERSION_DATE "12AUG2024"

// This disables the boot message to the LCD display (still goes to the terminal)
// Otherwise it overwrites the one in the call trace from APP_Init() since APP_Init is
// called in programming_models/traditional/main.c before the MIOS32 boot messages
#define MIOS32_LCD_BOOT_MSG_DELAY 0

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "M3 Super Pedal"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2024 S. Rush"


// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "jsrglobal.net" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "M3 SuperPedal"   // you will see this in the MIDI device list

#define MIOS32_USB_MIDI_NUM_PORTS 1           // 1 MIDI Outs

// Number of SRIO shift registers
#define MIOS32_SRIO_NUM_SR 4

// Don't use IIC or SPI
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_DONT_USE_SPI_MIDI

// No need for AINs
#define MIOS32_DONT_USE_AIN

// Need two UARTS for MO1, MO2, and MI1
#define MIOS32_UART_NUM 2                

// EEPROM emulation
// SIZE == 2048 halfwords -> 4096 bytes
#define EEPROM_EMULATED_SIZE 2048

#endif /* _MIOS32_CONFIG_H */
