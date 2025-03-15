
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
// Curves are generated from this tool:
//  https://sumire-io.gitlab.io/midi-velocity-curve-generator/
// https://www.logicprohelp.com/forums/topic/124960-midi-velocity-curve-generator/
// https://gitlab.com/sumire-io/midi-velocity-curve-generator.
/////////////////////////////////////////////////////////////////////////////

#define VELOCITY_CURVE_ARRAY_LENGTH 128

const int sigmoidVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 24, 26, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 54, 56, 58, 60, 62, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95, 96, 98, 100, 101, 103, 105, 106, 107, 109, 110, 111, 113, 114, 115, 116, 117, 118, 118, 119, 120, 121, 121, 122, 122, 123, 123, 124, 124, 124, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127};

const int concaveVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = { 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 24, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 39, 40, 41, 42, 44, 45, 46, 48, 49, 50, 52, 53, 55, 57, 58, 60, 61, 63, 65, 67, 68, 70, 72, 74, 76, 78, 79, 81, 83, 85, 87, 89, 91, 93, 95, 97, 100, 102, 104, 106, 108, 110, 112, 114, 117, 119, 121, 123, 125, 127 };

const int convexVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = { 1, 4, 6, 9, 11, 14, 17, 19, 22, 24, 27, 29, 31, 34, 36, 38, 41, 43,
45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 64, 66, 68, 69, 71, 73, 74, 76, 77, 79, 80, 81, 83, 84, 85, 86, 88, 89, 90, 91, 92, 93,
94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 103, 104, 105, 106, 106, 107, 108, 108, 109, 109, 110, 111, 111, 112, 112, 113, 113,
114, 114, 115, 115, 116, 116, 116, 117, 117, 118, 118, 118, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 122, 122, 122, 122,
122, 123, 123, 123, 123, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 127, 127 };

const int saturationVelocityCurve[VELOCITY_CURVE_ARRAY_LENGTH] = {1, 4, 6, 9, 11, 14, 17, 19, 22, 24, 27, 29, 32, 34, 37, 39, 41, 44, 46, 48, 51, 53, 55, 57, 
   9, 62, 64, 66, 68, 70, 72, 74, 75, 77, 79, 81, 83, 84, 86, 88, 89, 91, 92, 94, 95, 97, 98, 100, 101, 102, 104, 105, 106, 107, 
   108, 109, 110, 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 120, 121, 122, 122, 123, 123, 124, 125, 125, 126, 126, 126, 
   127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 
   127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127};

/////////////////////////////////////////////////////////////////////////////
// Returns curved velocity.
/////////////////////////////////////////////////////////////////////////////
int VELOCITY_LookupVelocity(int velocity, velocity_curve_t curve) {
   int curvedVelocity = velocity;
   if (curve != VELOCITY_CURVE_LINEAR) {
      const int (*pCurveArray)[VELOCITY_CURVE_ARRAY_LENGTH] = NULL;
      switch (curve) {
      case VELOCITY_CURVE_SIGMOID:
         pCurveArray = &sigmoidVelocityCurve;
         break;
      case VELOCITY_CURVE_CONVEX:
         pCurveArray = &convexVelocityCurve;
         break;
      case VELOCITY_CURVE_CONCAVE:
         pCurveArray = &concaveVelocityCurve;
         break;
      default:
         DEBUG_MSG("VELOCITY_LookupVelocity: Invalid velocity curve index: %d", curve);
      }

      if (pCurveArray != NULL) {
         curvedVelocity = (*pCurveArray)[velocity];
      }
   }
   return curvedVelocity;
}

/////////////////////////////////////////////////////////////////////////////
// Returns point to velocity curve name for use in HMI
/////////////////////////////////////////////////////////////////////////////
const char* VELOCITY_GetVelocityCurveName(velocity_curve_t curve) {
   switch (curve) {
   case VELOCITY_CURVE_LINEAR:
      return "Linear";
   case VELOCITY_CURVE_CONVEX:
      return "Convex";
   case VELOCITY_CURVE_CONCAVE:
      return "Concave";
   case VELOCITY_CURVE_SIGMOID:
      return "Sigmoid";
   case VELOCITY_CURVE_SATURATION:
      return "Saturation";
   default:
      DEBUG_MSG("VELOCITY_GetVelocityCurveName: Invalid velocity curve: %d", curve);
      return "Invalid";
   }
}

/////////////////////////////////////////////////////////////////////////////
// Returns pointer to velocity abbreivation
/////////////////////////////////////////////////////////////////////////////
const char* VELOCITY_GetVelocityCurveAbbr(velocity_curve_t curve) {
   switch (curve) {
   case VELOCITY_CURVE_LINEAR:
      return "Lin";
   case VELOCITY_CURVE_CONVEX:
      return "Cvx";
   case VELOCITY_CURVE_CONCAVE:
      return "Con";
   case VELOCITY_CURVE_SIGMOID:
      return "Sig";
   case VELOCITY_CURVE_SATURATION:
      return "Sat";
   default:
      DEBUG_MSG("VELOCITY_GetVelocityCurveAbbr: Invalid velocity curve: %d", curve);
      return "Invalid";
   }

}


