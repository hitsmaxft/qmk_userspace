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

#pragma once
#define OLED_IC OLED_IC_SSD1306

#define SPLIT_USB_DETECT
#define MASTER_LEFT

#define OLED_CS_PIN 29
#define OLED_DC_PIN 28
#define OLED_DISPLAY_128X32 TRUE

// mock apple keyboard vid;pid
// #undef VENDOR_ID
// #define VENDOR_ID 0x05AC
// #undef PRODUCT_ID
// #define PRODUCT_ID 0x0220

// Timing constants
#define OLED_APM_INTERVAL_MS 1000
#define LOGO_HIDE_DELAY_MS 3000
#define SHIFT_TAB_TIMEOUT_MS 1000

// Layer configuration
#define TRI_LAYER_LOWER_LAYER 2  // Sets the default for the "lower" layer.
#define TRI_LAYER_UPPER_LAYER 3  // Sets the default for the "upper" layer.
#define TRI_LAYER_ADJUST_LAYER 4 // Sets the default for the "adjust" layer.
#define WIN_LAYER_DEF_LAYER 1    // Sets the default for the "lower" layer.

// Tapping configuration
#define TAPPING_TERM_MS 180
#define PERMISSIVE_HOLD
#define CHORDAL_HOLD

// Flow tap configuration
#define FLOW_TAP_DEBUG
#define FLOW_TAP_MOD TRUE
#define FLOW_TAP_TERM 40
#define FLOW_TAP_TERM_MS 40
#define QUICK_TAP_TERM_MS 120

#undef RETRO_TAPPING
#undef HOLD_ON_OTHER_KEY_PRESS

