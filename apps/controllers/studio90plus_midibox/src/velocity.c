
/*
 * Contains velocity processing optimized for Studio 90.
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

#include "velocity.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Curves are generated from this tool:  https://sumire-io.gitlab.io/midi-velocity-curve-generator/
/////////////////////////////////////////////////////////////////////////////

#define VELOCITY_CURVE_ARRAY_LENGTH 128

const int balancedVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 27, 28, 30, 31, 32, 34, 35,
37, 38, 40, 41, 43, 45, 46, 48, 50, 51, 53, 54, 56, 58, 59, 61, 63, 64, 66, 68, 69, 71, 73, 74, 76,
77, 79, 81, 82, 84, 85, 87, 88, 90, 91, 92, 94, 95, 97, 98, 99, 100, 102, 103, 104, 105, 106, 107,
108, 109, 110, 111, 112, 113, 114, 115, 115, 116, 117, 117, 118, 119, 119, 120, 120, 121, 121, 122,
122, 123, 123, 123, 124, 124, 125, 125, 125, 126, 126, 126, 127 };

const int bassBoostVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = { 1, 4, 7, 10, 13, 16, 19, 22, 25, 27, 30, 33, 36, 39, 41, 44,
47, 49, 52, 54, 57, 59, 62, 64, 66, 68, 71, 73, 75, 77, 79, 81, 82, 84, 86, 87, 89, 91, 92, 94, 95,
96, 98, 99, 100, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 111, 112, 113, 114, 114, 115, 116,
116, 117, 118, 118, 119, 119, 119, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 123, 124,
124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126,
126, 126, 126, 126, 126, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };


/////////////////////////////////////////////////////////////////////////////
// Returns curved velocity.
/////////////////////////////////////////////////////////////////////////////
int VELOCITY_LookupVelocity(int velocity, velocity_curve_t curve) {
   int curvedVelocity = velocity;
   if (curve != VELOCITY_CURVE_LINEAR) {
      const int (*pCurveArray)[VELOCITY_CURVE_ARRAY_LENGTH] = NULL;
      switch (curve) {
      case VELOCITY_CURVE_BALANCED:
         pCurveArray = &balancedVelocityCurve;
         break;
      case VELOCITY_CURVE_BASS_BOOST:
         pCurveArray = &bassBoostVelocityCurve;
      default:
         DEBUG_MSG("VELOCITY_LookupVelocity: Invalid velocity curve index: %d", curve);
      }

      if (pCurveArray != NULL) {
         curvedVelocity = (*pCurveArray)[velocity];
      }
   }
   DEBUG_MSG("VELOCITY_LookupVelocity:  '%s': Velocity=%d, CurvedVelocity=%d",VELOCITY_GetVelocityCurveName(curve),velocity,curvedVelocity);
   return curvedVelocity;
}

/////////////////////////////////////////////////////////////////////////////
// Returns point to velocity curve name for use in HMI
/////////////////////////////////////////////////////////////////////////////
const char* VELOCITY_GetVelocityCurveName(velocity_curve_t curve) {
   switch (curve) {
   case VELOCITY_CURVE_LINEAR:
      return "Linear";
   case VELOCITY_CURVE_BALANCED:
      return "Balanced";
   case VELOCITY_CURVE_BASS_BOOST:
      return "Bass Boost";
   default:
      DEBUG_MSG("VELOCITY_GetVelocityCurveName: Invalid velocity curve: %d", curve);
      return "Invalid";
   }

}


