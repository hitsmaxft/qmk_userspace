#pragma once

/**
 * @file tap_hold.h
 * @brief Tap-hold and flow tap functionality
 *
 * This file provides tap-hold key functionality and flow tap
 * implementation for ergonomic modifier access and quick
 * key combinations.
 */

#include QMK_KEYBOARD_H

/**
 * Tap/hold functionality interface for Lily58 2040 keymap
 * Handles flow tap, chordal hold, and shift tab functionality
 */

#ifdef FLOW_TAP_TERM

/**
 * Check if two key presses are on opposite hands
 * @param last Previous key record
 * @param current Current key record
 * @return true if keys are on opposite hands
 */
bool is_opposite_hand_tap_flow(keyrecord_t *last, keyrecord_t *current);

/**
 * Reset chordal flow tap state
 */
void reset_chordal_flow_tap(void);

/**
 * Get flow tap term based on keycode and record
 * @param keycode Current keycode
 * @param record Current key record
 * @param prev_keycode Previous keycode
 * @return Flow tap term value
 */
uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode);

#endif // FLOW_TAP_TERM

/**
 * Stop shift tab hold functionality
 */
void stop_shift_hold(void);

/**
 * Process tap/hold related keycodes
 * @param keycode Keycode to process
 * @param record Key record
 * @return true if keycode was handled
 */
bool process_tap_hold_keycode(uint16_t keycode, keyrecord_t *record);

/**
 * Scan function for tap/hold functionality
 * Called from matrix_scan_user()
 */
void tap_hold_scan(void);

// External variables for shift tab functionality
extern bool     shift_tab_active;
extern uint16_t shift_tab_timer_value;
