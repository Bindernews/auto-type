#pragma once

#define _CHECK (defined(KEYBOARD_OUT) + defined(STDIO_OUT))
#if _CHECK == 0
#error "Must define either KEYBOARD_OUT or STDIO_OUT"
#elif _CHECK > 1
#error "Must define only ONE (1) of KEYBOARD_OUT or STDIO_OUT"
#endif
#undef _CHECK

#define _CHECK (defined(CFG_BUTTON_CONTROLS) + defined(CFG_DEFAULT_CONTROLS))
#if _CHECK == 0
#error "Must define either CFG_BUTTON_CONTROLS or CFG_DEFAULT_CONTROLS"
#elif _CHECK > 1
#error "Must define only ONE (1) of CFG_BUTTON_CONTROLS or CFG_DEFAULT_CONTROLS"
#endif
#undef _CHECK

#if !defined(CFG_LED_WHEN_PRESSED)
#define CFG_LED_WHEN_PRESSED 0
#endif
