#pragma once

#include QMK_KEYBOARD_H

#include <stdio.h>
#include "action.h"

// show magic number
//  max char in one line
#define MAX_CHAR_LINE 21
#define KEY_LOGS_MAX_CHAR_LINE 21
// max history chars, reserved 2 char of each line for padding
#define KEY_LOGS_MAX_CHARS ((MAX_CHAR_LINE * 2))

#define KEY_LOGS_COUNT ((MAX_CHAR_LINE - MAX_CHAR_LINE % 2))
#define KEY_LOGS_COMBO_LINE ((KEY_LOGS_COUNT / 2))

#define KEY_LOG_ARR_LEN KEY_LOGS_MAX_CHARS

#define SPC_NAME 0xDB
#define ENT_NAME 0xDE
#define TAB_NAME 0xDA
#define ESC_NAME 0x9E

void set_keylog(uint16_t keycode, keyrecord_t *record);

const char  find_keytable(uint16_t keycode);
const char *read_keymods(void);
const char *read_keylogs(void);
const char *read_keylog(void);

void reset_keylogs_str(void);

void oled_ext_oled_task_user(void);
