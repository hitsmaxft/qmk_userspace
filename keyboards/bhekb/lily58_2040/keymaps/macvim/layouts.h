#pragma once

#include QMK_KEYBOARD_H

/**
 * Layout definitions for Lily58 2040 keymap
 * Defines custom layouts and modifier key mappings
 */

// Homerow mod layout mapping
// clang-format off
#define LAYOUT_HOMEROW_MOD(k0A, k0B, k0C, k0D, k0E, k0F, k5F, k5E, k5D, k5C, k5B, k5A, k1A, k1B, k1C, k1D, k1E, k1F, k6F, k6E, k6D, k6C, k6B, k6A, k2A, k2B, k2C, k2D, k2E, k2F, k7F, k7E, k7D, k7C, k7B, k7A, k3A, k3B, k3C, k3D, k3E, k3F, k4F, k9F, k8F, k8E, k8D, k8C, k8B, k8A, k4B, k4C, k4D, k4E, k9E, k9D, k9C, k9B) \
    { \
        {k0A, k0B, k0C, k0D, k0E, k0F}, \
        {k1A, HOMEROW_MOD_LEFT_CTRL(k1B), k1C, k1D, k1E, k1F}, \
        {k2A, HOMEROW_MOD_LEFT_CTRL(k2B), HOMEROW_MOD_LEFT_ALT(k2C), HOMEROW_MOD_LEFT_GUI(k2D), HOMEROW_MOD_LEFT_SHIFT(k2E), k2F}, \
        {k3A, HOMEROW_MOD_LEFT_GUI(k3B), k3C, k3D, k3E, k3F}, \
        {XXX, k4B, k4C, k4D, k4E, k4F}, \
        {k5A, k5B, k5C, k5D, k5E, k5F}, \
        {k6A, k6B, k6C, k6D, k6E, k6F}, \
        {k7A, HOMEROW_MOD_RIGHT_CTRL(k7B), HOMEROW_MOD_RIGHT_ALT(k7C), HOMEROW_MOD_RIGHT_GUI(k7D), HOMEROW_MOD_RIGHT_SHIFT(k7E), k7F}, \
        {k8A, k8B, k8C, k8D, k8E, k8F}, \
        { XXX, k9B, k9C, k9D, k9E, k9F } \
    }
// clang-format on

// Homerow mod key mappings
#define HOMEROW_MOD_LEFT_CTRL(kc) LCTL_T(kc)
#define HOMEROW_MOD_LEFT_ALT(kc) LALT_T(kc)
#define HOMEROW_MOD_LEFT_GUI(kc) LGUI_T(kc)
#define HOMEROW_MOD_LEFT_SHIFT(kc) LSFT_T(kc)

#define HOMEROW_MOD_RIGHT_CTRL(kc) RCTL_T(kc)
#define HOMEROW_MOD_RIGHT_ALT(kc) RALT_T(kc)
#define HOMEROW_MOD_RIGHT_GUI(kc) RGUI_T(kc)
#define HOMEROW_MOD_RIGHT_SHIFT(kc) RSFT_T(kc)

// Specific homerow mod mappings
#define LEFT_CTRL_A LCTL_T(KC_A)
#define LEFT_ALT_S LALT_T(KC_S)
#define LEFT_GUI_D LGUI_T(KC_D)
#define LEFT_SHIFT_F LSFT_T(KC_F)

#define RIGHT_CTRL_SEMICOLON RCTL_T(KC_SCLN)
#define RIGHT_ALT_L RALT_T(KC_L)
#define RIGHT_GUI_K RGUI_T(KC_K)
#define RIGHT_SHIFT_J RSFT_T(KC_J)

#define LEFT_CTRL_D LCTL_T(KC_D)
#define RIGHT_CTRL_K RCTL_T(KC_K)

// Tap/hold key mappings
#define RIGHT_SHIFT_GRAVE RSFT_T(KC_GRV)
#define TAP_HOLD_BACKSPACE LT(LAYER_RAISE, KC_BSPC)
#define SHIFT_TAP_MINUS RSFT_T(KC_MINS)
#define SHIFT_TAP_UNDERSCORE RSFT_T(KC_UNDS)
#define TAP_HOLD_SPACE LT(LAYER_NAVIGATION, KC_SPC)
#define TAP_HOLD_NUM_TAB LT(LAYER_NUMBER, KC_TAB)
#define TAP_HOLD_NUM_ESC LT(LAYER_NUMBER, KC_ESC)
