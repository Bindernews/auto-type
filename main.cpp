#include <stdlib.h>
#include <string.h>
#include <pico/time.h>
#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <stdio.h>
#include "decode.h"
#include "config_check.h"

/** Timeout before registering if a click was single or double in Control Mode. */
#define DOUBLE_CLICK_TIMEOUT (500)
/** In Default Mode, wait INITIAL_DELAY ms before starting to print data. Ignored in Control Mode. */
#define INITIAL_DELAY (4000)
/** Delay between keypresses in keyboard mode. Currently ignored. */
#define KEY_PRESS_DELAY (1)

/** GPIO pin number */
#define PIN_CTRL_PREV  16
/** GPIO pin number */
#define PIN_CTRL_ENTER 17
/** GPIO pin number */
#define PIN_CTRL_NEXT  18
#define NUM_INPUT_PINS 3

enum {
    /** No control command, carry on. */
    CONTROL_NONE  = 0,
    /** Begin printing the currently selected source. */
    CONTROL_ENTER = 1 << 0,
    /** Print the name of the currently selected source. */
    CONTROL_INFO  = 1 << 1,
    /** Cancel current print. */
    CONTROL_CANCEL = 1 << 2,
};

enum {
    /** Sent when a print begins. */
    EVENT_BEGIN =   1 << 0,
    /** Sent after a print ends. */
    EVENT_END =     1 << 1,
    /** Sent after the user has requested the source name. */
    EVENT_INFO =    1 << 2,
    /** Sent after a print has been cancelled (also sends EVENT_END). */
    EVENT_CANCEL =  1 << 3,
};

#if defined(KEYBOARD_OUT)
#include "usb_descriptors.h"
#include "tusb.h"
#include "bsp/board.h"
#endif

#if defined(STDIO_OUT)
#endif

/////////////
// Utility //
/////////////

/** Return true if \c v contains all of the bits of mask \c m . Short for "mask-test". */
static inline bool mtest(int v, int m) { return (v & m) == m; }

extern "C" {
uint8_t map_key(char c);
}

/**
 * Returns true if \c t is passed the the current time.
 */
static inline bool is_passed_time(absolute_time_t t)
{
    return absolute_time_diff_us(t, get_absolute_time()) >= 0;
}

//////////////
// Controls //
//////////////

/** Called to init the controls module. */
void controls_init();
/** Called continuously in a loop. */
void controls_task();
/** Callback for various events. */
void controls_event(int event, int data);
/** The current source index to print. */
static int s_source;
/** Current control state. */
static int s_control_state;


//////////////////////////////
// Implementation Functions //
//////////////////////////////

static void impl_init();
static void impl_task();
static void impl_event(int event, int data);
static bool impl_print(const char *message, size_t count);

//////////
// Main //
//////////

/** Helper function to decode and print \c count characters using \c impl_print . */
bool decode_some(int count);

/** Helper function to invoke each event callback. */
static inline void pass_event(int event, int data)
{
    controls_event(event, data);
    impl_event(event, data);
}

/** Toggle the built-in LED on and off \c count times, useful for debugging. */
static void debug_led(uint count, uint timeout=150)
{
    #ifndef NDEBUG
    for (uint i = 0; i < count; i++) {
        gpio_put(25, 1);
        sleep_ms(timeout);
        gpio_put(25, 0);
        sleep_ms(timeout);
    }
    #endif
}

int main()
{
    int mode = 0;

    stdio_init_all();
    controls_init();
    impl_init();
    if (decode_init() != 0) {
        return 0;
    }

    while (1) {
        controls_task();
        impl_task();

        if (s_control_state == CONTROL_INFO) {
            const char *name = decode_source_name(s_source);
            if (name) {
                size_t name_len = strlen(name);
                impl_print(name, name_len);
                impl_print("\n", 1);
            }
            pass_event(EVENT_INFO, 0);
        }
        if (s_control_state == CONTROL_ENTER) {
            if (decode_begin(s_source) != 0) {
                return 0;
            }
            mode = 1;
            pass_event(EVENT_BEGIN, 0);
        }
        if (s_control_state == CONTROL_CANCEL && mode == 1) {
            mode = 0;
            pass_event(EVENT_END | EVENT_CANCEL, 0);
        }
        if (mode == 1) {
            if (!decode_some(10)) {
                mode = 0;
                pass_event(EVENT_END, 0);
            }
        }
    }
    return 0;
}

