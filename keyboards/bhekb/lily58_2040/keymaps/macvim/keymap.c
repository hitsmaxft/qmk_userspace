/*
 * Copyright (c) 2024 BHE
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
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

#include QMK_KEYBOARD_H

#include <stdbool.h>
#include "action.h"
#include "info_config.h"
#include "keycodes.h"
#include "quantum_keycodes.h"
#include "keymap_us.h"

#ifdef OLED_ENABLE
#    include "oled_driver.h"
#endif

#include "community_modules.h"
#include "community_modules_introspection.h"

#define XXXXXX KC_NO

enum lily_58_custom_keycode {
    // vim yank
    UK_VGCP = SAFE_RANGE,
    UK_CAPR,
    UK_STAB,
};

#define LBASE 0

#define BASE_MAC 0
#define BASE_WIN 1

#define LLOWER 2
#define LLW 2
#define LRAISE 3

// adjust layer
#define LFUNC 4
#define ADJUST 4
#define LFN 5
#define LNAVI 5
#define LNUM 6
#define LDEBUG 7

// right shift mod
#define TRS_GRV RSFT_T(KC_GRV)

#define TU_BSPC LT(LRAISE, KC_BSPC)

#define HRM_A(kc) LCTL_T(kc)
#define HRM_S(kc) LALT_T(kc)
#define HRM_D(kc) LGUI_T(kc)
#define HRM_F(kc) LSFT_T(kc)

#define HRM_C(kc) RCTL_T(kc)
#define HRM_L(kc) RALT_T(kc)
#define HRM_K(kc) RGUI_T(kc)
#define HRM_J(kc) RSFT_T(kc)

#define LCT_A LCTL_T(KC_A)
#define LAT_S LALT_T(KC_S)
#define LGT_D LGUI_T(KC_D)
#define LST_F LSFT_T(KC_F)

#define RCT_SC RCTL_T(KC_SCLN)
#define RAT_L RALT_T(KC_L)
#define RGT_K RGUI_T(KC_K)
#define RST_J RSFT_T(KC_J)

#define LCT_D LCTL_T(KC_D)
#define RCT_K RCTL_T(KC_K)

// hrm layout mapping
// clang-format off
#define LAYOUT_HRM(k0A, k0B, k0C, k0D, k0E, k0F, k5F, k5E, k5D, k5C, k5B, k5A, k1A, k1B, k1C, k1D, k1E, k1F, k6F, k6E, k6D, k6C, k6B, k6A, k2A, k2B, k2C, k2D, k2E, k2F, k7F, k7E, k7D, k7C, k7B, k7A, k3A, k3B, k3C, k3D, k3E, k3F, k4F, k9F, k8F, k8E, k8D, k8C, k8B, k8A, k4B, k4C, k4D, k4E, k9E, k9D, k9C, k9B)                                              \
    {                                                                                                                                                                                                                                                                                                                                                             \
        {k0A, k0B, k0C, k0D, k0E, k0F}, \
        {k1A, HRM_A(k1B), k1C, k1D, k1E, k1F}, \
        {k2A, HRM_A(k2B), HRM_S(k2C), HRM_D(k2D), HRM_F(k2E), k2F}, \
        {k3A, HRM_D(k3B), k3C, k3D, k3E, k3F}, \
        {XXX, k4B, k4C, k4D, k4E, k4F}, \
        {k5A, k5B, k5C, k5D, k5E, k5F}, \
        {k6A, k6B, k6C, k6D, k6E, k6F}, \
        {k7A, HRM_C(k7B), HRM_L(k7C), HRM_K(k7D), HRM_J(k7E), k7F}, \
        {k8A, k8B, k8C, k8D, k8E, k8F},  \
        { XXX, k9B, k9C, k9D, k9E, k9F                                                                                                                                                                                                                                                                                                                          \
        }                                                                                                                                                                                                                                                                                                                                                         \
    }

// clang-format on
// for tab
#define ST_MINS RSFT_T(KC_MINS)
#define ST_UNDS RSFT_T(KC_UNDS)
#define UK_SPC LT(LNAVI, KC_SPC)

static int logo_show_delay = 50;

#ifdef FLOW_TAP_TERM

// dual hand flow tap
static float        border_for_splithand     = MATRIX_ROWS / 2.0 + 0.5;
static uint16_t     flow_tap_prev_time       = 0;
static uint16_t     flow_tap_prev_keycode    = KC_NO;
static keyrecord_t *last_tap_flow_key_record = 0;

// ignore chordal tap flow
bool is_oppsite_hand_tap_flow(keyrecord_t *last, keyrecord_t *current) {
    return (last->event.key.row < border_for_splithand && current->event.key.row > border_for_splithand) || (last->event.key.row > border_for_splithand && current->event.key.row < border_for_splithand);
}
void flow_tap_chordal_reset(void) {
    flow_tap_prev_time       = 0;
    last_tap_flow_key_record = 0;
    flow_tap_prev_keycode    = KC_NO;
}

// disable tap flow while pressing with two hand
uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    if (is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode)) {
        if (last_tap_flow_key_record != 0 && prev_keycode == flow_tap_prev_keycode && is_oppsite_hand_tap_flow(last_tap_flow_key_record, record)) {
            // match prekeycode and check hand
#    ifdef FLOW_TAP_DEBUG
            dprintf("tap_flow disable by chordal");
#    endif // TAP_FLOW_DEBUG
            flow_tap_chordal_reset();
            return 0; // disable tap flow for chord hand
        }
        // save last pressed key for next checking
        flow_tap_prev_time       = record->event.time;
        last_tap_flow_key_record = record;
        return FLOW_TAP_TERM;
    }
    flow_tap_chordal_reset();
    return 0;
}
#endif

// combo start
// const uint16_t PROGMEM hj_combo1[] = {KC_H, RST_J, COMBO_END};
// const uint16_t PROGMEM fg_combo2[] = {LST_F, KC_G, COMBO_END};
combo_t key_combos[] = {
    // COMBO(hj_combo1, KC_BSPC),
    // COMBO(fg_combo2, KC_ESC), // keycodes with modifiers are possible too!
};

// combo end

bool     is_shift_tab_active = false; // ADD this near the beginning of keymap.c
uint16_t shift_tab_timer     = 0;     // we will be using them soon.

bool is_master = false;

uint32_t hide_logo(uint32_t trigger_time, void *cb_arg) {
    /* do something */
    logo_show_delay = 0;
    reset_keylogs_str();
    return 0;
}

