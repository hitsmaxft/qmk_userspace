#pragma once
#include <stdint.h>

#ifndef OLED_APM_INTERVAL
#    define OLED_APM_INTERVAL 1000
#endif
#define APM_BUCKET_SIZE 60

void     apm_incr_key_counter(void);
uint32_t apm_read_keycode(void);
uint32_t apm_calc_result(uint32_t trigger_time, void *cb_arg);
