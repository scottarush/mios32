// $Id$
/*
 * Instrument selection page
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_hwcfg.h"

#include "seq_trg.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 1 << ui_selected_instrument;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    // select new layer/instrument

    if( encoder >= SEQ_TRG_NumInstrumentsGet(visible_track) )
      return -1;
    ui_selected_instrument = encoder;

    if( seq_hwcfg_button_beh.ins_sel ) {
      // if toggle function active: jump back to previous menu
      // this is especially useful for the emulated MBSEQ, where we can only click on a single button
      // (trigger gets deactivated when clicking on GP button or moving encoder)
      seq_ui_button_state.INS_SEL = 0;
      SEQ_UI_PageSet(ui_inssel_prev_page);
    }

    return 1; // value changed
  } else if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, SEQ_TRG_NumInstrumentsGet(visible_track)-1, incrementer);
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INSSEL_Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // -> same handling like for encoders
    return Encoder_Handler(button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
      return -1; // unsupported (yet)

    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      if( depressed ) return 0; // ignore when button depressed
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      if( depressed ) return 0; // ignore when button depressed
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // layout drum mode (lower line shows drum labels):
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Select Drum Instrument:                                                         
  //  BD   SD   LT   MT   HT   CP   MA   RS   CB   CY   OH   CH  Smp1 Smp2 Smp3 Smp4 
  // ...horizontal VU meters...

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters

    SEQ_LCD_CursorSet(0, 1);

    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      u8 drum;
      u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
      for(drum=0; drum<num_instruments; ++drum) {
	if( seq_core_trk[visible_track].layer_muted & (1 << drum) )
	  SEQ_LCD_PrintString("Mute ");
	else
	  SEQ_LCD_PrintHBar((seq_layer_vu_meter[drum] >> 3) & 0xf);
      }
    } else {
      seq_core_trk_t *t = &seq_core_trk[visible_track];
      u16 mask = 1 << visible_track;

      if( seq_core_trk_muted & mask ) {
	SEQ_LCD_PrintString("Mute ");
      } else {
	SEQ_LCD_PrintHBar(t->vu_meter >> 3);
      }
    }

    return 0; // no error
  }

  u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  int i;
  for(i=0; i<num_instruments; ++i) {
    if( i == ui_selected_instrument && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintTrackDrum(visible_track, i, (char *)seq_core_trk[visible_track].name);
      } else {
	SEQ_LCD_PrintFormattedString("INS%2d", i+1);
      }
    }
  }
    
  SEQ_LCD_PrintSpaces(80 - (5*num_instruments));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INSSEL_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(SEQ_UI_INSSEL_Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show horizontal VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

  return 0; // no error
}
