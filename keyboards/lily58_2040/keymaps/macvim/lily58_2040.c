
#include "custom_keycodes.h"
#include "layers.h"
#include "layouts.h"
#include "tap_hold.h"
#include "oled_display.h"
#include "community_modules.h"
#include "community_modules_introspection.h"

#ifdef OLED_ENABLE
#    include "oled_driver.h"
static int logo_show_delay = 0;
#endif

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
    oled_display_init();
#endif
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
        apm_incr_key_counter();
        set_keylog(keycode, record);
    }
#endif

    // Process custom keycodes
    if (process_custom_keycode(keycode, record)) {
        return true;
    }

    return true;
};

void matrix_scan_user(void) {
    tap_hold_scan();
}