bool decode_some(int count)
{
    char buf[count + 1];
    int len;
    for (len = 0; len < count; len++) {
        int c = decode_next_char();
        if (c == -1) {
            break;
        }
        buf[len] = (char)c;
    }
    if (len > 0) {
        impl_print(buf, len);
    }
    return len == count;
}


#if defined(KEYBOARD_OUT)
bool usb_on = false;
absolute_time_t next_write = nil_time;

void task_debug_a();
bool send_next_report();
uint8_t map_key(char c);

static inline bool is_next_write_time()
{
    return is_passed_time(next_write);
}

/**
 * Wait until the HID keyboard is ready to send more data.
 */
static void hid_wait()
{
    while (!tud_hid_ready()) { tud_task(); }
}

/**
 * Type (on and off) a single character via the HID keyboard.
 * This assumes the HID interface is connected, but will block if the keyboard isn't ready to receive more data.
 */
static void report_char(char c)
{
    uint8_t keycode[6] = { 0 };
    uint8_t cm = map_key(c);
    uint8_t mod = 0;
    if (cm & 0x80) {
        mod |= 0x02;
    }
    cm &= 0x7F;
    keycode[0] = cm;
    // Press key
    hid_wait();
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, mod, keycode);
    // Unpress key
    hid_wait();
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
}

static void impl_init()
{
    board_init();
    tusb_init();
}

static void impl_task()
{
    tud_task();
    // task_debug_a();
}

static void impl_event(int event, int data)
{
    (void) event;
    (void) data;
}

static bool impl_print(const char* msg, size_t count)
{
    if (usb_on && is_next_write_time()) {
        for (size_t i = 0; i < count; i++) {
            report_char(msg[i]);
        }
    }
    return true;
}

inline void task_debug_a()
{
    uint8_t keycode[6] = {0};

    if (!(usb_on && tud_hid_ready() && is_next_write_time())) {
        return;
    }

    // send 'a'
    keycode[0] = 0x04;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
    // send key release
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    next_write = make_timeout_time_ms(500);
}

//-------------------
// USB HID callbacks
//-------------------

// Invoked when device is mounted
void tud_mount_cb()
{
    // When the USB is mounted, reset variables
    usb_on = true;
    //next_write = make_timeout_time_ms(INITIAL_DELAY);
}

// Invoked when device is unmounted
void tud_umount_cb()
{
    usb_on = false;
}

// Invoked to return data to the host
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
        uint8_t *buffer, uint16_t reqLen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqLen;
    
    return 0;
}

// Invoked when we receive data from the host (via SET REPORT)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
        uint8_t const *buffer, uint16_t bufSize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufSize;

    return;
}
#endif

#if defined(STDIO_OUT)
void impl_init()
{}

void impl_task()
{}

void impl_event(int event, int data)
{
    (void) event;
    (void) data;
}

static bool impl_print(const char *msg, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        putchar(msg[i]);
    }
    return true;
}
#endif

#if defined(CFG_BUTTON_CONTROLS)
static const uint PIN_IDS[] = { PIN_CTRL_PREV, PIN_CTRL_ENTER, PIN_CTRL_NEXT };
enum {
    PIN_IDX_PREV = 0,
    PIN_IDX_ENTER,
    PIN_IDX_NEXT,
};

struct control_state_s {
    absolute_time_t dbl_click_timeout;
    bool printing;
    int click_count;
    bool pins[NUM_INPUT_PINS];
    bool pins_prev[NUM_INPUT_PINS];
};

