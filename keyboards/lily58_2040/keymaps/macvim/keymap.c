// im
// clang-format off

#include QMK_KEYBOARD_H

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


// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[LBASE] = LAYOUT(
  KC_ESC , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_BSPC,
  KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_BSLS,
  KC_LCTL, KC_A   , KC_S   , KC_D   , KC_F   , KC_G   ,                   KC_H   , KC_J   , KC_K   , KC_L   , KC_SCLN, KC_QUOT,
  KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   , KC_NO, KC_NO,     KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, TRS_GRV,
                             KC_LALT, KC_LGUI, MO(LLW), KC_SPC , KC_ENT     , TU_BSPC, KC_LBRC, KC_RBRC
),

[LLOWER] = LAYOUT(
  KC_NO  , KC_TILD, KC_EXLM, KC_AT  , KC_HASH, KC_DLR ,                   KC_PERC, KC_CIRC, KC_AMPR, KC_UNDS, KC_PLUS, _______,
  KC_NO  , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_LCBR, KC_RCBR, KC_PIPE,
  _______, KC_A   , KC_S   , KC_D   , KC_F   , KC_G   ,                   KC_H   , KC_J   , KC_K   , KC_L   , KC_COLN, KC_DQUO,
  _______, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   , _______, _______, KC_N   , KC_M   , KC_LABK, KC_RABK, KC_QUES, _______,
                             _______, _______, _______, _______, _______, _______, _______, _______
),
[LRAISE] = LAYOUT(
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_F6  , KC_F7  , KC_F8  , KC_MINS, KC_EQL , KC_GRV ,
  MO(LFN), KC_Q   , KC_MS_U, KC_E   , KC_R   , KC_T   ,                   KC_WH_U, KC_WH_D, KC_I   , KC_LBRC, KC_RBRC, KC_MINS,
  _______, KC_MS_L, KC_MS_D, KC_MS_R, KC_F   , KC_G   ,                   KC_LEFT, KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_NO  , KC_NO  , KC_LBRC, KC_RBRC, KC_BTN1, KC_BTN2, KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             _______, _______, _______, KC_SPC , KC_ENT , _______, _______, _______
),
[LFUNC] = LAYOUT(
  KC_F   , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  KC_NO  , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_MINS,
  KC_NO  , KC_A   , KC_S   , KC_D   , KC_F   , KC_G   ,                   KC_H   , KC_J   , KC_K   , KC_L   , KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_V   , QK_BOOT, KC_LBRC, KC_RBRC, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , _______, _______, _______
),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(   KC_A,    KC_B), ENCODER_CCW_CW(   KC_C ,    KC_D) },
    [1] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [2] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [3] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
};
