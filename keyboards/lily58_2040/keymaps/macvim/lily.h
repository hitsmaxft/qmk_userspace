#pragma once

#include <stdio.h>
#include "action.h"

void set_keylog(uint16_t keycode, keyrecord_t *record);

const char find_keytable(uint16_t keycode);
const char *read_logo(void);
const char *read_keymods(void);
const char *read_keylogs(void);
const char *read_keylog(void);

void reset_keylogs_str(void);