static struct control_state_s s_ctrl;

void controls_init()
{
    s_source = 0;
    s_control_state = 0;

    // Init pins and pin state
    for (int i = 0; i < NUM_INPUT_PINS; i++) {
        gpio_set_dir(PIN_IDS[i], false);
        gpio_pull_up(PIN_IDS[i]);
        s_ctrl.pins[i] = false;
        s_ctrl.pins_prev[i] = false;
    }
    // Init other state
    s_ctrl.dbl_click_timeout = nil_time;
    s_ctrl.click_count = 0;
    s_ctrl.printing = false;
}

void controls_task()
{
    struct control_state_s *self = &s_ctrl;
    bool released[NUM_INPUT_PINS];
    /** Is the user done clicking the enter button? */
    bool click_done = false;

    // Update pin states
    for (int i = 0; i < NUM_INPUT_PINS; i++) {
        self->pins_prev[i] = self->pins[i];
        self->pins[i] = !gpio_get(PIN_IDS[i]);
        released[i] = !self->pins[i] && self->pins_prev[i];
    }
    #if CFG_LED_WHEN_PRESSED
    if (self->pins[0] || self->pins[1] || self->pins[2]) {
        gpio_put(25, 1);
    } else {
        gpio_put(25, 0);
    }
    #endif

    // Count our enter clicks
    if (released[PIN_IDX_ENTER]) {
        // New click has arrived
        self->click_count += 1;
        self->dbl_click_timeout = make_timeout_time_ms(DOUBLE_CLICK_TIMEOUT);
    }
    // If we passed the timeout and the user isn't currently holding down the key
    // then detect it as the number of clicks.
    else if (is_passed_time(self->dbl_click_timeout) && !self->pins[PIN_IDX_ENTER]) {
        // Done clicking
        click_done = true;
    }

    s_control_state = CONTROL_NONE;
    // If we're NOT currently printing we can do all sorts of things
    if (!self->printing) {
        #if defined(CFG_PIN_PREV_ENABLED) || 1
        // Test if previous/down button was pressed
        if (released[PIN_IDX_PREV]) {
            s_source -= 1;
            if (s_source < 0) {
                s_source = decode_num_sources() - 1;
            }
        }
        #endif
        // Test if next/up button was pressed
        if (released[PIN_IDX_NEXT]) {
            s_source += 1;
            if (s_source >= decode_num_sources()) {
                s_source = 0;
            }
        }
        // Test for single-click
        if (click_done && self->click_count == 1) {
            s_control_state = CONTROL_INFO;
        }
        // Test for double-click
        if (click_done && self->click_count == 2) {
            s_control_state = CONTROL_ENTER;
        }
    }
    // If we ARE printing, then double-click will cancel
    else {
        // We're printing so a double-click will cancel
        if (click_done && self->click_count == 2) {
            s_control_state = CONTROL_CANCEL;
        }
    }
    
    // Reset click state
    if (click_done) {
        self->click_count = 0;
        self->dbl_click_timeout = nil_time;
    }
}

void controls_event(int event, int data)
{
    struct control_state_s *self = &s_ctrl;

    if (mtest(event, EVENT_BEGIN)) {
        self->printing = true;
    }
    if (mtest(event, EVENT_END)) {
        self->printing = false;
    }
}

#elif defined(CFG_DEFAULT_CONTROLS)
static int s_printing = 0;

void controls_init()
{
    s_source = 0;
    s_control_state = CONTROL_ENTER;
    sleep_ms(INITIAL_DELAY);
}

void controls_task()
{}

void controls_event(int event, int data)
{
    if (mtest(event, EVENT_BEGIN)) {
        s_printing = true;
        s_control_state = CONTROL_NONE;
    }
    if (mtest(event, EVENT_END)) {
        s_printing = false;
        s_source++;
        if (s_source >= decode_num_sources()) {
            s_source = -1;
        } else {
            s_control_state = CONTROL_ENTER;
        }
    }
}
#endif
