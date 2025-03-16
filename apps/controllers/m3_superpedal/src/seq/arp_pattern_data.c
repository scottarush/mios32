// $Id$
/*
 * Arpeggiator pattern data
 *
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

#include "arp_pattern_data.h"

//------------------------------------------------------------------------
// Pattern Definitions
const arp_pattern_t patterns[NUM_PATTERNS] = {
   {4,{{NORM,1,0,0},{NORM,2,0,0},{NORM,3,0,0},{NORM,4,0,0}}},  
   {4,{{NORM,4,0,0},{NORM,3,0,0},{NORM,2,0,0},{NORM,1,0,0}}},   
   {8,{{NORM,1,0,0},{NORM,2,0,0},{NORM,3,0,0},{NORM,4,0,0},{NORM,1,1,0},{NORM,3,0,0},{NORM,2,0,0},{NORM,1,0,0}}},   
   {4,{{CHORD,1,0,0},{REST,2,0,0},{REST,3,0,0},{REST,4,0,0}}},
   {4,{{CHORD,1,0,0},{REST,2,0,0},{CHORD,3,0,0},{REST,4,0,0}}}
};

const char * patternNames[] = {
   // 0..10
   "Ascending",
   "Descending",
   "Ascend & Descend",
   "Chord 1/4 Notes",
   "Chord 1/8 Notes"
};
const char * patternShortNames[] = {
   // 0..10
   "Asc ",
   "Desc",
   "AscDesc",
   "Chord 4",
   "Chord 8"
};
