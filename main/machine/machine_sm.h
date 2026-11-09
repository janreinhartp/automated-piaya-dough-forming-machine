#ifndef MACHINE_SM_H
#define MACHINE_SM_H

#include "esp_err.h"
#include "machine_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    machine_state_t state;
    machine_state_t state_before_pause;
    machine_settings_t run_cfg;
    uint32_t current_qty;
    uint32_t step_deadline_ms;
    uint32_t pause_remaining_ms;
    uint32_t run_start_ms;
    machine_fault_t fault;
    bool auto_active;
} machine_sm_t;

void machine_sm_reset(machine_sm_t *sm);
const char *machine_sm_step_name(machine_state_t state);

/**
 * Apply outputs for current state (call after transitions).
 * Returns the first actuator/Modbus error (ESP_OK if all succeeded); all outputs
 * are still attempted. Callers should route non-OK through
 * machine_sm_handle_io_error().
 */
esp_err_t machine_sm_apply_outputs(const machine_sm_t *sm);

/**
 * If err != ESP_OK and SM is not already in FAULT, enter FAULT_IO_COMM.
 * Returns true if a new fault was raised. Safe to call while faulting (log only).
 */
bool machine_sm_handle_io_error(machine_sm_t *sm, esp_err_t err);

/**
 * Advance SM using now_ms (esp_timer or tick ms).
 * Returns true if state changed.
 */
bool machine_sm_tick(machine_sm_t *sm, uint32_t now_ms);

bool machine_sm_start_auto(machine_sm_t *sm, const machine_settings_t *cfg, uint32_t now_ms);
void machine_sm_pause(machine_sm_t *sm, uint32_t now_ms);
void machine_sm_resume(machine_sm_t *sm, uint32_t now_ms);
void machine_sm_stop(machine_sm_t *sm);
void machine_sm_enter_fault(machine_sm_t *sm, machine_fault_t fault);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_SM_H */
