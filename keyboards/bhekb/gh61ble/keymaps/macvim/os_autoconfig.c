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


#ifdef OS_DETECTION_ENABLE

bool process_detected_host_os_kb(os_variant_t detected_os) {
    if (!process_detected_host_os_user(detected_os)) {
        return false;
    }

    keymap_config.raw = eeconfig_read_keymap();

    switch (detected_os) {
        case OS_WINDOWS:
            /**
            if (!keymap_config.swap_lctl_lgui || !keymap_config.swap_rctl_rgui){
                keymap_config.swap_lctl_lgui = true;
                keymap_config.swap_rctl_rgui = true;
                eeconfig_update_keymap(keymap_config.raw);
            }
            break;
            **/
        case OS_MACOS:
        case OS_IOS:
        case OS_LINUX:
        case OS_UNSURE:
            if (keymap_config.swap_lctl_lgui || keymap_config.swap_rctl_rgui){
                keymap_config.swap_lctl_lgui = false;
                keymap_config.swap_rctl_rgui = false;
                eeconfig_update_keymap(keymap_config.raw);
            }
            break;
    }

    return true;
}
#else

#endif