void __keyboard_post_init_user(void) {
#ifdef OLED_ENABLE
    is_master = is_keyboard_master();

    // oled_write(read_logo(), false);
    defer_exec(3000, hide_logo, NULL);
    defer_exec(OLED_APM_INTERVAL, apm_calc_result, NULL);
#endif
}

#ifdef OLED_ENABLE
bool oled_task_user(void) {
    // oled_ext_oled_task_user();
    char charbuffer[21] = {0};

    // if (logo_show_delay > 0) {
    //     return false;
    // }
    // if (!is_keyboard_master()) {
    //     oled_write(read_logo(), false);
    //     return false;
    // }

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
            layer_name = "RAI";
            break;
        case LFUNC:
            layer_name = "ADJ";
            break;
        case LNAVI:
            layer_name = "NAV";
            break;
        case LNUM:
            layer_name = "NUM";
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

    return true;
}
#endif

#ifdef CHORDAL_HOLD
// auto define left and right hand keys
// clang-format off
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
    // FORMAT__START
    '*', '*', '*', '*', '*', '*',                     '*', '*', '*', '*', '*',  '*',
    '*', '*', 'L', 'L', 'L', '*',                     '*', 'R', 'R', 'R', '*',  '*',
    '*', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R',  '*',
    '*', '*', '*', 'L', 'L', 'L',  '*', '*',  'R', 'R', 'R', '*', '*', '*',
                               '*', 'L', 'L',  'L', 'R',  'R', '*', '*'
    // FORMAT__END
);

// clang-format on
#endif

void stop_shift_hold(void) {
    if (is_shift_tab_active) {
        is_shift_tab_active = false;

        shift_tab_timer = 0;
        unregister_code(KC_LSFT);
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef OLED_ENABLE
    if (record->event.pressed) {
        logo_show_delay = 0;
        apm_incr_key_counter();
        set_keylog(keycode, record);
    }
#endif

    bool stop_shift_hold = is_shift_tab_active;

    switch (keycode) {
        case UK_VGCP:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LSFT) SS_TAP(X_QUOT) SS_DELAY(200) SS_UP(X_LSFT) SS_DELAY(100) SS_TAP(X_KP_PLUS) "y" SS_DELAY(300));
            } else {
            }
            break;
        case UK_CAPR:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LGUI) SS_DOWN(X_LSFT) SS_DOWN(X_LCTL) "4" SS_UP(X_LGUI) SS_UP(X_LSFT) SS_UP(X_LCTL));
            }
            break;
        case UK_STAB:
            if (record->event.pressed) {
                // keep hold shift
                stop_shift_hold = false;
                if (!is_shift_tab_active) {
                    is_shift_tab_active = true;
                    register_code(KC_LSFT);
                }
                shift_tab_timer = timer_read();
                register_code(KC_TAB);
            } else {
                unregister_code(KC_TAB);
            }
            // if (record->event.pressed) {
            //     SEND_STRING(SS_DOWN(X_LSFT) SS_DOWN(X_TAB) SS_DELAY(200) SS_UP(X_LSFT) SS_UP(X_TAB));
            // }
            break;
    }

    // auto release shift hold for scrolling tab
    if (is_shift_tab_active && stop_shift_hold) {
        shift_tab_timer     = 0;
        is_shift_tab_active = false;
        unregister_code(KC_LSFT);
    }

    return true;
};

