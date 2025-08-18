#include "oled_display.h"
#include <stdio.h>
#include "layers.h"
#include "community_modules.h"
#include "community_modules_introspection.h"

#ifdef OLED_ENABLE
#    include "oled_driver.h"
static int logo_show_delay = 0;

// OLED variables
static bool is_master          = false;
static int  logo_display_delay = 0;

uint32_t hide_logo(uint32_t trigger_time, void *cb_arg) {
    logo_display_delay = 0;
    reset_keylogs_str();
    return 0;
}

void oled_display_init(void) {
    is_master          = is_keyboard_master();
    logo_display_delay = 1;

    oled_write(read_logo(), false);

    if (!is_keyboard_master()) {
        return;
    }

    defer_exec(LOGO_HIDE_DELAY_MS, hide_logo, NULL);
    defer_exec(OLED_APM_INTERVAL_MS, apm_calc_result, NULL);
}

const char *get_layer_name(uint16_t layer_id) {
    switch (layer_id) {
        case LAYER_BASE_MAC:
            return "Def";
        case LAYER_LOWER:
            return "LOW";
        case LAYER_RAISE:
            return "RAI";
        case LAYER_FUNCTION:
            return "ADJ";
        case LAYER_NAVIGATION:
            return "NAV";
        case LAYER_NUMBER:
            return "NUM";
        default:
            return "";
    }
}

bool oled_display_update(void) {
    char charbuffer[21] = {0};

    print("on oled display\n");
    if (logo_show_delay > 0 || !is_keyboard_master()) {
        oled_write(read_logo(), false);
        return false;
    }
    if (debug_enable) {
        oled_write_ln_P(PSTR(read_keylog()), false);
    } else {
        oled_write_ln_P(PSTR("Lily58 QMK"), false);
    }

    uint16_t    layer_id   = get_highest_layer(layer_state);
    const char *layer_name = get_layer_name(layer_id);

    sprintf(charbuffer, "%3s ", layer_name);
    oled_write_P(PSTR(charbuffer), false);

    sprintf(charbuffer, "APM: %3li\n", apm_read_keycode());
    oled_write_P(PSTR(charbuffer), false);

    oled_write_P(PSTR(read_keylogs()), false);

    return true;
}

#endif // OLED_ENABLE
