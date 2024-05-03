#pragma once

#include QMK_KEYBOARD_H


enum macvim_keycodes {
  KC_AP_NOP = SAFE_RANGE,
  KC_AP_IAP
};

//max to 20 is enough
extern uint8_t layer_mask[20];
