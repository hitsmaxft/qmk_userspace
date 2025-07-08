#pragma once

/**
 * @file oled_display.h
 * @brief OLED display functionality
 *
 * This file provides OLED display management including system
 * information display, logo rendering, and display state
 * management for the Lily58 keyboard.
 */

#include QMK_KEYBOARD_H

/**
 * OLED display interface for Lily58 2040 keymap
 * Handles OLED initialization, display updates, and key logging
 */

#ifdef OLED_ENABLE

/**
 * Initialize OLED display
 * Called from keyboard_post_init_user()
 */
void oled_display_init(void);

/**
 * Update OLED display
 * Called from oled_task_user()
 * @return true if display was updated
 */
bool oled_display_update(void);

/**
 * Get layer name for display
 * @param layer_id Layer ID
 * @return Layer name string
 */
const char* get_layer_name(uint16_t layer_id);

#endif // OLED_ENABLE