void matrix_scan_user(void) { // The very important timer.
    if (is_shift_tab_active) {
        // auto remove shift hold after 1000
        if (timer_elapsed(shift_tab_timer) > 1000) {
            unregister_code(KC_LSFT);
            is_shift_tab_active = false;
        }
    }
}
// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.

// clang-format off

//chordal hold layout
#define LN_TAB LT(LNUM, KC_TAB)
#define LN_ESC LT(LNUM, KC_ESC)

//define LN_TAB ,  ,
//define LN_TAB , , KC_TAB)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[BASE_MAC] = LAYOUT_HRM(
//FORMAT__START

QK_GESC ,KC_1    ,KC_2    ,KC_3    ,KC_4    ,KC_5    ,                        KC_6    ,KC_7    ,KC_8    ,KC_9    ,KC_0    ,KC_GRAVE,
KC_TAB  ,KC_Q    ,KC_W    ,KC_E    ,KC_R    ,KC_T    ,                        KC_Y    ,KC_U    ,KC_I    ,KC_O    ,KC_P    ,KC_BSLS ,
QK_GESC ,KC_A        ,KC_S        ,KC_D    ,KC_F    ,KC_G    ,                          KC_H    ,KC_J    ,KC_K    ,KC_L    ,KC_SCLN ,KC_QUOT ,
LN_TAB  ,KC_Z    ,KC_X    ,KC_C    ,KC_V    ,KC_B    ,UK_VGCP ,     UK_CAPR ,     KC_N    ,KC_M    ,KC_COMM ,KC_DOT  ,KC_SLSH ,KC_BSPC,
                                    KC_LSFT ,LN_ESC  ,TL_LOWR ,UK_SPC  ,KC_ENT  ,TL_UPPR ,KC_BSPC ,KC_RSFT
//FORMAT__END
//
),

[BASE_WIN] = LAYOUT(
  _______, _______, _______, _______, _______, _______,                   _______, _______, _______, _______, _______, _______,
  _______, _______, _______, _______, _______, _______,                   _______, _______, _______, _______, _______, _______,
  _______, LCT_A  , _______, LCT_D  , _______, _______,                   _______, _______, RCT_K  , _______, RCT_SC , _______,
  KC_LGUI, _______, _______, _______, _______, _______,_______, _______, _______, _______, _______, _______,_______, _______,
            _______, _______  ,  _______, _______, _______,_______, _______,  _______
),


// Symbol
[LLOWER] = LAYOUT(
//FORMAT__START
QK_GESC ,KC_1    ,KC_2    ,KC_3    ,KC_4    ,KC_5    ,                        KC_6    ,KC_7    ,KC_8    ,KC_9    ,KC_0    ,KC_BSPC ,
MO(LFN) ,KC_EXLM ,KC_AT   ,KC_HASH ,KC_DLR  ,KC_PERC ,                        KC_CIRC ,KC_AMPR ,KC_ASTR ,KC_LPRN ,KC_RPRN ,KC_PIPE ,
KC_NO   ,KC_NO   ,KC_LBRC ,KC_RBRC ,KC_LCBR ,KC_RCBR ,                        KC_EQL  ,KC_UNDS ,KC_MINS ,KC_PLUS ,KC_DQUO ,KC_EQL  ,
KC_NO   ,KC_TRNS ,KC_GRAVE,KC_TILD,KC_PIPE ,KC_BSLS ,KC_NO   ,KC_NO   ,KC_SLSH ,KC_QUES ,KC_LABK,   KC_RABK , KC_QUOT , _______,
                                    _______ ,MO(LDEBUG),_______ ,KC_NO   ,KC_GRAVE ,MO(LFUNC),KC_TILDE,_______
//FORMAT__END
),

