#include "machine_sm.h"
#include "actuators.h"
#include "history.h"
#include "settings.h"

#include <string.h>

static void arm_deadline(machine_sm_t *sm, uint32_t now_ms, uint32_t dur_ms)
{
    sm->step_deadline_ms = now_ms + dur_ms;
}

static bool deadline_reached(const machine_sm_t *sm, uint32_t now_ms)
{
    return (int32_t)(now_ms - sm->step_deadline_ms) >= 0;
}

void machine_sm_reset(machine_sm_t *sm)
{
    memset(sm, 0, sizeof(*sm));
    sm->state = MACHINE_IDLE;
}

const char *machine_sm_step_name(machine_state_t state)
{
    switch (state) {
    case MACHINE_IDLE: return "Idle";
    case MACHINE_MIXING: return "Mixing";
    case MACHINE_MIXING_COMPLETE: return "Mix Complete";
    case MACHINE_DOOR_OPENING: return "Door Opening";
    case MACHINE_DOUGH_TRANSFER: return "Dough Transfer";
    case MACHINE_DOOR_CLOSING: return "Door Closing";
    case MACHINE_PRESS_ADVANCING: return "Press Advancing";
    case MACHINE_PRESS_HOLDING: return "Press Holding";
    case MACHINE_CUTTER_EXTENDING: return "Cutter Extending";
    case MACHINE_CUTTER_RETRACTING: return "Cutter Retracting";
    case MACHINE_CONVEYOR_RUNNING: return "Conveyor";
    case MACHINE_FLATTENER_EXTENDING: return "Flattener Extending";
    case MACHINE_FLATTENER_RETRACTING: return "Flattener Retracting";
    case MACHINE_COUNTER_UPDATE: return "Counting";
    case MACHINE_PAUSED: return "Paused";
    case MACHINE_STOPPING: return "Stopping";
    case MACHINE_COMPLETED: return "Completed";
    case MACHINE_FAULT: return "Fault";
    default: return "Unknown";
    }
}

void machine_sm_apply_outputs(const machine_sm_t *sm)
{
    switch (sm->state) {
    case MACHINE_MIXING:
        actuators_mixer(true);
        actuators_door_extend(false);
        actuators_press_coast();
        actuators_cutter_extend(false);
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_DOOR_OPENING:
    case MACHINE_DOUGH_TRANSFER:
        actuators_mixer(false);
        actuators_door_extend(true);
        actuators_press_coast();
        actuators_cutter_extend(false);
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_DOOR_CLOSING:
    case MACHINE_MIXING_COMPLETE:
        actuators_mixer(false);
        actuators_door_extend(false);
        actuators_press_coast();
        actuators_cutter_extend(false);
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_PRESS_ADVANCING:
        actuators_mixer(false);
        actuators_door_extend(false);
        actuators_press_forward(true);
        actuators_cutter_extend(false);
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_PRESS_HOLDING:
        actuators_mixer(false);
        actuators_door_extend(false);
        actuators_press_coast();
        actuators_cutter_extend(false);
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_CUTTER_EXTENDING:
        actuators_cutter_extend(true);
        actuators_press_coast();
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_CUTTER_RETRACTING:
        actuators_cutter_extend(false);
        actuators_press_coast();
        actuators_conveyor(false);
        actuators_flattener_extend(false);
        break;
    case MACHINE_CONVEYOR_RUNNING:
        actuators_conveyor(true);
        actuators_cutter_extend(false);
        actuators_flattener_extend(false);
        actuators_press_coast();
        break;
    case MACHINE_FLATTENER_EXTENDING:
        actuators_conveyor(false);
        actuators_flattener_extend(true);
        actuators_press_coast();
        break;
    case MACHINE_FLATTENER_RETRACTING:
    case MACHINE_COUNTER_UPDATE:
        actuators_flattener_extend(false);
        actuators_conveyor(false);
        actuators_press_coast();
        break;
    case MACHINE_PAUSED:
        actuators_mixer(false);
        actuators_conveyor(false);
        actuators_door_extend(false);
        actuators_cutter_extend(false);
        actuators_flattener_extend(false);
        actuators_press_coast();
        break;
    case MACHINE_IDLE:
    case MACHINE_STOPPING:
    case MACHINE_COMPLETED:
    case MACHINE_FAULT:
    default:
        actuators_all_safe();
        break;
    }
}

bool machine_sm_start_auto(machine_sm_t *sm, const machine_settings_t *cfg, uint32_t now_ms)
{
    if (cfg == NULL || !settings_validate(cfg)) {
        return false;
    }
    sm->run_cfg = *cfg;
    sm->current_qty = 0;
    sm->auto_active = true;
    sm->fault = FAULT_NONE;
    sm->run_start_ms = now_ms;
    sm->state = MACHINE_MIXING;
    arm_deadline(sm, now_ms, sm->run_cfg.mixing_duration_ms);
    machine_sm_apply_outputs(sm);
    return true;
}

