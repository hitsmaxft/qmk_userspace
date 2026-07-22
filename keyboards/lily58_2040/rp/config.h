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

#define DYNAMIC_KEYMAP_LAYER_COUNT 10
#define VIAL_COMBO_ENTRIES 20
#define VIAL_KEY_OVERRIDE_ENTRIES 20
#define VIAL_TAP_DANCE_ENTRIES 20
#define DYNAMIC_KEYMAP_MACRO_COUNT 50

#define USB_VBUS_PIN 19
#define USE_SERIAL
#define SERIAL_PIO_USE_PIO0
#define SERIAL_USART_TX_PIN GP1

/* Split detect method */
// by eeprom  and flashing option
//#define EE_HANDS
// by usb

#define MASTER_LEFT
/* Reset. */
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP17
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 1000U

// for oled display
#define OLED_IC  OLED_IC_SSD1306

#define OLED_CS_PIN 29
#define OLED_DC_PIN 28
#define OLED_DISPLAY_128X32 TRUE
