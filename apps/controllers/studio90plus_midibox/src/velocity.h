/*
 * Header file for velocity curve handling.
 *
 */

#ifndef _VELOCITY_H
#define _VELOCITY_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
typedef enum velocity_curve_e {
   VELOCITY_CURVE_LINEAR = 0,
   VELOCITY_CURVE_BALANCED = 1,
   VELOCITY_CURVE_BASS_BOOST = 2
} velocity_curve_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern int VELOCITY_LookupVelocity(int velocity, velocity_curve_t curve);

extern const char* VELOCITY_GetVelocityCurveName(velocity_curve_t curve);

#endif /* _VELOCITY_H */
