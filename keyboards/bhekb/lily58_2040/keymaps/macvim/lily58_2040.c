
#include "lily58_2040.h"

#ifdef OLED_ENABLE
#    include "oled_driver.h"
#endif

#ifdef FLOW_TAP_TERM

// dual hand flow tap
static float        border_for_splithand     = MATRIX_ROWS / 2.0 + 0.5;
static uint16_t     flow_tap_prev_time       = 0;
static uint16_t     flow_tap_prev_keycode    = KC_NO;
static keyrecord_t *last_tap_flow_key_record = 0;
static int          logo_show_delay          = 0;

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
