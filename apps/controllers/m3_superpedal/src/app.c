/*
 * M3 SuperPedal
 *
 * ==========================================================================
 *  Copyright (C) 2024 Scott Rush (scottarush@yahoo.com)
 *  Licensed for personal non-commercial use only.
 *  Uses open-source software from midibox.org subject to project licensing terms.
 * ==========================================================================
 */

 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "app.h"
#include "hmi.h"
#include "pedals.h"
#include "indicators.h"
#include "presets.h"
#include "terminal.h"
#include "tasks.h"
#include "uip_task.h"
#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// define priority level for sequencer
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// Mutex for MIDI IN/OUT handler in modules/midi_router/midi_router.c
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

// local prototype of the 1ms task function
static void TASK_Period_1mS(void* pvParameters);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void) {
   // create semaphores
   xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
   xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

   // install SysEx callback
   MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

   // install MIDI Rx/Tx callback functions
   MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
   MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

   // install timeout callback function
   MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

   
   // Init the rotary encoder
   mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(0);   
   enc_config.cfg.type = DETENTED2; // see mios32_enc.h for available types
   enc_config.cfg.sr = 4;    // J6 of DINx4
   enc_config.cfg.pos = 4;   // D4/D5
   enc_config.cfg.speed = NORMAL;
   enc_config.cfg.speed_par = 0;
   MIOS32_ENC_ConfigSet(0, enc_config);

   // read EEPROM content
  // PRESETS_Init(0);

   // init MIDI port/router handling
   MIDI_PORT_Init(0);
   MIDI_ROUTER_Init(0);

   // init terminal
   //TERMINAL_Init(0);

   // init MIDImon
   MIDIMON_Init(0);

   // start uIP task
   UIP_TASK_Init(0);

   // print welcome message on MIOS terminal
   MIOS32_MIDI_SendDebugMessage("\n");
   MIOS32_MIDI_SendDebugMessage("=================\n");
   MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
   MIOS32_MIDI_SendDebugMessage("=================\n");
   MIOS32_MIDI_SendDebugMessage("\n");
   
   // init pedal functions
   PEDALS_Init(0);

   // init the indicators
   IND_Init();

   // init the HMI
   HMI_Init();

   // start 1ms task
   xTaskCreate(TASK_Period_1mS, "1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void) {
   // PWM modulate the status LED (this is a sign of life)
   u32 timestamp = MIOS32_TIMESTAMP_Get();
   MIOS32_BOARD_LED_Set(1, (timestamp % 20) <= ((timestamp / 100) % 10));

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package) {
   // -> MIDI Router
   MIDI_ROUTER_Receive(port, midi_package);

   // -> MIDI Port Handler (used for MIDI monitor function)
   MIDI_PORT_NotifyMIDIRx(port, midi_package);

   // forward to MIDI Monitor
   // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
   // (the SysEx stream would interfere with monitor messages)
   u8 filter_sysex_message = (port == USB0) || (port == UART0);
   MIDIMON_Receive(port, midi_package, filter_sysex_message);
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in) {
   // -> MIDI Router
   MIDI_ROUTER_ReceiveSysEx(port, midi_in);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void) {

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a switch has been toggled
// pin_value is 1 when switch released, and 0 when switch pressed.
//
// This function decodes the pin number to function and dispatches to the
// appropriate handler.
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value) {

   u32 timestamp = MIOS32_TIMESTAMP_Get();

   // Use pressed = 1 as more intuitive, but it is opposite 
   // for switches which are normally open (1) and pedals which are normal closed (0)
   u8 switchPressed = 0;
   if (pin_value == 0)
      switchPressed = 1;

   u8 pedalPressed = pin_value;
      
   //DEBUG_MSG("pin=%d value=%d",pin,pin_value);

   switch (pin) {
   case 15:
      // pedal #1
      PEDALS_NotifyChange(1, pedalPressed, timestamp);
      return;
   case 14:
      // pedal #2
      PEDALS_NotifyChange(2, pedalPressed, timestamp);
      return;
   case 13:
      // pedal #3
      PEDALS_NotifyChange(3, pedalPressed, timestamp);
      return;
   case 12:
      // pedal #4
      PEDALS_NotifyChange(4, pedalPressed, timestamp);
      return;
   case 11:
      // pedal #5
      PEDALS_NotifyChange(5, pedalPressed, timestamp);
      return;
   case 10:
      // pedal #6
      PEDALS_NotifyChange(6, pedalPressed, timestamp);
      return;
   case 9:
      // pedal #7
      PEDALS_NotifyChange(7, pedalPressed, timestamp);
      return;
   case 8:
      // pedal #8
      PEDALS_NotifyChange(8, pedalPressed, timestamp);
      return;
   case 0:
      // pedal #9
      PEDALS_NotifyChange(9, pedalPressed, timestamp);
      return;
   case 1:
      // pedal #10
      PEDALS_NotifyChange(10, pedalPressed, timestamp);
      return;
   case 2:
      // pedal #11
      PEDALS_NotifyChange(11, pedalPressed, timestamp);
      return;
   case 3:
      // pedal #12
      PEDALS_NotifyChange(12, pedalPressed, timestamp);
      return;
   case 4:
      // make switch
      PEDALS_NotifyMakeChange(switchPressed, timestamp);
      return;
   case 23:
      HMI_NotifyToeToggle(1,switchPressed,timestamp);
      return;
   case 22:
      HMI_NotifyToeToggle(2,switchPressed,timestamp);
      return;
   case 21:
      HMI_NotifyToeToggle(3,switchPressed,timestamp);
      return;
   case 20:
      HMI_NotifyToeToggle(4,switchPressed,timestamp);
      return;
   case 19:
      HMI_NotifyToeToggle(5,switchPressed,timestamp);
      return;
   case 18:
      HMI_NotifyToeToggle(6,switchPressed,timestamp);
      return;
   case 17:
      HMI_NotifyToeToggle(7,switchPressed,timestamp);
      return;
   case 16:
      HMI_NotifyToeToggle(8,switchPressed,timestamp);
      return;   
   case 31:
      HMI_NotifyStompToggle(1,switchPressed,timestamp);
      return;
   case 27:
      HMI_NotifyStompToggle(2,switchPressed,timestamp);
      return;
   case 26:
      HMI_NotifyStompToggle(3,switchPressed,timestamp);
      return;
   case 25:
      HMI_NotifyStompToggle(4,switchPressed,timestamp);
      return;
   case 7:
      HMI_NotifyStompToggle(5,switchPressed,timestamp);
      return;  
   case 30:
      HMI_NotifyBackToggle(switchPressed,timestamp);
      return;               
   case 24:
      HMI_NotifyEncoderSwitchToggle(switchPressed,timestamp);
      return;
  default:
      // Invalid pin
      DEBUG_MSG("Invalid pin=%d, switchPressed=%d", pin, switchPressed);
      return;
   }

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer) {
   HMI_NotifyEncoderChange(incrementer);
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically every 1 ms to update the HMI state
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS(void* pvParameters) {
   portTickType xLastExecutionTime;

   // Initialise the xLastExecutionTime variable on task entry
   xLastExecutionTime = xTaskGetTickCount();

   while (1) {
      vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

      // skip delay gap if we had to wait for more than 5 ticks to avoid
      // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
      portTickType xCurrentTickCount = xTaskGetTickCount();
      if (xLastExecutionTime < (xCurrentTickCount - 5))
         xLastExecutionTime = xCurrentTickCount;

      // Service the indicators
      IND_1msTick();
   }
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte) {
   // filter MIDI In port which controls the MIDI clock
   if (MIDI_ROUTER_MIDIClockInGet(port) == 1) {
      // SEQ_BPM_NotifyMIDIRx(midi_byte);
   }
   return 0; // no error, no filtering
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package) {
   return MIDI_PORT_NotifyMIDITx(port, package);
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port) {
   // forward to SysEx parser
   // MBKB_SYSEX_TimeOut(port);

   // print message on screen
   //SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

   return 0;
}


