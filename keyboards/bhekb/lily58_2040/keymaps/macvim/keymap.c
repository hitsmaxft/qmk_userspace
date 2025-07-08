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

#include "lily58_2040.h"
// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.

// clang-format off

//chordal hold layout

//define LN_TAB ,  ,
//define LN_TAB , , KC_TAB)

// combo start
// const uint16_t PROGMEM hj_combo1[] = {KC_H, RST_J, COMBO_END};
// const uint16_t PROGMEM fg_combo2[] = {LST_F, KC_G, COMBO_END};
#ifdef COMBO_ENABLE
combo_t key_combos[] = {
    // COMBO(hj_combo1, KC_BSPC),
    // COMBO(fg_combo2, KC_ESC), // keycodes with modifiers are possible too!
};
#endif
// combo end



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
