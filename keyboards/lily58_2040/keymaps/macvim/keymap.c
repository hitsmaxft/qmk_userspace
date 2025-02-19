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

#include "keycodes.h"
#include "quantum_keycodes.h"
#include QMK_KEYBOARD_H


#include "keymap_us.h"
#include "send_string_keycodes.h"

#include "lily.h"

#ifdef OLED_ENABLE
#    include "oled_driver.h"

#endif

#include "keycode.h"

#define XXXXXX KC_NO

enum lily_58_custom_keycode {
    // vim yank
    UK_VYANK = SAFE_RANGE,
    UK_SCRCAP,
    UK_SFTTAB,
};

#define LBASE 0
#define LRAISE 1
#define LLOWER 2
#define LLW 2
#define LFUNC 3
#define LFN 3
#define LDEBUG 4
#define LNUM 5

// right shift mod
#define TRS_GRV RSFT_T(KC_GRV)

#define TU_BSPC LT(LRAISE, KC_BSPC)
#define LCT_A LCTL_T(KC_A)
#define LAT_S LALT_T(KC_S)
#define LGT_D LGUI_T(KC_D)
#define LST_F LSFT_T(KC_F)

#define RCT_SC RCTL_T(KC_SCLN)
#define RAT_L RALT_T(KC_L)
#define RGT_K RGUI_T(KC_K)
#define RST_J RSFT_T(KC_J)

// for tab
#define ST_MINS RSFT_T(KC_MINS)
#define ST_UNDS RSFT_T(KC_UNDS)
#define UK_SPC LT(LFUNC, KC_SPC)

static int logo_show_delay = 50;


bool is_shift_tab_active = false; // ADD this near the beginning of keymap.c
uint16_t shift_tab_timer = 0;     // we will be using them soon.


bool is_master = false;


uint32_t hide_logo(uint32_t trigger_time, void *cb_arg) {
    /* do something */
    logo_show_delay = 0;
    reset_keylogs_str();
    return 0;
}

void keyboard_post_init_user(void) {
    is_master = is_keyboard_master();

#ifdef OLED_ENABLE
    oled_write(read_logo(), false);
    defer_exec(3000, hide_logo, NULL);
    defer_exec(OLED_APM_INTERVAL, apm_calc_result, NULL);
#endif
}

#ifdef OLED_ENABLE

bool shutdown_user(bool jump_to_bootloader) {
    logo_show_delay = 1;
    oled_write(read_logo(), false);
    return true;
}

bool oled_task_user(void) {
    char charbuffer[21] = {0};

    if (logo_show_delay > 0) {
        return false;
    }

    if (!is_keyboard_master()) {
        oled_write(read_logo(), false);
        return false;
    }

    if (debug_enable) {
        oled_write_ln_P(PSTR(read_keylog()), false);
    } else {
        oled_write_ln_P(PSTR("Lily58 QMK"), false);
    }

    uint16_t layer_id = get_highest_layer(layer_state);

    const char *layer_name = "  ";
    switch (layer_id) {
        case LBASE:
            layer_name = "Def";
            break;
        case LLOWER:
            layer_name = "LOW";
            break;
        case LRAISE:
            layer_name = "UP";
            break;
        case LFUNC:
            layer_name = "FN";
            break;
        case LDEBUG:
            layer_name = "DBG";
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            layer_name = "";
    }
    sprintf(charbuffer, "%3s ", layer_name);
    oled_write_P(PSTR(charbuffer), false);

    sprintf(charbuffer, "APM: %3li\n", apm_read_keycode());
    oled_write_P(PSTR(charbuffer), false);

    oled_write_P(PSTR(read_keylogs()), false);

    return false;
}
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef OLED_ENABLE
    if (record->event.pressed) {
        logo_show_delay = 0;
        apm_incr_key_counter();
        set_keylog(keycode, record);
    }
#endif

    switch (keycode) {
        case KC_TAB:
            if (is_shift_tab_active ) {
                is_shift_tab_active = false;
                shift_tab_timer = 0;
                unregister_code(KC_LSFT);
            }
            break;
        case UK_VYANK:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LSFT) SS_TAP(X_QUOT) SS_DELAY(200) SS_UP(X_LSFT) SS_DELAY(100) SS_TAP(X_KP_PLUS) "y" SS_DELAY(300));
            } else {
            }
            break;
        case UK_SCRCAP:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LGUI) SS_DOWN(X_LSFT) SS_DOWN(X_LCTL) "4" SS_UP(X_LGUI) SS_UP(X_LSFT) SS_UP(X_LCTL));
            }
            break;
        case UK_SFTTAB:
            if (record->event.pressed) {
                if (!is_shift_tab_active) {
                    is_shift_tab_active = true;
                    register_code(KC_LSFT);
                }
                shift_tab_timer = timer_read();
                register_code(KC_TAB);
            } else {
                unregister_code(KC_TAB);
            }
            //if (record->event.pressed) {
            //    SEND_STRING(SS_DOWN(X_LSFT) SS_DOWN(X_TAB) SS_DELAY(200) SS_UP(X_LSFT) SS_UP(X_TAB));
            //}
            break;
    }
    return true;
};

