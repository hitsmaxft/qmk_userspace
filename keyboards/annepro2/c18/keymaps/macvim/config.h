/*
 * Copyright (c) 2024 BHE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#define MK_KINETIC_SPEED 1
#define MOUSEKEY_MAX_SPEED 4
#define MOUSEKEY_MOVE_DELTA 	30
#define MOUSEKEY_INITIAL_SPEED 	500
#define MOUSEKEY_BASE_SPEED 	4000
#define MOUSEKEY_TIME_TO_MAX 60
#define MOUSEKEY_ACCELERATED_SPEED 	4000
#define MOUSEKEY_WHEEL_ACCELERATED_MOVEMENTS 60

//#define MATRIX_IO_DELAY 40
//#define DEBUG_MATRIX_SCAN_RATE


#define DEBOUNCE 10

//per key timeout for leader key
#define LEADER_PER_KEY_TIMING
#define LEADER_TIMEOUT 250

#define TAPPING_TERM 170
#define TAPPING_TERM_PER_KEY

#undef RETRO_TAPPING
#define QUICK_TAP_TERM 60
#define QUICK_TAP_TERM_PER_KEY
#undef PERMISSIVE_HOLD
#undef HOLD_ON_OTHER_KEY_PRESS
#define CHORDAL_HOLD

/*
 * The USB console leaves little flash headroom on the C18. Keep the normal
 * keymap's complete RGB effect set, but omit the larger framebuffer/reactive
 * animations from BLE diagnostic builds.
 */
#ifdef ANNEPRO2_BLE_DEBUG
#    undef ENABLE_RGB_MATRIX_PIXEL_FRACTAL
#    undef ENABLE_RGB_MATRIX_PIXEL_FLOW
#    undef ENABLE_RGB_MATRIX_PIXEL_RAIN
#    undef ENABLE_RGB_MATRIX_TYPING_HEATMAP
#    undef ENABLE_RGB_MATRIX_DIGITAL_RAIN
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#    undef ENABLE_RGB_MATRIX_SPLASH
#    undef ENABLE_RGB_MATRIX_MULTISPLASH
#    undef ENABLE_RGB_MATRIX_SOLID_SPLASH
#    undef ENABLE_RGB_MATRIX_SOLID_MULTISPLASH
#endif
