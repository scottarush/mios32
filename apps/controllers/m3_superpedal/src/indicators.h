/*
 * include for M3 super pedal  LED indicators
 *
 *  Copyright (C) 2024 Scott Rush
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 */

#ifndef _IND_H
#define _IND_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
#define NUM_LED_INDICATORS 8
#define IND_TEMP_FLASH_STATE_DEFAULT_DURATION 450

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
   IND_OFF = 0,
   IND_ON = 1,
   IND_ON_LOW = 2,
   IND_FLASH_SLOW = 3,
   IND_FLASH_BLIP = 4,
   IND_FLASH_INVERSE_BLIP = 5,
   IND_FLASH_FAST = 6
} indicator_state_t;

typedef enum indicator_ramp_e {
   IND_RAMP_NONE = 0,
   IND_RAMP_UP = 1,
   IND_RAMP_DOWN = 2,
   IND_RAMP_UP_DOWN = 3
} indicator_ramp_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern void IND_Init(void);

extern void IND_1msTick();

extern void IND_SetIndicatorState(u8 indicatorNum, indicator_state_t state, u8 brightness,indicator_ramp_t rampMode);
extern void IND_SetBlipIndicator(u8 indicatorNum,u8 inverse,float frequency,u8 brightness);
extern void IND_SetFlashIndicator(u8 indicatorNum,float frequency,u8 brightness);

extern void IND_SetTempIndicatorState(u8 indicatorNum, indicator_state_t tempState, u16 duration_ms, indicator_state_t targetState,u8 brightness);

extern void IND_ClearAll();

extern void IND_FlashAll(u8 flashFast);

extern indicator_state_t IND_GetIndicatorState(u8 indicatorNum);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _IND_H */
