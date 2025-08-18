
#include "config.h"
#include "quantum.h"
#include "custom_keycodes.h"
#include "action_tapping.h"
#include "tap_hold.h"
#include "oled_display.h"
#include "community_modules.h"
#include "community_modules_introspection.h"

#ifdef FLOW_TAP_TERM_MS

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

#endif

bool     is_shift_tab_active = false; // ADD this near the beginning of keymap.c
uint16_t shift_tab_timer     = 0;     // we will be using them soon.

bool is_master = false;

void keyboard_post_init_user(void) {
#ifdef OLED_ENABLE
    oled_display_init();
#endif
    // Customise these values to desired behaviour
    /* The lines `debug_enable = true;` and `debug_matrix = true;` are setting two variables to `true`. */
    debug_enable = true;
    debug_matrix = true;
    // debug_keyboard=true;
    // debug_mouse=true;
}

#ifdef OLED_ENABLE
bool oled_task_user(void) {
    return oled_display_update();
}
#endif

#ifdef CHORDAL_HOLD
// Auto define left and right hand keys
// clang-format off
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
    '*', '*', '*', '*', '*', '*',                     '*', '*', '*', '*', '*',  '*',
    '*', '*', 'L', 'L', 'L', '*',                     '*', 'R', 'R', 'R', '*',  '*',
    '*', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R',  '*',
    '*', '*', '*', 'L', 'L', 'L',  '*', '*',  'R', 'R', 'R', '*', '*', '*',
                               '*', 'L', 'L',  'L', 'R',  'R', '*', '*'
);
// clang-format on
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef OLED_ENABLE
    if (record->event.pressed) {
        apm_incr_key_counter();
        set_keylog(keycode, record);
    }
#endif

    // Processed custom keycodes
    if (process_custom_keycode(keycode, record)) {
        return true;
    }

    return true;
};

void matrix_scan_user(void) {
    tap_hold_scan();
}
