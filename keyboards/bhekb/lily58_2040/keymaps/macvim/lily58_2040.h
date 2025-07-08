#pragma once

#include QMK_KEYBOARD_H
#include <stdbool.h>
#include "keycodes.h"
#include "quantum_keycodes.h"
#include "keymap_us.h"

#include "community_modules.h"
#include "community_modules_introspection.h"

enum lily_58_custom_keycode {
    // vim yank
    UK_VGCP = SAFE_RANGE,
    UK_CAPR,
    UK_STAB,
};

#define LBASE 0

#define BASE_MAC 0
#define BASE_WIN 1

#define LLOWER 2
#define LLW 2
#define LRAISE 3

// adjust layer
#define LFUNC 4
#define ADJUST 4
#define LFN 5
#define LNAVI 5
#define LNUM 6
#define LDEBUG 7

// right shift mod
#define TRS_GRV RSFT_T(KC_GRV)

#define TU_BSPC LT(LRAISE, KC_BSPC)

#define HRM_A(kc) LCTL_T(kc)
#define HRM_S(kc) LALT_T(kc)
#define HRM_D(kc) LGUI_T(kc)
#define HRM_F(kc) LSFT_T(kc)

#define HRM_C(kc) RCTL_T(kc)
#define HRM_L(kc) RALT_T(kc)
#define HRM_K(kc) RGUI_T(kc)
#define HRM_J(kc) RSFT_T(kc)

#define LCT_A LCTL_T(KC_A)
#define LAT_S LALT_T(KC_S)
#define LGT_D LGUI_T(KC_D)
#define LST_F LSFT_T(KC_F)

#define RCT_SC RCTL_T(KC_SCLN)
#define RAT_L RALT_T(KC_L)
#define RGT_K RGUI_T(KC_K)
#define RST_J RSFT_T(KC_J)

#define LCT_D LCTL_T(KC_D)
#define RCT_K RCTL_T(KC_K)

// hrm layout mapping
// clang-format off
#define LAYOUT_HRM(k0A, k0B, k0C, k0D, k0E, k0F, k5F, k5E, k5D, k5C, k5B, k5A, k1A, k1B, k1C, k1D, k1E, k1F, k6F, k6E, k6D, k6C, k6B, k6A, k2A, k2B, k2C, k2D, k2E, k2F, k7F, k7E, k7D, k7C, k7B, k7A, k3A, k3B, k3C, k3D, k3E, k3F, k4F, k9F, k8F, k8E, k8D, k8C, k8B, k8A, k4B, k4C, k4D, k4E, k9E, k9D, k9C, k9B)                                              \
    {                                                                                                                                                                                                                                                                                                                                                             \
        {k0A, k0B, k0C, k0D, k0E, k0F}, \
        {k1A, HRM_A(k1B), k1C, k1D, k1E, k1F}, \
        {k2A, HRM_A(k2B), HRM_S(k2C), HRM_D(k2D), HRM_F(k2E), k2F}, \
        {k3A, HRM_D(k3B), k3C, k3D, k3E, k3F}, \
        {XXX, k4B, k4C, k4D, k4E, k4F}, \
        {k5A, k5B, k5C, k5D, k5E, k5F}, \
        {k6A, k6B, k6C, k6D, k6E, k6F}, \
        {k7A, HRM_C(k7B), HRM_L(k7C), HRM_K(k7D), HRM_J(k7E), k7F}, \
        {k8A, k8B, k8C, k8D, k8E, k8F},  \
        { XXX, k9B, k9C, k9D, k9E, k9F                                                                                                                                                                                                                                                                                                                          \
        }                                                                                                                                                                                                                                                                                                                                                         \
    }

// clang-format on
// for tab
#define ST_MINS RSFT_T(KC_MINS)
#define ST_UNDS RSFT_T(KC_UNDS)
#define UK_SPC LT(LNAVI, KC_SPC)

#define LN_TAB LT(LNUM, KC_TAB)
#define LN_ESC LT(LNUM, KC_ESC)

#define XXXXXX KC_NO
