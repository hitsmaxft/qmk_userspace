#include <stdint.h>
#include <stdio.h>
#include "action_layer.h"
#include "action_util.h"
#include "debug.h"
#include "keycodes.h"
#include "modifiers.h"
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

char keylogs_mods[KEY_LOGS_COUNT] = {};
static int  keylogs_str_idx              = 0;

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

void set_keylog(uint16_t keycode, keyrecord_t *record) {
    if (!inited) {
        reset_keylogs_str();
        inited = 1;
    }

    if (keylogs_str_idx >= KEY_LOGS_MAX_CHARS - 1) {
        // reset output
        reset_keylogs_str();
    } else if (keylogs_str_idx == KEY_LOGS_COUNT) {
        // add_sep
        keylogs_str_idx++;
    }

    int tap_layer = -1;

    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count) {
            keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        } else {
            // keycode = 0xE0 + biton(QK_MOD_TAP_GET_MODS(keycode) & 0xF) + biton(QK_MOD_TAP_GET_MODS(keycode) & 0x10);
        }
    } else if (IS_QK_LAYER_TAP(keycode) && record->tap.count) {
        keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        tap_layer = QK_LAYER_TAP_GET_LAYER(keycode);
    } else if (IS_QK_MODS(keycode)) {
        // keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    } else if (IS_QK_ONE_SHOT_MOD(keycode)) {
        // keycode = 0xE0 + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0xF) + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0x10);
    }

    char name = ' ';
    char mod  = ' ';

    name = find_keytable(keycode);

    uint8_t mod_state = get_mods();

    char modGui  = (mod_state & MOD_MASK_GUI) ? 0xd5 : ' ';
    char modCtl  = (mod_state & MOD_MASK_CTRL) ? 0xd6 : ' ';
    char modAlt  = (mod_state & MOD_MASK_ALT) ? 0xd7 : ' ';
    char modShft = (mod_state & MOD_MASK_SHIFT) ? 0x18 : ' ';

    // update keylog
    snprintf(keylog_str, sizeof(keylog_str), "%d %dx%d,k%2d:%c %c%c%c%c %d", keylogs_str_idx, record->event.key.row, record->event.key.col, keycode, name, modGui, modCtl, modAlt, modShft, keylogs_str_idx);

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

    keylogs_str[keylogs_str_idx++] = mod;
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
