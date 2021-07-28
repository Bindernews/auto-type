#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// defined by board.mk
#ifndef CFG_TUSB_MCU
  #error CFG_TUSB_MCU must be defined
#endif

#define BOARD_DEVICE_RHPORT_NUM 0
#define CFG_TUSB_RHPORT0_MODE  OPT_MODE_DEVICE
#define CFG_TUSB_OS OPT_OS_PICO

//----------------------
// Device Configuration
//----------------------

#define CFG_TUD_ENDPOINT0_SIZE 64

// Class
#define CFG_TUD_CDC    0
#define CFG_TUD_MSC    0
#define CFG_TUD_HID    1
#define CFG_TUD_MIDI   0
#define CFG_TUD_VENDOR 0

#define CFG_TUD_HID_BUFSIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 16


#ifdef __cplusplus
}
#endif
