#pragma once

/**
 * @file layers.h
 * @brief Layer definitions for MacVim keymap
 *
 * This file defines the layer structure and layer switching logic
 * for the MacVim keymap, including base layers, symbol layers,
 * and function layers.
 */

#include QMK_KEYBOARD_H

/**
 * Layer definitions for Lily58 2040 keymap
 * Each layer provides different functionality and key mappings
 */

// Base layers
#define LAYER_BASE_MAC 0
#define LAYER_BASE_WIN 1

// Function layers
#define LAYER_LOWER 2
#define LAYER_RAISE 3
#define LAYER_FUNCTION 4
#define LAYER_NAVIGATION 5
#define LAYER_NUMBER 6
#define LAYER_DEBUG 7

// Legacy aliases for backward compatibility (to be removed)
#define LBASE 0
#define LLOWER 2
#define LRAISE 3
#define LFUNC 4
#define LNAVI 5
#define LNUM 6
#define LDEBUG 7
