#include "tap_hold.h"
#include "quantum.h"

#ifdef FLOW_TAP_TERM

// Flow tap variables
static float        border_for_splithand     = MATRIX_ROWS / 2.0 + 0.5;
static uint16_t     flow_tap_prev_time       = 0;
static uint16_t     flow_tap_prev_keycode    = KC_NO;
static keyrecord_t *last_tap_flow_key_record = 0;

bool is_opposite_hand_tap_flow(keyrecord_t *last, keyrecord_t *current) {
    return (last->event.key.row < border_for_splithand && current->event.key.row > border_for_splithand) || (last->event.key.row > border_for_splithand && current->event.key.row < border_for_splithand);
}

void reset_chordal_flow_tap(void) {
    flow_tap_prev_time       = 0;
    last_tap_flow_key_record = 0;
    flow_tap_prev_keycode    = KC_NO;
}

uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    if (is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode)) {
        if (last_tap_flow_key_record != 0 && prev_keycode == flow_tap_prev_keycode && is_opposite_hand_tap_flow(last_tap_flow_key_record, record)) {
#    ifdef FLOW_TAP_DEBUG
            dprintf("tap_flow disable by chordal");
#    endif
            reset_chordal_flow_tap();
            return 0; // disable tap flow for chord hand
        }
        // save last pressed key for next checking
        flow_tap_prev_time       = record->event.time;
        last_tap_flow_key_record = record;
        return FLOW_TAP_TERM_MS;
    }
    reset_chordal_flow_tap();
    return 0;
}

#endif // FLOW_TAP_TERM

// Shift tab variables
bool     shift_tab_active      = false;
uint16_t shift_tab_timer_value = 0;

void stop_shift_hold(void) {
    if (shift_tab_active) {
        shift_tab_active      = false;
        shift_tab_timer_value = 0;
        unregister_code(KC_LSFT);
    }
}

bool process_tap_hold_keycode(uint16_t keycode, keyrecord_t *record) {
    // This function is called from process_record_user() for tap/hold related keycodes
    // Currently handled in custom_keycodes.c
    return false;
}

void tap_hold_scan(void) {
    if (shift_tab_active) {
        // auto remove shift hold after 1000ms
        if (timer_elapsed(shift_tab_timer_value) > SHIFT_TAB_TIMEOUT_MS) {
            unregister_code(KC_LSFT);
            shift_tab_active = false;
        }
    }
}
