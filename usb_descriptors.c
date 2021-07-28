#include "tusb_config.h"
#include "tusb.h"
#include "usb_descriptors.h"

// RP2040-specific
#include <pico/unique_id.h>

// See https://github.com/obdev/v-usb/blob/master/usbdrv/USB-IDs-for-free.txt
// for notes on why we're using this VID and PID.

#define MFR_STRING "example.com"

#define STR_BUF_SIZE 32
#define EPNUM_HID 0x81
enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL,
};

tusb_desc_device_t const desc_device = {
    .bLength          = sizeof(tusb_desc_device_t),
    .bDescriptorType  = TUSB_DESC_DEVICE,
    .bcdUSB           = 0x0200,
    .bDeviceClass     = 0x00,
    .bDeviceSubClass  = 0x00,
    .bDeviceProtocol  = 0x00,
    .bMaxPacketSize0  = CFG_TUD_ENDPOINT0_SIZE,
    
    .idVendor         = 0x16c0,
    .idProduct        = 0x27db,
    .bcdDevice        = 0x0100,
    
    .iManufacturer    = 0x01,
    .iProduct         = 0x02,
    .iSerialNumber    = 0x03,

    .bNumConfigurations = 1
};

// Invoked for GET DEVICE DESCRIPTOR
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(REPORT_ID_KEYBOARD) ),
    TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(REPORT_ID_MOUSE)    ),
};

// Invoked for GET HID REPORT DESCRIPTOR
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    
    return desc_hid_report;
}


#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, protocol, report descriptor len, EP in & out address, size & poll interval
    //TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

// Invoked for GET CONFIGURATION DESCRIPTOR
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}


char const *string_desc_arr[] = {
    (const char[]) {0x09, 0x04},  // 0: support English lang
    MFR_STRING,                   // 1: Manufacturer
    "Keyboard",                   // 2: Product
    "FECABEBA",                   // 3: Serial no, should use chip ID
};

// Save flash space by converting strings to UTF-16 at runtime.
// This buffer MUST exist for the lifetime of the USB data transfer so we just make it static.
static uint16_t _str_buf[STR_BUF_SIZE];
// Invoked for GET STRING DESCRIPTOR
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    uint8_t len = 0;

    if (index == 0) {
        memcpy(&_str_buf[1], string_desc_arr[0], 2);
        len = 1;
    }
    // This part is RP2040-specific to give an actual serial number.
    else if (index == 3) {
        pico_unique_board_id_t id;
        pico_get_unique_board_id(&id);
        char buf[9];
        snprintf(buf, 9, "%02x%02x%02x%02x", id.id[0], id.id[1], id.id[2], id.id[3]);
        for (int i = 0; i < 8; i++) {
            _str_buf[i + 1] = (uint16_t)buf[i];
        }
        len = 8;
    }
    else {
        // Convert ASCII string to UTF-16
        if (index >= sizeof(string_desc_arr) / sizeof(char*)) {
            return NULL;
        }

        const char *str = string_desc_arr[index];
        len = strlen(str);
        if (len > STR_BUF_SIZE - 1) {
            len = STR_BUF_SIZE - 1;
        }

        for (uint8_t i = 0; i < len; i++) {
            _str_buf[1 + i] = str[i];
        }
    }

    // First byte is length, second byte is string type
    _str_buf[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
    return _str_buf;
}
