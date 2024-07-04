/*
 * M3 SuperPedal
 *
 * ==========================================================================
 *  Copyright (C) 2024 Scott Rush (scottarush@yahoo.com)
 *  Licensed for personal non-commercial use only.
 *  Uses open-source software from midibox.org subject to
 * midibox project licensing terms.
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
#include <msd.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <mid_parser.h>

#include "mid_file.h"
#include "midio_file.h"
#include "file.h"
#include "arp.h"
#include "app.h"
#include "hmi.h"
#include "pedals.h"
#include "indicators.h"
#include "midi_presets.h"
#include "persist.h"

#include "seq.h"

#include "terminal.h"
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines and types
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// define priority level for sequencer
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_Period_1ms ( tskIDLE_PRIORITY + 3 )

// Sequencer processing task.  Note that is even higher priority than MIDI receive task
#define PRIORITY_TASK_ARP		( tskIDLE_PRIORITY + 4 )

// SD Card with lower priority
#define PRIORITY_TASK_PERIOD_1mS_SD ( tskIDLE_PRIORITY + 2 )


typedef enum {
   MSD_DISABLED,
   MSD_INIT,
   MSD_READY,
   MSD_SHUTDOWN
} msd_state_t;

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// This is taken from the MIDIO128 SDCard implementation.  May not be used/neeeded here
u8 hw_enabled;
/////////////////////////////////////////////////////////////////////////////
// Local variables 
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN/OUT handler in modules/midi_router/midi_router.c
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;
static volatile msd_state_t msd_state;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

// local prototype of the 1ms task function
static void TASK_Period_1ms(void* pvParameters);

static void TASK_ARP(void* pvParameters);
static void TASK_Period_1mS_SD(void* pvParameters);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void) {
   // hardware will be enabled once configuration has been loaded from SD Card
   // (resp. no SD Card is available)
   hw_enabled = 0;

   // create semaphores
   xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
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

   // init MIDI port/router handling
   MIDI_PORT_Init(0);
   MIDI_ROUTER_Init(0);


   // init MIDI file handler
   MID_FILE_Init(0);
   // init MIDI parser module
   MID_PARSER_Init(0);

   // install callback functions
   MID_PARSER_InstallFileCallbacks(&MID_FILE_read, &MID_FILE_eof, &MID_FILE_seek);
   // TODO:  Use the MID_PARSER to play sequences from MID files on preset
  // MID_PARSER_InstallEventCallbacks(&SEQ_PlayEvent, &SEQ_PlayMeta);

   // initialize MIDI handler for Sequencer used by Arpeggiator
   SEQ_MIDI_OUT_Init(0);

   // initialize Arpeggiator
   ARP_Init(0);

   // init terminal
   TERMINAL_Init(0);

   // init MIDImon
   MIDIMON_Init(0);

   // print welcome message on MIOS terminal
   MIOS32_MIDI_SendDebugMessage("\n");
   MIOS32_MIDI_SendDebugMessage("=================\n");
   MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
   MIOS32_MIDI_SendDebugMessage("=================\n");
   MIOS32_MIDI_SendDebugMessage("\n");

   // Init the persistence handler
   if (PERSIST_Init(0) < 0) {
      MIOS32_MIDI_SendDebugMessage("Error initializing EEPROM");
   }

   // init pedal functions
   PEDALS_Init(0);

   // init the indicators
   IND_Init();

   // Init the MIDI Presets
   MIDI_PRESETS_Init();

   // init the HMI last as it depends on above.
   HMI_Init();

   // initial load of filesystem
   s32 status = FILE_Init(0);
   if (status != 0) {
      DEBUG_MSG("APP_Init: FILE_Init failed with status=%d");
   }

   // start 1ms task
   xTaskCreate(TASK_Period_1ms, "1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_Period_1ms, NULL);

   // install sequencer task
   xTaskCreate(TASK_ARP, "SEQ", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_ARP, NULL);

   // SDCard task
   xTaskCreate(TASK_Period_1mS_SD, "1mS_SD", 2 * configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_SD, NULL);

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

   // -> MIDI file recorder
   MID_FILE_Receive(port, midi_package);

   //////////////////////////////////////////////////////
   // Notifications to Sequencer
   /////////////////////////////////////////////////////
     // Note On received
   if (midi_package.chn == Chn1 &&
      (midi_package.type == NoteOn || midi_package.type == NoteOff)) {

      // branch depending on Note On/Off event
      if (midi_package.event == NoteOn && midi_package.velocity > 0)
         ARP_NotifyNoteOn(midi_package.note, midi_package.velocity);
      else
         ARP_NotifyNoteOff(midi_package.note);
   }
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
      HMI_NotifyToeToggle(1, switchPressed, timestamp);
      return;
   case 22:
      HMI_NotifyToeToggle(2, switchPressed, timestamp);
      return;
   case 21:
      HMI_NotifyToeToggle(3, switchPressed, timestamp);
      return;
   case 20:
      HMI_NotifyToeToggle(4, switchPressed, timestamp);
      return;
   case 19:
      HMI_NotifyToeToggle(5, switchPressed, timestamp);
      return;
   case 18:
      HMI_NotifyToeToggle(6, switchPressed, timestamp);
      return;
   case 17:
      HMI_NotifyToeToggle(7, switchPressed, timestamp);
      return;
   case 16:
      HMI_NotifyToeToggle(8, switchPressed, timestamp);
      return;
   case 31:
      HMI_NotifyStompToggle(1, switchPressed, timestamp);
      return;
   case 27:
      HMI_NotifyStompToggle(2, switchPressed, timestamp);
      return;
   case 26:
      HMI_NotifyStompToggle(3, switchPressed, timestamp);
      return;
   case 25:
      HMI_NotifyStompToggle(4, switchPressed, timestamp);
      return;
   case 7:
      HMI_NotifyStompToggle(5, switchPressed, timestamp);
      return;
   case 30:
      HMI_NotifyBackToggle(switchPressed, timestamp);
      return;
   case 24:
      HMI_NotifyEncoderSwitchToggle(switchPressed, timestamp);
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
static void TASK_Period_1ms(void* pvParameters) {
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
// This task is called periodically each mS to handle arpeggiator requests
/////////////////////////////////////////////////////////////////////////////
static void TASK_ARP(void* pvParameters)
{
   portTickType xLastExecutionTime;

   // Initialise the xLastExecutionTime variable on task entry
   xLastExecutionTime = xTaskGetTickCount();

   while (1) {
      vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

      // execute arpeggiator handler
      ARP_Handler();

      // send timestamped MIDI events
      SEQ_MIDI_OUT_Handler();
   }
}
/////////////////////////////////////////////////////////////////////////////
// This task handles the SD Card
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_SD(void* pvParameters)
{
   const u16 sdcard_check_delay = 1000;
   u16 sdcard_check_ctr = 0;
   u8 lun_available = 0;

   while (1) {
      vTaskDelay(1 / portTICK_RATE_MS);

      // each second: check if SD Card (still) available
      if (msd_state == MSD_DISABLED && ++sdcard_check_ctr >= sdcard_check_delay) {
         sdcard_check_ctr = 0;

         MUTEX_SDCARD_TAKE;
         s32 status = FILE_CheckSDCard();

         if (status == 1) {
            DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());

            // stop sequencer
            SEQ_BPM_Stop();

            // load all file infos
            MIDIO_FILE_LoadAllFiles(1); // including HW info

            // immediately go to next step
            sdcard_check_ctr = sdcard_check_delay;
         }
         else if (status == 2) {
            DEBUG_MSG("SD Card disconnected\n");
            // invalidate all file infos
            MIDIO_FILE_UnloadAllFiles();

            // stop sequencer
            SEQ_BPM_Stop();

            // change status
            MIDIO_FILE_StatusMsgSet("No SD Card");
         }
         else if (status == 3) {
            if (!FILE_SDCardAvailable()) {
               DEBUG_MSG("SD Card not found\n");
               MIDIO_FILE_StatusMsgSet("No SD Card");
            }
            else if (!FILE_VolumeAvailable()) {
               DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
               MIDIO_FILE_StatusMsgSet("No FAT");
            }
            else {
               // create the default files if they don't exist on SD Card
               MIDIO_FILE_CreateDefaultFiles();

               // load first MIDI file
               MID_FILE_UI_NameClear();
               SEQ_SetPauseMode(1);
               // TODO:  Need to pull in the sequence play function
               SEQ_PlayFileReq(0, 1);
            }

            hw_enabled = 1; // enable hardware after first read...
         }

         MUTEX_SDCARD_GIVE;
      }

      // MSD driver
      if (msd_state != MSD_DISABLED) {
         MUTEX_SDCARD_TAKE;

         switch (msd_state) {
         case MSD_SHUTDOWN:
            // switch back to USB MIDI
            MIOS32_USB_Init(1);
            msd_state = MSD_DISABLED;
            break;

         case MSD_INIT:
            // LUN not mounted yet
            lun_available = 0;

            // enable MSD USB driver
            //MUTEX_J16_TAKE;
            if (MSD_Init(0) >= 0)
               msd_state = MSD_READY;
            else
               msd_state = MSD_SHUTDOWN;
            //MUTEX_J16_GIVE;
            break;

         case MSD_READY:
            // service MSD USB driver
            MSD_Periodic_mS();

            // this mechanism shuts down the MSD driver if SD card has been unmounted by OS
            if (lun_available && !MSD_LUN_AvailableGet(0))
               msd_state = MSD_SHUTDOWN;
            else if (!lun_available && MSD_LUN_AvailableGet(0))
               lun_available = 1;
            break;
         }

         MUTEX_SDCARD_GIVE;
      }
   }

}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte) {
   // filter MIDI In port which controls the MIDI clock
   if (MIDI_ROUTER_MIDIClockInGet(port) == 1) {
      SEQ_BPM_NotifyMIDIRx(midi_byte);
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

   return 0; // no error, no filtering
}


/////////////////////////////////////////////////////////////////////////////
// MSD access functions
/////////////////////////////////////////////////////////////////////////////
s32 TASK_MSD_EnableSet(u8 enable)
{
  MIOS32_IRQ_Disable();
  if( msd_state == MSD_DISABLED && enable ) {
    msd_state = MSD_INIT;
  } else if( msd_state == MSD_READY && !enable )
    msd_state = MSD_SHUTDOWN;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 TASK_MSD_EnableGet()
{
  return (msd_state == MSD_READY) ? 1 : 0;
}

s32 TASK_MSD_FlagStrGet(char str[5])
{
  str[0] = MSD_CheckAvailable() ? 'U' : '-';
  str[1] = MSD_LUN_AvailableGet(0) ? 'M' : '-';
  str[2] = MSD_RdLEDGet(250) ? 'R' : '-';
  str[3] = MSD_WrLEDGet(250) ? 'W' : '-';
  str[4] = 0;

  return 0; // no error
}



