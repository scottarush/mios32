/ $Id$
//! \defgroup PEDALS
//!
//! M3 Pedal Handler
//!
//! \{
/*
 * ==========================================================================
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

#include "pedals.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////
pedal_config_t pedal_config;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialize the pedal handler
/////////////////////////////////////////////////////////////////////////////
void PEDALS_Init(){
    pedal_config_t* pc = (pedal_config_t *)&pedal_config;

    pc->midi_ports = 0x1011;  
    pc->midi_chn = 1;

    pc->num_pedals = 12;

    pc->verbose_level = 0;

    pc->delay_fastest = 100;
    pc->delay_fastest_black_keys = 120;
    pc->delay_slowest = 500;

    pc->octave = 0;
    pc->transpose = 0;
    pc->left_pedal_note_number = 24;
}

/////////////////////////////////////////////////////////////////////////////
// Called from application upon any pedal change
// @param pedalNum:  0=left most pedal, make switch=num_pedals;
// @param value: 0=pressed, 1=released
// @param timestamp:  event timestamp
/////////////////////////////////////////////////////////////////////////////
void PEDALS_NotifyChange(u8 pedalNum,u8 value,u32 timestamp){
    pedal_config_t* pc = (pedal_config_t *)&pedal_config;

    if ((pedalNum < 0) || (pedalNum > pc->num_pedals){
        DEBUG_MSG("Invalid pedalNum");
    }
    if (pedalNum == pc->num_pedals){
        // This is the make switch.  Do make processing
        // TODO:  Continue from here

        return;
    }
    // Otherwise, this is a pedal event
    u8 velocity = 127;
    // Compute the midi note_number
    u8 note_number = pc->octave*12 + pc->left_pedal_note_number + pc->transpose;

    if (value == 1){
        // This is a release.  Just send note off event.
        PEDALS_SendNote(note_number,velocity,1);
        return;
    }
    // Otherwise, this is a press.
    // TODO.
}

/////////////////////////////////////////////////////////////////////////////
//! Help function to send a MIDI note over given ports
/////////////////////////////////////////////////////////////////////////////
static s32 PEDALS_SendNote(u8 note_number, u8 velocity, u8 released) {
    pedal_config_t* pc = (pedal_config_t *)&pedal_config;

    u8 sent_note = note_number;

	int i;
	u16 mask = 1;
	for (i = 0; i < 16; ++i, mask <<= 1) {
		if (pc->midi_ports & mask) {
			// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
			mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);

			if (released)
				MIOS32_MIDI_SendNoteOff(port, midi_chn - 1, sent_note, velocity);
			else
				MIOS32_MIDI_SendNoteOn(port, midi_chn - 1, sent_note, velocity);
		}
    }

	return 0; // no error
}
#endif
