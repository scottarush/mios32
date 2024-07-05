# $Id$

################################################################################
# following setup taken from environment variables
################################################################################

PROCESSOR =	$(MIOS32_PROCESSOR)
FAMILY    = 	$(MIOS32_FAMILY)
BOARD	  = 	$(MIOS32_BOARD)
LCD       =     $(MIOS32_LCD)


################################################################################
# Source Files, include paths and libraries
################################################################################

# Scott Rush:  January 2024.  Added Studio90Plus specific keyboard driver 
# pending future pull into mainline keyboard module.
THUMB_SOURCE    = src/app.c \
 		  src/pedals.c \
		  src/indicators.c \
		  src/hmi.c \
		  src/arp_hmi.c\
		  src/midi_presets.c\
        src/persist.c\
		  src/terminal.c\
		  src/arp.c \
		  src/arp_modes.c\
		  src/seq/mid_file.c \
		  src/seq/midio_file.c \
		  src/seq/seq.c \
		  src/seq/seq_chord.c \
		  src/seq/seq_scale.c \
		  src/seq/seq_core.c \
		  src/seq/notestack.c

#		  src/shell.c \


#  (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE =     -I src
C_INCLUDE +=    -I src/seq

A_INCLUDE =     -I src

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	m3_superpedal

DEBUG     =	-g
OPTIMIZE  =	-Os

CFLAGS =	$(DEBUG) $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# MIDI Router (and port handling)
include $(MIOS32_PATH)/modules/midi_router/midi_router.mk

# MIDImo
include $(MIOS32_PATH)/modules/midimon/midimon.mk

# UIP driver and standard task aren't used (no OSC), but included because of
# dependencies from the midi_router

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# UIP Standard Task (with DHCPC + OSC server and client)
include $(MIOS32_PATH)/modules/uip_task_standard/uip_task_standard.mk

# EEPROM emulation
include $(MIOS32_PATH)/modules/eeprom/eeprom.mk

# Notestack functions for arpeggiator/sequencer
# Can't use the module for up/down arpeggiation.  See local copy in seq/notestack.c for details.
#include $(MIOS32_PATH)/modules/notestack/notestack.mk

# Use platform sequence BPM and MidiOut functions
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# FILE Access Layer
include $(MIOS32_PATH)/modules/file/file.mk

# Portable randomize module
include $(MIOS32_PATH)/modules/random/random.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
