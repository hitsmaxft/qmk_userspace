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
