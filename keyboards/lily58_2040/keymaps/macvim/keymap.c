// im
// clang-format off

#include "keycodes.h"
#include "keymap_us.h"
#include QMK_KEYBOARD_H

#include "keycode.h"

#define XXXXXX KC_NO

#define LBASE 0
#define LRAISE 1
#define LLOWER 2
#define LLW 2
#define LFUNC 3
#define LFN 3
#define LDEBUG 4

#define TRS_GRV MT(MOD_RSFT, KC_GRV)
#define TU_BSPC LT(LRAISE,KC_BSPC)
#define LCT_A LCTL_T(KC_A)
#define LAT_S LALT_T(KC_S)
#define LGT_D LGUI_T(KC_D)
#define LST_F LSFT_T(KC_F)

#define RCT_SC RCTL_T(KC_SCLN)
#define RAT_L RALT_T(KC_L)
#define RGT_K RGUI_T(KC_K)
#define RST_J RSFT_T(KC_J)
#define UK_SPC LT(LFUNC, KC_SPC)

// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[LBASE] = LAYOUT(
  KC_ESC , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_BSPC,
  KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_BSLS,
  KC_LCTL, LCT_A  , LAT_S  , LGT_D  , LST_F  , KC_G   ,                   KC_H   , RST_J  , RGT_K  , RAT_L  , RCT_SC , KC_QUOT,
  KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   , KC_NO , KC_ENT,  KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, TRS_GRV,
                             KC_LALT, KC_LGUI, MO(LLW), UK_SPC,  KC_ENT , MO(LRAISE), KC_LBRC, KC_RBRC
),


[LRAISE] = LAYOUT(
  KC_NO  , KC_TILD, KC_EXLM, KC_AT  , KC_HASH, KC_DLR ,                   KC_PERC, KC_CIRC, KC_AMPR, KC_UNDS, KC_PLUS, _______,
  KC_NO  , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_PIPE,
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_NO  , KC_MINS, KC_EQL , KC_LBRC, KC_RBRC, KC_NO ,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_NO  , KC_F6  , _______, _______, KC_N   , KC_M   , KC_LABK, KC_RABK, KC_QUES, _______,
                             _______, _______, _______, _______, _______, _______, _______, _______
),

[LLOWER] = LAYOUT(
  _______, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_F7  , KC_F8  , KC_MINS, KC_EQL , KC_GRV ,
  MO(LFN), KC_EXLM, KC_AT  , KC_HASH, KC_DLR , KC_PERC,                   KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_PIPE,
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_BSPC, KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_QUOT,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_NO  , KC_F6  , KC_LBRC, KC_RBRC, KC_ENT,  KC_NO, KC_COMM,   KC_NO  , KC_NO  , KC_TILD,
                             _______, _______, _______, KC_SPC , KC_ENT , _______, _______, _______
),
[LFUNC] = LAYOUT(
  KC_NO   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
 MO(LDEBUG), KC_NO  , KC_NO , KC_MS_U, KC_PGUP, KC_PGDN ,                  KC_Y   , KC_U   , KC_MS_WH_UP,  KC_MS_WH_DOWN, KC_P   , KC_MINS,
  KC_NO  , KC_MS_L, KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT , KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_HOME, KC_END , KC_LBRC, KC_RBRC, KC_MS_BTN1, KC_MS_BTN2, KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , _______, _______, _______
),
[LDEBUG] = LAYOUT(
  KC_F   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  KC_NO  , KC_NO  , KC_NO  , KC_MS_U, KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_MINS,
  KC_NO  , KC_MS_L, KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT, KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_HOME, QK_BOOT, KC_LBRC, KC_RBRC, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , _______, _______, _______
),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(   KC_TAB,   LSFT(KC_TAB)), ENCODER_CCW_CW(   KC_PGUP ,    KC_PGDN) },
    [1] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [2] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [3] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [4] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
};
