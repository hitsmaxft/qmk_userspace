#include <stdint.h>
#include <stdio.h>
#include "action_layer.h"
#include "action_util.h"
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


#define MAX_CHAR_LINE = 21
#define KEY_LOG_COUNT 20
#define KEY_LOGS_SEP ((KEY_LOG_COUNT/2))
#define KEY_LOG_ARR_LEN  ((KEY_LOG_COUNT*2 + 2))

char keylog_str[24]   = {};
char keylogs_str[ KEY_LOG_ARR_LEN]  = {};
char keylogs_mods[KEY_LOG_COUNT] = {};
int  keylogs_str_idx  = 0;

static bool inited = false;

void reset_keylogs_str(void) {
    keylogs_str_idx = 0;
    for (int i = 0; i < KEY_LOG_ARR_LEN - 1; i++) {
        keylogs_str[i]  = ' ';
    }
    keylogs_str[KEY_LOG_COUNT]  = '\n';
    keylogs_str[KEY_LOG_COUNT*2 + 1]  = '\n';
}


const char code_to_name[60] = {' ', ' ', ' ', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'R', 0x1C, 0x20, 'T', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ';', '\'', ' ', ',', '.', '/', ' ', ' ', ' '};


char find_keytable(uint16_t keycode) {

    char name = ' ';
    if (keycode < 0x3C) {
        name = code_to_name[keycode];
    } else {
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
    }


    return name;
}

void set_keylog(uint16_t keycode, keyrecord_t *record) {
    if (!inited) {
        reset_keylogs_str();
        inited = 1;
    }

    int tap_layer = -1;

    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count) {
          keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        } else {
          //keycode = 0xE0 + biton(QK_MOD_TAP_GET_MODS(keycode) & 0xF) + biton(QK_MOD_TAP_GET_MODS(keycode) & 0x10);
        }
    } else if (IS_QK_LAYER_TAP(keycode) && record->tap.count) {
        keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        tap_layer = QK_LAYER_TAP_GET_LAYER(keycode);
    } else if (IS_QK_MODS(keycode)) {
        //keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    } else if (IS_QK_ONE_SHOT_MOD(keycode)) {
        //keycode = 0xE0 + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0xF) + biton(QK_ONE_SHOT_MOD_GET_MODS(keycode) & 0x10);
    }

    char name = ' ';
    char mod = ' ';
    int add_sep = 0;

    name = find_keytable(keycode);

    uint8_t mod_state = get_mods();

    char modGui  = (mod_state & MOD_MASK_GUI) ? 0xd5 : ' ';
    char modCtl  = (mod_state & MOD_MASK_CTRL) ? 0xd6 : ' ';
    char modAlt  = (mod_state & MOD_MASK_ALT) ? 0xd7 : ' ';
    char modShft = (mod_state & MOD_MASK_SHIFT) ?  0x18 : ' ';

    // update keylog
    snprintf(keylog_str, sizeof(keylog_str), "%d %dx%d,k%2d:%c %c%c%c%c", keylogs_str_idx, record->event.key.row, record->event.key.col, keycode, name, modGui, modCtl, modAlt, modShft);

    if (keylogs_str_idx == KEY_LOG_COUNT) {
        // reset output
        keylogs_str_idx = 0;
        reset_keylogs_str();
        add_sep = 0;
    } else if (keylogs_str_idx  == KEY_LOGS_SEP) {
        //add new line
        add_sep = 1;
    }

    if (tap_layer >-1) {
        if (tap_layer == 0 )  {
            name = find_keytable(KC_0);
        } else {
            name = find_keytable(KC_1  + (tap_layer - 1));
        }
        mod  = CHAR_LAYER;
    } else {
        if (mod_state & MOD_MASK_GUI) {
            mod  = 0xd5;
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

    keylogs_str[keylogs_str_idx * 2 + add_sep ] = mod;
    keylogs_str[keylogs_str_idx * 2 + add_sep  + 1 ] = name;

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
