#include <stdint.h>
#include QMK_KEYBOARD_H
#include "keycodes.h"
#include "keymap_us.h"
#include "quantum_keycodes.h"
#include "send_string_keycodes.h"
#include "lily.h"

#ifdef OLED_ENABLE
#include "oled_driver.h"

#endif

#include "keycode.h"

#define XXXXXX KC_NO

enum lily_58_custom_keycode {
    // vim yank
    UK_VYANK = SAFE_RANGE,
    UK_SCRCAP
};

#define LBASE 0
#define LRAISE 1
#define LLOWER 2
#define LLW 2
#define LFUNC 3
#define LFN 3
#define LDEBUG 4

#define TRS_GRV MT(MOD_RSFT, KC_GRV)
#define TU_BSPC LT(LRAISE, KC_BSPC)
#define LCT_A LCTL_T(KC_A)
#define LAT_S LALT_T(KC_S)
#define LGT_D LGUI_T(KC_D)
#define LST_F LSFT_T(KC_F)

#define RCT_SC RCTL_T(KC_SCLN)
#define RAT_L RALT_T(KC_L)
#define RGT_K RGUI_T(KC_K)
#define RST_J RSFT_T(KC_J)
#define UK_SPC LT(LFUNC, KC_SPC)

#define OLED_APM_INTERVAL 60000

static int logo_show_delay = 50;

static uint16_t press_key_count = 0;
static uint8_t keycode_apm = 0;

bool is_master = false;

uint32_t calc_apm(uint32_t trigger_time, void *cb_arg) {
    /* do something */
    keycode_apm = press_key_count;
    press_key_count = 0;
    return 60000;
}

uint32_t hide_logo(uint32_t trigger_time, void *cb_arg) {
    /* do something */
    logo_show_delay = 0;
    return 0;
}

void keyboard_post_init_user(void) {

    is_master = is_keyboard_master();

#ifdef OLED_ENABLE
    oled_write(read_logo(), false);
    defer_exec(3000, hide_logo, NULL);
    defer_exec(60000, calc_apm, NULL);
#endif
}

#ifdef OLED_ENABLE

bool shutdown_user(bool jump_to_bootloader) {
    logo_show_delay=1;
    oled_write(read_logo(), false);
    return true;

}

bool oled_task_user(void)  {
    char charbuffer[21]  = {0};

    if (logo_show_delay> 0) {
        return false;
    }

    if (!is_keyboard_master()) {
        oled_write(read_logo(), false);
        return false;
    }

    uint16_t layer_id = get_highest_layer(layer_state);

    sprintf(charbuffer, "apm: %d ", keycode_apm);

    oled_write_P(PSTR(charbuffer), false);
    oled_write_P(PSTR(" "), false);
    switch (layer_id) {
        case LBASE:
            oled_write_P(PSTR("Default\n"), false);
            break;
        case LLOWER:
            oled_write_P(PSTR("LOWER\n"), false);
            break;
        case LRAISE:
            oled_write_P(PSTR("RAISE\n"), false);
            break;
        case LFUNC:
            oled_write_P(PSTR("FUNC\n"), false);
            break;
        case LDEBUG:
            oled_write_P(PSTR("DEBUG\n"), false);
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            oled_write_P(PSTR("Undefined\n"), false);
    }

    oled_write_ln_P(PSTR(read_keylog()), false);
    oled_write_ln_P(PSTR(read_keylogs()), false);


    return false;
}
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef OLED_ENABLE
    if (record->event.pressed) {
        logo_show_delay = 0;
        press_key_count ++;
        set_keylog(keycode, record);
    }
#endif

    switch (keycode) {
        case UK_VYANK:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LSFT) SS_TAP(X_QUOT) SS_DELAY(200) SS_UP(X_LSFT) SS_TAP(X_KP_PLUS) "y");
            } else {
            }
            break;
        case UK_SCRCAP:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LGUI) SS_DOWN(X_LSFT) SS_DOWN(X_LCTL) "4" SS_UP(X_LGUI) SS_UP(X_LSFT) SS_UP(X_LCTL));
            }
            break;
    }
    return true;
};
// Default keymap. This can be changed in Vial. Use oled.c to change beavior that Vial cannot change.

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[LBASE] = LAYOUT(
  KC_ESC , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_BSPC,
  KC_TAB , KC_Q   , KC_W   , KC_E   , KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_BSLS,
  KC_LCTL, LCT_A  , LAT_S  , LGT_D  , LST_F  , KC_G   ,                   KC_H   , RST_J  , RGT_K  , RAT_L  , RCT_SC , KC_QUOT,
  KC_LSFT, KC_Z   , KC_X   , KC_C   , KC_V   , KC_B   ,UK_VYANK, UK_SCRCAP,  KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, TRS_GRV,
                             KC_LALT, KC_LGUI, MO(LLW), UK_SPC,  KC_ENT , MO(LRAISE), KC_LBRC, KC_RBRC
),


[LRAISE] = LAYOUT(
  KC_NO  , KC_TILD, KC_EXLM, KC_AT  , KC_HASH, KC_DLR ,                   KC_PERC, KC_CIRC, KC_AMPR, KC_UNDS, KC_PLUS, _______,
  KC_NO  , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_PIPE,
  _______, KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_NO  , KC_MINS, KC_EQL , KC_LBRC, KC_RBRC, KC_NO ,
  _______, KC_F7  , KC_F8  , KC_F9  , KC_NO  , KC_F6  , _______, _______, KC_N   , KC_M   , KC_LABK, KC_RABK, KC_QUES, _______,
                             _______, _______, _______, _______, _______, KC_NO  , KC_NO  , KC_NO
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
 MO(LDEBUG), KC_NO  , KC_NO , KC_MS_U, KC_PGUP, KC_PGDN ,                  KC_NO  , KC_MS_WH_UP,  KC_MS_WH_DOWN, KC_NO, KC_P , KC_MINS,
  KC_NO  , KC_MS_L, KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT , KC_DOWN, KC_UP  , KC_RIGHT, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_HOME, KC_END , KC_LBRC, KC_RBRC, KC_MS_BTN1, KC_MS_BTN2, KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , KC_ACL0, KC_ACL1, KC_ACL2
),
[LDEBUG] = LAYOUT(
  KC_F   , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                   KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_GRV ,
  KC_NO  , KC_NO  , KC_NO  , KC_MS_U, KC_R   , KC_T   ,                   KC_Y   , KC_U   , KC_I   , KC_O   , KC_P   , KC_MINS,
  KC_NO  , KC_MS_L, KC_MS_L, KC_MS_D, KC_MS_R, KC_F6  ,                   KC_LEFT, KC_DOWN, KC_UP  , KC_DOWN, KC_SCLN, KC_QUOT,
  KC_NO  , KC_Z   , KC_X   , KC_C   , KC_HOME, QK_BOOT, KC_LBRC, KC_RBRC, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SLSH, KC_RSFT,
                             KC_LALT, KC_LGUI, _______, KC_SPC , KC_ENT , KC_NO  , KC_NO  , KC_NO
),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(   KC_PGUP , KC_PGDN ), ENCODER_CCW_CW(   LSFT(KC_TAB), KC_TAB) },
    [1] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [2] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [3] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
    [4] = { ENCODER_CCW_CW(   KC_NO,    KC_NO), ENCODER_CCW_CW(   KC_NO,   KC_NO) },
};


