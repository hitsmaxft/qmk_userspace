#include <stdint.h>
#include <stdio.h>
#include "action_util.h"
#include "keycodes.h"
#include "modifiers.h"
#include "quantum.h"

char keylog_str[24]   = {};
char keylogs_str[21]  = {};
char keylogs_mods[21] = {};
int  keylogs_str_idx  = 0;

char find_keytable(uint16_t keycode) {
    char name = ' ';
    switch (keycode) {
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

    return name;
}

const char code_to_name[60] = {' ', ' ', ' ', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'R', 0x1C, 0x20, 'T', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ';', '\'', ' ', ',', '.', '/', ' ', ' ', ' '};

void set_keylog(uint16_t keycode, keyrecord_t *record) {
    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count) {
            keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        } else {
        }
    } else if (IS_QK_LAYER_TAP(keycode)) {
        if (record->tap.count) {
            keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
        } else {
        }
    }
    char name = ' ';

    if (keycode < 0x3C) {
        name = code_to_name[keycode];
    } else {
        name = find_keytable(keycode);
    }

    uint8_t mod_state = get_mods();

    char modGui  = (mod_state & MOD_MASK_GUI) ? 0xd5 : ' ';
    char modCtl  = (mod_state & MOD_MASK_CTRL) ? 0xd6 : ' ';
    char modAlt  = (mod_state & MOD_MASK_ALT) ? 0xd7 : ' ';
    char modShft = (mod_state & MOD_MASK_SHIFT) ?  0x18 : ' ';

    // update keylog
    snprintf(keylog_str, sizeof(keylog_str), "%dx%d,k%2d:%c %c%c%c%c", record->event.key.row, record->event.key.col, keycode, name, modGui, modCtl, modAlt, modShft);

    // update keylogs
    if (keylogs_str_idx == sizeof(keylogs_str) - 1) {
        keylogs_str_idx = 0;
        for (int i = 0; i < sizeof(keylogs_str) - 1; i++) {
            keylogs_str[i]  = ' ';
            keylogs_mods[i] = ' ';
        }
    }

    if (mod_state & MOD_MASK_GUI) {
        keylogs_mods[keylogs_str_idx] = 0xd5;
    } else if (mod_state & MOD_MASK_CTRL) {
        keylogs_mods[keylogs_str_idx] = 0xd6;
    } else if (mod_state & MOD_MASK_ALT) {
        keylogs_mods[keylogs_str_idx] = 0xd7;
    } else if (mod_state & MOD_MASK_SHIFT) {
        keylogs_mods[keylogs_str_idx] = 0x18;
    } else {
        keylogs_mods[keylogs_str_idx] = ' ';
    }

    keylogs_str[keylogs_str_idx] = name;

    keylogs_str_idx++;
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
