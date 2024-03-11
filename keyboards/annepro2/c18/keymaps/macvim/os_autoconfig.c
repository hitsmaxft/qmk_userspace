#include QMK_KEYBOARD_H


bool process_detected_host_os_kb(os_variant_t detected_os) {
    if (!process_detected_host_os_user(detected_os)) {
        return false;
    }
    
    keymap_config.raw = eeconfig_read_keymap();

    switch (detected_os) {
        case OS_WINDOWS:
            keymap_config.swap_lalt_lgui = true;
            eeconfig_update_keymap(keymap_config.raw);
            break;
        case OS_MACOS:
        case OS_IOS:
        case OS_LINUX:
        case OS_UNSURE:
            keymap_config.swap_lalt_lgui = false;
            eeconfig_update_keymap(keymap_config.raw);
            break;
    }

    return true;
}

