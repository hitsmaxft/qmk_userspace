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

#include <stdint.h>
#include <stdio.h>
#include "action_layer.h"
#include "action_util.h"
#include "debug.h"
#include "keycodes.h"
#include "modifiers.h"
#include "printf.h"
#include "quantum.h"

#define CHAR_LAYER 0xDD
#define CHAR_MOD 0x1B
#define CHAR_BS 0xD9
#define CHAR_CMD 0xD5
#define CHAR_CTL 0xD6
#define CHAR_ALT 0xD7
#define CHAR_SHT 0xD8
#define CHAR_UP 0x18
#define CHAR_DOWN 0x19
#define CHAR_RIGHT 0x1A
#define CHAR_LEFT 0x1B

// show magic number
//  max char in one line
#define MAX_CHAR_LINE 21
#define KEY_LOGS_MAX_CHAR_LINE 21
// max history chars, reserved 2 char of each line for padding
#define KEY_LOGS_MAX_CHARS ((MAX_CHAR_LINE * 2))

#define KEY_LOGS_COUNT ((MAX_CHAR_LINE - MAX_CHAR_LINE % 2))
#define KEY_LOGS_COMBO_LINE ((KEY_LOGS_COUNT / 2))

#define KEY_LOG_ARR_LEN KEY_LOGS_MAX_CHARS

static char keylog_str[24]               = {};
static char keylogs_str[KEY_LOG_ARR_LEN] = {};

// deprecate
char       keylogs_mods[KEY_LOGS_COUNT] = {};
static int keylogs_str_idx              = 0;

static bool inited = false;

void reset_keylogs_str(void) {
    keylogs_str_idx = 0;
    for (int i = 0; i < KEY_LOGS_MAX_CHARS - 1; i++) {
        if (debug_enable) {
            keylogs_str[i] = '.';
        } else {
            keylogs_str[i] = ' ';
        }
    }
    keylogs_str[KEY_LOGS_MAX_CHAR_LINE - 1] = ' ';
    keylogs_str[KEY_LOGS_MAX_CHARS - 1]     = ' ';
}

#define SPC_NAME 0xDB
#define ENT_NAME 0xDE
#define TAB_NAME 0xDA
#define ESC_NAME 0x9E

const char code_to_name[60] = {' ', ' ', ' ', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ENT_NAME, ESC_NAME, 0xD9, TAB_NAME, SPC_NAME, ' ', ' ', ' ', ' ', ' ', ' ', ';', '\'', ' ', ',', '.', '/', ' ', ' ', ' '};

char find_keytable(uint16_t keycode) {
    char name = ' ';
    if (keycode < 0x3C) {
        name = code_to_name[keycode];
    } else {
        switch (keycode) {
            case KC_BSPC:
                name = 0xD9; // replace space to dot
                break;
            case KC_SPC:
                name = 0xDB; // replace space to dot
                break;
            case KC_UP:
                name = 0x18;
                break;
            case KC_DOWN:
                name = 0x19;
                break;
            case KC_RIGHT:
                name = 0x1A;
                break;
            case KC_LEFT:
                name = 0x1B;
                break;
            default:
                break;
        }
    }

    return name;
}

void process_mods(uint16_t *kc, keyrecord_t *record, int *tap_layer) {
    uint16_t keycode = *kc;

    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count) {
            *kc = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        } else {
            // keycode = 0xE0 + biton(QK_MOD_TAP_GET_MODS(keycode) & 0xF) + biton(QK_MOD_TAP_GET_MODS(keycode) & 0x10);
        }
    } else if (IS_QK_LAYER_TAP(keycode) && record->tap.count) {
        *kc = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        *tap_layer = QK_LAYER_TAP_GET_LAYER(keycode);
    } else if (IS_QK_MODS(keycode)) {
        // *kc = QK_MODS_GET_BASIC_KEYCODE(keycode);
    } else if (IS_QK_ONE_SHOT_MOD(keycode)) {
        // *kc = 0xE0 + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0xF) + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0x10);
    }
}

void set_keylog(uint16_t keycode, keyrecord_t *record) {
    if (!inited) {
        reset_keylogs_str();
        inited = 1;
    }

    int tap_layer = -1;

    process_mods(&keycode, record, &tap_layer);

    char name = ' ';
    char mod  = ' ';

    name = find_keytable(keycode);


    static char buffer [10] = {0};

    uint8_t mod_state = get_mods();
    char modGui  = (mod_state & MOD_MASK_GUI) ? 0xd5 : '-';
    char modCtl  = (mod_state & MOD_MASK_CTRL) ? 0xd6 : '-';
    char modAlt  = (mod_state & MOD_MASK_ALT) ? 0xd7 : '-';
    char modShft = (mod_state & MOD_MASK_SHIFT) ? 0x18 : '-';
    sprintf(buffer, "%c%c%c%c",modCtl, modShft, modAlt, modGui);

    // update keylog
    snprintf(keylog_str, sizeof(keylog_str), "%2d %d:%d,c=%06d %3s", keylogs_str_idx, record->event.key.row, record->event.key.col, keycode, buffer);

    if (tap_layer > -1) {
        if (tap_layer == 0) {
            name = find_keytable(KC_0);
        } else {
            name = find_keytable(KC_1 + (tap_layer - 1));
        }
        mod = CHAR_LAYER;
    } else {
        if (mod_state & MOD_MASK_GUI) {
            mod = 0xd5;
        } else if (mod_state & MOD_MASK_CTRL) {
            mod = 0xd6;
        } else if (mod_state & MOD_MASK_ALT) {
            mod = 0xd7;
        } else if (mod_state & MOD_MASK_SHIFT) {
            mod = 0xD8;
        } else {
            mod = ' ';
        }
    }

    int next_char = (mod != ' ') ? 2 : 1;

    if (keylogs_str_idx + next_char > KEY_LOGS_MAX_CHARS) {
        // reset to first index
        reset_keylogs_str();
    } else if (keylogs_str_idx < KEY_LOGS_MAX_CHAR_LINE && keylogs_str_idx + next_char > KEY_LOGS_MAX_CHAR_LINE) {
        // new line
        keylogs_str[keylogs_str_idx++] = 0xDB;
    }

    if (mod != ' ') {
        keylogs_str[keylogs_str_idx++] = mod;
    }

    keylogs_str[keylogs_str_idx++] = name;
}

const char *read_keylog(void) {
    return keylog_str;
}

const char *read_keylogs(void) {
    return keylogs_str;
}
const char *read_keymods(void) {
    return keylogs_mods;
}
