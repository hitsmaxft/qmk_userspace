#include QMK_KEYBOARD_H

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


