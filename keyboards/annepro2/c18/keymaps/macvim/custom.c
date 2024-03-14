#include QMK_KEYBOARD_H

#include "macvim.h"

void  reset_to_iap(void);


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
    case KC_AP_IAP:
      if (record->event.pressed) {
          reset_to_iap();
          return false;
      } else {
        // Do something else when release
      }
      return true; // Let QMK send the enter press/release events
    default:
      return true; // Process all other keycodes normally
  }
}

void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
  //debug_enable=true;
  //debug_matrix=true;
  //debug_keyboard=true;
  //debug_mouse=true;
}

void reset_to_iap(void) {
    //this will write eeprom make keyboard boot into iap mode
    bootloader_jump();
}


void leader_start_user(void) {
    // Do something when the leader key is pressed
}

void leader_end_user(void) {
    if (leader_sequence_two_keys(KC_G, KC_P)) {
        // Leader, f => Types the below string
        SEND_STRING("git push\n");
    } else if (leader_sequence_two_keys(KC_G, KC_U)) {
        SEND_STRING("git add -u\n");
        return;
    } else if (leader_sequence_three_keys(KC_G, KC_A, KC_I)) {
        // Leader, d, d, s => Types the below string
        SEND_STRING("git ai-commit\n");
    } else if (leader_sequence_two_keys(KC_D, KC_D)) {
        return;
    } else if (leader_sequence_two_keys(KC_A, KC_S)) {
        // Leader, a, s => GUI+S
        tap_code16(LGUI(KC_S));
    }
}
