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
		  src/switches.c \
		  src/keyboard_presets.c \
		  src/terminal.c \
		  src/shell.c \
 		  src/keyboard.c \
		  src/velocity.c\
		  src/hmi.c \
		  src/persist.c
		  
# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE =     -I src
A_INCLUDE =     -I src

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	studio90_plus_mbkb

# Can't force a project output directory to work.   
#PROJECT_OUT = ~/midibox/build_output/$(PROJECT)

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

# KEYBOARD driver 
#include $(MIOS32_PATH)/modules/keyboard/keyboard.mk

# AINSER driver
include $(MIOS32_PATH)/modules/ainser/ainser.mk

# MIDI Router (and port handling)
include $(MIOS32_PATH)/modules/midi_router/midi_router.mk

# MIDImo
include $(MIOS32_PATH)/modules/midimon/midimon.mk

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# UIP Standard Task (with DHCPC + OSC server and client)
include $(MIOS32_PATH)/modules/uip_task_standard/uip_task_standard.mk

# EEPROM emulation
include $(MIOS32_PATH)/modules/eeprom/eeprom.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
