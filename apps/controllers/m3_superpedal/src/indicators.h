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
} indicator_states_t;

typedef enum indicator_color_e {
   IND_COLOR_RED = 0,
   IND_COLOR_GREEN = 1,
   IND_COLOR_YELLOW = 2
} indicator_color_t;

typedef enum indicator_ramp_e {
   IND_RAMP_NONE = 0,
   IND_RAMP_UP = 1,
   IND_RAMP_DOWN = 2,
   IND_RAMP_UP_DOWN = 3
} indicator_ramp_t;

typedef enum indicator_id_e {
   IND_TOE_1 = 1,
   IND_TOE_2 = 2,
   IND_TOE_3 = 3,
   IND_TOE_4 = 4,
   IND_TOE_5 = 5,
   IND_TOE_6 = 6,
   IND_TOE_7 = 7,
   IND_TOE_8 = 8,
   IND_STOMP_1 = 9,
   IND_STOMP_2 = 10,
   IND_STOMP_3 = 11,
   IND_STOMP_4 = 12,
   IND_STOMP_5 = 13,
} indicator_id_t;
#define NUM_LED_INDICATORS 13


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern void IND_Init(void);

extern void IND_1msTick();

extern void IND_SetIndicatorState(indicator_id_t indicatorNum, indicator_states_t state, u8 brightness,indicator_ramp_t rampMode);
extern void IND_SetIndicatorColor(indicator_id_t indicatorNum,indicator_color_t color);

extern void IND_SetBlipIndicator(indicator_id_t indicatorNum,u8 inverse,float frequency,u8 brightness);
extern void IND_SetFlashIndicator(indicator_id_t indicatorNum,float frequency,u8 brightness);

extern void IND_SetTempIndicatorState(indicator_id_t indicatorNum, indicator_states_t tempState, u16 duration_ms, indicator_states_t targetState,u8 brightness);

extern void IND_ClearAll();

extern void IND_FlashAll(u8 flashFast);

extern indicator_states_t IND_GetIndicatorState(indicator_id_t indicatorNum);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _IND_H */