void matrix_scan_user(void) { // The very important timer.
  if (is_shift_tab_active) {
    if (timer_elapsed(shift_tab_timer) > 1000) {
      unregister_code(KC_LSFT);
      is_shift_tab_active = false;
    }
  }
}
// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[LBASE] = LAYOUT(
  QK_GESC, KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_BSPC,
  KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_BSLS,
  KC_LCTL, LCT_A  , LAT_S  , LGT_D  , LST_F  , KC_G   ,                   KC_H   , RST_J  , RGT_K  , RAT_L  , RCT_SC , KC_QUOT,
  KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   ,UK_VYANK, UK_SCRCAP,KC_N  , KC_M   , KC_COMM, KC_DOT,KC_SLSH,  TRS_GRV,
                             KC_CAPS_LOCK, KC_TAB , MO(LLW), UK_SPC,  KC_ENT , MO(LRAISE), KC_NO , MO(LNUM)
),


[LRAISE] = LAYOUT(
  KC_NO  , KC_TILD, KC_EXLM, KC_AT  , KC_HASH, KC_DLR ,                   KC_PERC, KC_CIRC, KC_AMPR, KC_UNDS, KC_PLUS, _______,
  KC_NO  , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_PIPE,
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_F6  , KC_MINS, KC_EQL , KC_LBRC, KC_RBRC, KC_NO ,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_F10 , KC_F12 , _______, _______, KC_LBRC, KC_RBRC, KC_LABK, KC_RABK, KC_QUES, TRS_GRV ,
                             _______, _______, _______, _______, _______, KC_NO  , KC_NO  , KC_NO
),

[LLOWER] = LAYOUT(
  KC_NO  , KC_TILD, KC_EXLM, KC_AT  , KC_HASH, KC_DLR ,                   KC_PERC, KC_CIRC, KC_AMPR, KC_UNDS, KC_PLUS, _______,
  MO(LFN), KC_EXLM, KC_AT  , KC_HASH, KC_DLR , KC_PERC,                   KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_PIPE,
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_BSPC, KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_ENT ,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_TAB , KC_F6  , KC_LBRC, KC_RBRC, KC_LCBR, KC_RCBR, KC_COMM,  KC_DOT , KC_SLSH , KC_TILD,
                             _______, _______, _______, KC_SPC , KC_ENT , _______, _______, _______
),
[LFUNC] = LAYOUT(
  KC_NO   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  MO(LDEBUG),KC_NO , KC_NO , KC_MS_U, KC_PGUP, KC_PGDN ,                   KC_NO  , KC_MS_WH_UP,  KC_MS_WH_DOWN, KC_NO, KC_P , KC_MINS,
  KC_NO  , KC_NO   , KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT , KC_DOWN, KC_UP  , KC_RIGHT, KC_SCLN, KC_QUOT,
  KC_NO  , KC_NO   , KC_NO  , KC_NO  , KC_HOME, KC_END , KC_LBRC, KC_RBRC, KC_MS_BTN1, KC_MS_BTN2, KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______ , KC_SPC , KC_ENT  ,KC_ACL0, KC_ACL1, KC_ACL2
),
[LDEBUG] = LAYOUT(
  KC_F   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  KC_NO  , KC_NO  , KC_NO  , KC_MS_U, KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_MINS,
  KC_NO  , KC_MS_L, KC_MS_L, DB_TOGG, KC_MS_R, KC_F6  ,                   KC_LEFT, KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_HOME, QK_BOOT, KC_LBRC, KC_RBRC, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , KC_NO  , KC_NO  , KC_NO
),
[LNUM] = LAYOUT(
  KC_NO  , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  KC_NO  , KC_NO  , KC_NO  , KC_7   , KC_8   , KC_9   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_MINS,
  KC_NO  , KC_NO  , KC_NO  , KC_4   , KC_5  , KC_6   ,                   KC_LEFT, KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_1   , KC_2   , KC_3   , KC_LBRC, KC_RBRC, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , KC_NO  , KC_NO  , KC_NO
),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(   KC_PGUP , KC_PGDN ), ENCODER_CCW_CW(  UK_SFTTAB, KC_TAB) },
    [1] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [2] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [3] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_MS_WH_UP,   KC_MS_WH_DOWN) },
    [4] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [5] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
};
