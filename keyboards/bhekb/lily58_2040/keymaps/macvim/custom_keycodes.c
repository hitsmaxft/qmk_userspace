#include "custom_keycodes.h"
#include "tap_hold.h"
#include "quantum.h"

/**
 * Custom keycode implementations for Lily58 2040 keymap
 */

/**
 * Process custom keycodes
 * @param keycode Keycode to process
 * @param record Key record
 * @return true if keycode was handled
 */
bool process_custom_keycode(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case CUSTOM_VIM_YANK:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LSFT) SS_TAP(X_QUOT) SS_DELAY(200) SS_UP(X_LSFT) SS_DELAY(100) SS_TAP(X_KP_PLUS) "y" SS_DELAY(300));
            }
            return true;

        case CUSTOM_CAPTURE_REGION:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LGUI) SS_DOWN(X_LSFT) SS_DOWN(X_LCTL) "4" SS_UP(X_LGUI) SS_UP(X_LSFT) SS_UP(X_LCTL));
            }
            return true;

        case CUSTOM_SHIFT_TAB:
            if (record->event.pressed) {
                // Keep hold shift
                if (!shift_tab_active) {
                    shift_tab_active = true;
                    register_code(KC_LSFT);
                }
                shift_tab_timer_value = timer_read();
                register_code(KC_TAB);
            } else {
                unregister_code(KC_TAB);
            }
            return true;

        default:
            return false;
    }
}
