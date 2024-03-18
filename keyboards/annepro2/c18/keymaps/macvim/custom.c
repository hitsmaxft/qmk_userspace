#include QMK_KEYBOARD_H

#include "macvim.h"
#include "print.h"

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


void keyboard_pre_init_user(void) {
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

const ap2_led_t layer_color = {
    .p.blue  = 0xff,
    .p.red   = 0xff,
    .p.green = 0xff,
    .p.alpha = 0xff,
};


const ap2_led_t leader_blink_color = {
    .p.blue  = 0xff,
    .p.red   = 0x00,
    .p.green = 0x00,
    .p.alpha = 0xff,
};

void leader_start_user(void) {
    ap2_led_blink(4, 6, leader_blink_color, 3, 50);
    // Do something when the leader key is pressed
}

void leader_end_user(void) {
    ap2_led_blink(4, 6, leader_blink_color, 0, 50);

    if (leader_sequence_two_keys(KC_V, KC_Y)) {
        SEND_STRING("\"+y");
    } else if (leader_sequence_two_keys(KC_V, KC_P)) {
        SEND_STRING("\"+gp");
    } else if (leader_sequence_two_keys(KC_G, KC_P)) {
        SEND_STRING("git push\n");
    } else if (leader_sequence_two_keys(KC_G, KC_U)) {
        SEND_STRING("git add -u\n");
    } else if (leader_sequence_three_keys(KC_G, KC_A, KC_I)) {
        SEND_STRING("git ai-commit\n");
    } else if (leader_sequence_two_keys(KC_D, KC_D)) {
        return;
    } else if (leader_sequence_two_keys(KC_A, KC_S)) {
        // Leader, a, s => GUI+S
        tap_code16(LGUI(KC_S));
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer_num = get_highest_layer(state);

    dprintf("on layer %d\n", layer_num);

    if (IS_LAYER_ON_STATE(state, layer_num)) {
        dprintf("blink layer %d\n", layer_num);
        // light on number key
        ap2_led_blink(0, layer_num + 1, layer_color, 1, 90);
    } else if (IS_LAYER_OFF_STATE(state, layer_num)) {
        dprintf("off layer %d\n", layer_num);
        //todo remove layer
    }

    return state;
}
