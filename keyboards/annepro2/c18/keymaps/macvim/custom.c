/*
 * Copyright (c) 2024 BHE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include QMK_KEYBOARD_H

#include "macvim.h"
#include "print.h"

#ifndef __INDICATOR_COLOR__
//order BGR
    #define __INDICATOR_COLOR__ {0xFF, 0x00, 0x00, 0xff}
#endif


// store all masked layer
const ap2_led_t no_color = {
    .pv  = 0x00,
};

const ap2_led_t layer_color = {
    .pv = __INDICATOR_COLOR__,
};


/* layer settings */
uint8_t layer_mask[20];
static uint8_t last_layer_default = 0;

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


const ap2_led_t leader_blink_color = {
    .p.blue  = 0xff,
    .p.red   = 0x00,
    .p.green = 0x00,
    .p.alpha = 0xff,
};

void leader_start_user(void) {
    ap2_led_mask_set_key(4, 6, leader_blink_color);
    // Do something when the leader key is pressed
}

void leader_end_user(void) {
    ap2_led_mask_set_key(4, 6, no_color);

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


layer_state_t default_layer_state_set_user(layer_state_t state) {

    uint8_t layer_num = get_highest_layer(state);
    dprintf("set default layer to %d\n", layer_num);
    uint8_t last_dl = last_layer_default;
    ap2_led_mask_set_key(0, last_dl, no_color);

    last_layer_default = layer_num;
    if (layer_num > 0 ) {
        //will not reset by  layer_state_set_user
        layer_mask[layer_num] = 2;
        ap2_led_mask_set_key(0, layer_num, layer_color);
    }

    //remove mark
    layer_mask[last_dl] = 0;
    return state;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer_num = get_highest_layer(state);

    if (IS_LAYER_ON_STATE(state, layer_num)) {
        dprintf("entering layer %d\n", layer_num);
        // light on number key
        //reset other keys
        for (int i =0 ; i <20 ; i++) {
            if (layer_mask[i] == 1) {
                //reset color
                layer_mask[i] = 0;
                dprintf("reset layer %d led to off\n", i);
                ap2_led_mask_set_key(0, i, no_color);
            }
        }
        if ( 0 == layer_num) {
            //default layer
            //layer zero just blink
            ap2_led_blink(0, last_layer_default , layer_color, 1, 20);
        } else {
            //set color mask on key
            ap2_led_mask_set_key(0, layer_num, layer_color);
            layer_mask[layer_num] = 1;
        }


    } else if (IS_LAYER_OFF_STATE(state, layer_num)) {
        dprintf("leaving layer %d\n", layer_num);
        layer_mask[layer_num] = 0;
        ap2_led_mask_set_key(0, layer_num, no_color);
    } else {
        dprintf("skip layer %d\n", layer_num);
    }


    return state;
}
