#pragma once

/**
 * @file custom_keycodes.h
 * @brief Custom keycode definitions for MacVim keymap
 *
 * This file defines custom keycodes used throughout the keymap for
 * Vim operations, system shortcuts, and special functionality.
 */

#include QMK_KEYBOARD_H

/**
 * Custom keycode definitions for Lily58 2040 keymap
 * These extend the standard QMK keycodes with custom functionality
 */
enum lily_58_custom_keycode {
    // Start custom keycodes after SAFE_RANGE
    CUSTOM_VIM_YANK = SAFE_RANGE, // Vim yank functionality
    CUSTOM_CAPTURE_REGION,        // Screen capture region
    CUSTOM_SHIFT_TAB,             // Shift+Tab functionality
};

/**
 * Process custom keycodes
 * @param keycode Keycode to process
 * @param record Key record
 * @return true if keycode was handled
 */
bool process_custom_keycode(uint16_t keycode, keyrecord_t *record);