// FN line and quick symbol
[LRAISE] = LAYOUT_HRM(
//FORMAT__START
QK_GESC ,KC_1    ,KC_2    ,KC_3    ,KC_4    ,KC_5    ,                        KC_6    ,KC_7    ,KC_8    ,KC_9    ,KC_0    ,KC_BSPC ,
KC_NO   ,KC_F1   ,KC_F2   ,KC_F3   ,KC_F4   ,KC_F5   ,                        KC_F6   ,KC_F7   ,KC_F8   ,KC_F9   ,KC_F10  ,KC_F11  ,
KC_NO   ,KC_5    ,KC_4    ,KC_3    ,KC_2    ,KC_1    ,                                        KC_BSPC ,KC_MINS ,KC_EQL  ,KC_MINS ,KC_EQL  ,KC_F12  ,
KC_CAPS ,KC_6    ,KC_7    ,KC_8    ,KC_9    ,KC_0    ,_______ ,_______ ,KC_TAB  ,KC_INS  ,KC_HOME ,KC_END  ,KC_QUES ,TRS_GRV ,
                                    _______ ,KC_TAB ,MO(LFUNC),KC_BSPC ,KC_DEL  ,KC_NO   ,KC_BSPC ,KC_NO
//FORMAT__END
),
//adjust layer
[LFUNC] = LAYOUT(
  KC_F   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   PDF(1) , PDF(0)  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_F11 , KC_F12 , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , QK_BOOT,QK_REBOOT,DB_TOGG, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
                             KC_LALT, KC_LGUI, _______, KC_NO   , KC_NO, KC_NO  , KC_NO  , KC_NO
),
[LNAVI] = LAYOUT(
  KC_NO   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_NO   , KC_NO  , KC_NO , KC_NO   , KC_NO   , KC_NO ,
  MO(LDEBUG),KC_NO , KC_NO , KC_MS_U , KC_PGUP, KC_PGDN ,                  KC_MS_BTN1, KC_MS_BTN3, KC_MS_BTN2, KC_MS_WH_UP, KC_MS_WH_DOWN, KC_NO,
  KC_NO  , KC_NO   , KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT , KC_DOWN, KC_UP  , KC_RIGHT, KC_NO , KC_NO,
  KC_NO  , KC_NO   , KC_NO  , KC_NO  , KC_HOME, KC_END , KC_LBRC, KC_RBRC, KC_MS_WH_DOWN, KC_MS_WH_UP, KC_NO, KC_NO, KC_NO, KC_NO,

                             KC_LALT, MO(LDEBUG), _______ , KC_NO , KC_BSPC,KC_ACL0, KC_ACL1, KC_ACL2
),
[LNUM] = LAYOUT(
  KC_NO  , KC_NO  , KC_7   , KC_8   , KC_9   , KC_NO  ,                             KC_NO  , KC_NO  , KC_NO  , KC_NO, KC_0   , KC_GRV ,
  KC_NO  , KC_NO  , KC_F10   , KC_F7   , KC_F8   , KC_F9  ,                         KC_7   , KC_8   , KC_9   , KC_KP_PLUS, KC_KP_MINUS   , KC_MINS,
  KC_NO  , KC_NO  , KC_F11   , KC_F4   , KC_F5   , KC_F6  ,                         KC_4   , KC_5   , KC_6   , KC_KP_ASTERISK, KC_KP_SLASH  , KC_QUOT,
  KC_NO  , KC_NO  , KC_F12   , KC_F1  , KC_F2  , KC_F3  , KC_NO , KC_NO,    KC_1   , KC_2   , KC_3   , KC_NO, KC_NO  , KC_RSFT,
                                            KC_NO , KC_NO  ,_______ , KC_NO , KC_0  ,  KC_DOT  , KC_MINUS   , KC_NO
),
[LDEBUG] = LAYOUT(
  KC_F   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , DB_TOGG,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
  KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , QK_BOOT,QK_REBOOT,DB_TOGG, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
                             KC_LALT, KC_LGUI, _______, KC_NO   ,           KC_NO, KC_NO  , KC_NO  , KC_NO
),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(   KC_PGUP , KC_PGDN ), ENCODER_CCW_CW(  UK_STAB, KC_TAB) },
    [1] = { ENCODER_CCW_CW(   KC_PGUP , KC_PGDN ), ENCODER_CCW_CW(  UK_STAB, KC_TAB) },
    [2] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [4] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [3] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_MS_WH_UP,   KC_MS_WH_DOWN) },
    [5] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [6] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [7] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
};