void machine_sm_pause(machine_sm_t *sm, uint32_t now_ms)
{
    if (!sm->auto_active || sm->state == MACHINE_PAUSED ||
        sm->state == MACHINE_IDLE || sm->state == MACHINE_COMPLETED ||
        sm->state == MACHINE_FAULT) {
        return;
    }
    sm->pause_remaining_ms = (sm->step_deadline_ms > now_ms) ? (sm->step_deadline_ms - now_ms) : 0;
    sm->state_before_pause = sm->state;
    sm->state = MACHINE_PAUSED;
    machine_sm_apply_outputs(sm);
}

void machine_sm_resume(machine_sm_t *sm, uint32_t now_ms)
{
    if (sm->state != MACHINE_PAUSED) {
        return;
    }
    sm->state = sm->state_before_pause;
    arm_deadline(sm, now_ms, sm->pause_remaining_ms);
    machine_sm_apply_outputs(sm);
}

void machine_sm_stop(machine_sm_t *sm)
{
    sm->auto_active = false;
    sm->state = MACHINE_IDLE;
    machine_sm_apply_outputs(sm);
}

void machine_sm_enter_fault(machine_sm_t *sm, machine_fault_t fault)
{
    sm->fault = fault;
    sm->auto_active = false;
    sm->state = MACHINE_FAULT;
    machine_sm_apply_outputs(sm);
}

static void enter(machine_sm_t *sm, machine_state_t next, uint32_t now_ms, uint32_t dur)
{
    sm->state = next;
    arm_deadline(sm, now_ms, dur);
    machine_sm_apply_outputs(sm);
}

bool machine_sm_tick(machine_sm_t *sm, uint32_t now_ms)
{
    if (!sm->auto_active || sm->state == MACHINE_PAUSED || sm->state == MACHINE_FAULT ||
        sm->state == MACHINE_IDLE || sm->state == MACHINE_COMPLETED) {
        return false;
    }

    if (sm->state == MACHINE_COUNTER_UPDATE) {
        sm->current_qty++;
        if (sm->current_qty >= sm->run_cfg.target_quantity) {
            uint32_t dur = now_ms - sm->run_start_ms;
            history_record_run(sm->current_qty, dur, 0);
            sm->auto_active = false;
            sm->state = MACHINE_COMPLETED;
            machine_sm_apply_outputs(sm);
            return true;
        }
        enter(sm, MACHINE_PRESS_ADVANCING, now_ms, sm->run_cfg.press_advancement_duration_ms);
        return true;
    }

    if (!deadline_reached(sm, now_ms)) {
        return false;
    }

    switch (sm->state) {
    case MACHINE_MIXING:
        enter(sm, MACHINE_MIXING_COMPLETE, now_ms, 50);
        break;
    case MACHINE_MIXING_COMPLETE:
        enter(sm, MACHINE_DOOR_OPENING, now_ms, sm->run_cfg.door_open_duration_ms);
        break;
    case MACHINE_DOOR_OPENING:
        enter(sm, MACHINE_DOUGH_TRANSFER, now_ms, sm->run_cfg.dough_transfer_duration_ms);
        break;
    case MACHINE_DOUGH_TRANSFER:
        enter(sm, MACHINE_DOOR_CLOSING, now_ms, sm->run_cfg.door_close_duration_ms);
        break;
    case MACHINE_DOOR_CLOSING:
        enter(sm, MACHINE_PRESS_ADVANCING, now_ms, sm->run_cfg.press_advancement_duration_ms);
        break;
    case MACHINE_PRESS_ADVANCING:
        enter(sm, MACHINE_PRESS_HOLDING, now_ms, sm->run_cfg.press_hold_duration_ms);
        break;
    case MACHINE_PRESS_HOLDING:
        enter(sm, MACHINE_CUTTER_EXTENDING, now_ms, sm->run_cfg.cutter_extend_duration_ms);
        break;
    case MACHINE_CUTTER_EXTENDING:
        enter(sm, MACHINE_CUTTER_RETRACTING, now_ms, sm->run_cfg.cutter_return_duration_ms);
        break;
    case MACHINE_CUTTER_RETRACTING:
        enter(sm, MACHINE_CONVEYOR_RUNNING, now_ms, sm->run_cfg.conveyor_duration_ms);
        break;
    case MACHINE_CONVEYOR_RUNNING:
        enter(sm, MACHINE_FLATTENER_EXTENDING, now_ms, sm->run_cfg.flattener_extend_duration_ms);
        break;
    case MACHINE_FLATTENER_EXTENDING:
        enter(sm, MACHINE_FLATTENER_RETRACTING, now_ms, sm->run_cfg.flattener_return_duration_ms);
        break;
    case MACHINE_FLATTENER_RETRACTING:
        sm->state = MACHINE_COUNTER_UPDATE;
        machine_sm_apply_outputs(sm);
        break;
    default:
        break;
    }
    return true;
}
