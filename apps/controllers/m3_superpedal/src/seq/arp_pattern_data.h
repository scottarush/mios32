/*
 * Header for Arpeggiator pattern data
 *
 * ==========================================================================
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _ARP_PAT_DATA_H
#define _ARP_PAT_DATA_H

#include "arp_pattern.h"

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
extern const arp_pattern_t patterns[NUM_PATTERNS];
extern const char * patternNames[];
extern const char * patternShortNames[];
#endif /* _ARP_PAT_DATA_H */
