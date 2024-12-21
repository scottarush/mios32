// $Id$
/*
 * Studio 90 Plus Midibox KB
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 *  Extensions/Additions for Studio 90 Plus added by Scott Rush 2024
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

#include <keyboard.h>
#include <ainser.h>
#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "hmi.h"
#include "app.h"
#include "keyboard_presets.h"
#include "terminal.h"
#include "tasks.h"
#include "uip_task.h"
#include "osc_client.h"
#include "switches.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// define priority level for sequencer
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;


/////////////////////////////////////////////////////////////////////////////
//! global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

// local prototype of the task function
static void TASK_Period_1mS(void* pvParameters);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);

static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value);


// counter forswitch read cycles
u32 switchReadCounter;

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

   // limit the number of DIN/DOUT SRs which will be scanned for faster scan rate
   MIOS32_SRIO_ScanNumSet(2);

   // init keyboard functions
   KEYBOARD_Init(0);

   // Init the switches
   SWITCHES_Init();
   switchReadCounter = 0;

   // init the HMI
   HMI_Init(0);

   // read EEPROM content
   PRESETS_Init(0);

   // init MIDI port/router handling
   MIDI_PORT_Init(0);
   MIDI_ROUTER_Init(0);

   // initialize the AINSER module(s)
   AINSER_Init(0);
   AINSER_NumModulesSet(0); // 1 module
   AINSER_MuxedSet(0, 0);   // disable muxing
   AINSER_NumPinsSet(0, 8); // 8 pins

   // init terminal
   TERMINAL_Init(0);

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

   // use Prescaler 256 for onise immunity with Studio 90 plus.  This change goes along with a 4 us delay in MIOS32_srio.c.
   // (was MIOS32_SPI_PRESCALER_128, initialized by MIOS32_SRIO_Init())
   MIOS32_SPI_TransferModeInit(MIOS32_SRIO_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);


   // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
   // start the scan here - and retrigger it whenever it's finished
   APP_SRIO_ServicePrepare();
   MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);

   // start tasks
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

   // scan AINSER pins
   AINSER_Handler(APP_AINSER_NotifyChange);

   // Read the switches if counter has expired
   if (switchReadCounter >= SWITCH_READ_TIME_MS) {
      SWITCHES_Read();
      switchReadCounter = 0;
   }
   else {
      switchReadCounter++;
   }
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
   // -> keyboard handler
   KEYBOARD_SRIO_ServicePrepare();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void) {
   // -> keyboard handler
   KEYBOARD_SRIO_ServiceFinish();

   // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
   // start the scan here - and retrigger it whenever it's finished
   APP_SRIO_ServicePrepare();
   MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value) {
   // -> keyboard
   KEYBOARD_AIN_NotifyChange(pin, pin_value);
#if 0
   MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x10 + pin, pin_value >> 5);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value) {
   // -> continue with normal AIN handler
   APP_AIN_NotifyChange(128 + pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to check for keyboard MIDI events
// and also calls a general HMI tick used for things like display flash timing.
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

      // -> keyboard handler
      KEYBOARD_Periodic_1mS();

      // MIDI In/Out monitor
      // TODO: call from low-prio task
      MIDI_PORT_Period1mS();

      // update AINs with current value
      // the keyboard driver will only send events on value changes

      int pin;

      for (pin = 0; pin < 8; ++pin) {
         KEYBOARD_AIN_NotifyChange(pin, MIOS32_AIN_PinGet(pin));
      }

      // Tick the HMI
      HMI_1msTick();
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

/**
/////////////////////////////////////////////////////////////////////////////
// Keyboard notification hook from Keyboard module.  We'll send our own
// Midi notes from here.
//
// Not used here becuase it was not properly defined in keyboard.c.
// The depressed param wasn't included so we can't do the Off/On forwarding.
// For now, just modify the keyboard module for diagnostics.
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_NOTIFY_TOGGLE_HOOK(u8 kb, u8 note_number, u8 velocity, u8 depressed) {

   keyboard_config_t* kc = (keyboard_config_t*)&keyboard_config[kb];

   if (kc->midi_chn) {
      int i;
      u16 mask = 1;
      for (i = 0; i < 16; ++i, mask <<= 1) {
         if (kc->midi_ports & mask) {
            // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
            mios32_midi_port_t port = 0x10 + ((i & 0xc) << 2) + (i & 3);
            if (depressed && kc->scan_release_velocity)
               MIOS32_MIDI_SendNoteOff(port, kc->midi_chn - 1, note_number, velocity);
            else
               MIOS32_MIDI_SendNoteOn(port, kc->midi_chn - 1, note_number, velocity);
         }
      }
   }
   return 0;
}

**/

