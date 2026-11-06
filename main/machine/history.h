#ifndef MACHINE_HISTORY_H
#define MACHINE_HISTORY_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t last_qty;
    uint32_t total_cycles;
    uint32_t last_duration_ms;
    uint32_t last_unix;
} production_history_t;

esp_err_t history_load(production_history_t *out);
esp_err_t history_record_run(uint32_t qty, uint32_t duration_ms, uint32_t unix_ts);
void history_get(production_history_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_HISTORY_H */
