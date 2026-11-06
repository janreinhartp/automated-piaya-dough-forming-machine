#ifndef MACHINE_SM_H
#define MACHINE_SM_H

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

/** Apply outputs for current state (call after transitions). */
void machine_sm_apply_outputs(const machine_sm_t *sm);

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
