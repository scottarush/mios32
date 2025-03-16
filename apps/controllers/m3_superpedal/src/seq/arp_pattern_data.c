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
// Pattern Definitions.  Each step is a 16th note.
const arp_pattern_t patterns[NUM_PATTERNS] = {
   //Ascending
   {4,{{STEP_TYPE_NORM,1,0,0},{STEP_TYPE_NORM,2,0,0},{STEP_TYPE_NORM,3,0,0},{STEP_TYPE_NORM,4,0,0}}},  

   // Descending
   {4,{{STEP_TYPE_NORM,4,0,0},{STEP_TYPE_NORM,3,0,0},{STEP_TYPE_NORM,2,0,0},{STEP_TYPE_NORM,1,0,0}}},   

   // Ascend & Descend
   {8,{{STEP_TYPE_NORM,1,0,0},{STEP_TYPE_NORM,2,0,0},{STEP_TYPE_NORM,3,0,0},{STEP_TYPE_NORM,4,0,0},
      {STEP_TYPE_NORM,1,1,0},{STEP_TYPE_NORM,3,0,0},{STEP_TYPE_NORM,2,0,0},{STEP_TYPE_NORM,1,0,0}}},   

   // Chord 1/4 Notes
   {4,{{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_REST,0,0,0},{STEP_TYPE_OFF,0,0,0},{STEP_TYPE_OFF,0,0,0}}},
   
   // Chord 1/8 Notes
   {4,{{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_OFF,0,0,0},{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_OFF,0,0,0}}},

   // Chord 1/16 Notes
   {4,{{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_CHORD,0,0,0},{STEP_TYPE_CHORD,0,0,0}}},

};

const char * patternNames[] = {
   "Ascending Octaves",
   "Descending Octaves",
   "Asc & Desc Octaves",
   "Chorded 1/4 Notes",
   "Chorded 1/8 Notes",
   "Chorded 1/16 Notes",
};
const char * patternShortNames[] = {
   "Asc ",
   "Desc",
   "AscDesc",
   "Chrd 4",
   "Chrd 8",
   "Chrd 16",
};
