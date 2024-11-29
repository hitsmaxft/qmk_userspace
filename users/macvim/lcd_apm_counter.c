#include QMK_KEYBOARD_H

#include "lily.h"

#define APM_BUCKET_SIZE 60
static uint8_t apm_bucket_idx               = 0;
static uint8_t apm_buckets[APM_BUCKET_SIZE] = {0};
static uint8_t press_key_count              = 0;
static uint32_t keycode_apm                  = 0;

/**
 * calculate apm by 6 slot;
 */
uint32_t calc_apm(uint32_t trigger_time, void *cb_arg) {
    apm_buckets[apm_bucket_idx] = press_key_count;
    press_key_count             = 0;

    // next update slot
    apm_bucket_idx = (apm_bucket_idx + 1) % APM_BUCKET_SIZE;

    uint32_t sum = 0;
    for (int i = 0; i < APM_BUCKET_SIZE; i++) {
        sum += apm_buckets[i];
    }

    // reset
    keycode_apm = sum;
    return OLED_APM_INTERVAL;
}

void apm_incr_key_counter(void) {
    press_key_count++;
}

uint32_t read_keycode_apm(void) {
    return keycode_apm;
}
