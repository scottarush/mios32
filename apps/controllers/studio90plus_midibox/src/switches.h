// $Id$
/*
 * include for Studio 90 Plus switches
 * ==========================================================================
 */

#ifndef _SWITCHES_H
#define _SWITCHES_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define NUM_STUDIO90_SWITCHES 13

// Set these defines to the bit numbers of the switch within the J10 port.
#define STUDIO90_SWITCH_0_J10_INDEX 0
#define STUDIO90_SWITCH_1_J10_INDEX 1
#define STUDIO90_SWITCH_2_J10_INDEX 2
#define STUDIO90_SWITCH_3_J10_INDEX 3
#define STUDIO90_SWITCH_4_J10_INDEX 4
#define STUDIO90_SWITCH_5_J10_INDEX 5
#define STUDIO90_SWITCH_6_J10_INDEX 6
#define STUDIO90_SWITCH_7_J10_INDEX 7
#define STUDIO90_SWITCH_8_J10_INDEX 8
#define STUDIO90_SWITCH_9_J10_INDEX 9
#define STUDIO90_SWITCH_10_J10_INDEX 10
#define STUDIO90_SWITCH_SELECT_J10_INDEX 11
#define STUDIO90_SWITCH_PAGE_UP_J10_INDEX 12
#define STUDIO90_SWITCH_PAGE_DOWN_J10_INDEX 13

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void SWITCHES_Init(void);
extern void SWITCHES_Read();

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TERMINAL_H */
