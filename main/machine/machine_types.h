#ifndef MACHINE_TYPES_H
#define MACHINE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RELAY_R1_MIXER = 0,
    RELAY_R2_DOOR,
    RELAY_R3_PRESS_FWD,
    RELAY_R4_PRESS_REV,
    RELAY_R5_CUTTER,
    RELAY_R6_CONVEYOR,
    RELAY_R7_FLATTENER,
    RELAY_R8_SPARE,
    RELAY_COUNT
} relay_id_t;

typedef enum {
    MACHINE_IDLE = 0,
    MACHINE_MIXING,
    MACHINE_MIXING_COMPLETE,
    MACHINE_DOOR_OPENING,
    MACHINE_DOUGH_TRANSFER,
    MACHINE_DOOR_CLOSING,
    MACHINE_PRESS_ADVANCING,
    MACHINE_PRESS_HOLDING,
    MACHINE_CUTTER_EXTENDING,
    MACHINE_CUTTER_RETRACTING,
    MACHINE_CONVEYOR_RUNNING,
    MACHINE_FLATTENER_EXTENDING,
    MACHINE_FLATTENER_RETRACTING,
    MACHINE_COUNTER_UPDATE,
    MACHINE_PAUSED,
    MACHINE_STOPPING,
    MACHINE_COMPLETED,
    MACHINE_FAULT
} machine_state_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_PCF8575_COMM,
    FAULT_SETTINGS_INVALID,
    FAULT_INTERNAL
} machine_fault_t;

typedef enum {
    CMD_RECOVERY_ACK = 1,
    CMD_START_AUTO,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_STOP,
    CMD_FAULT_CLEAR,
    CMD_SETTINGS_RELOAD,
    CMD_MANUAL_MIXER_ON,
    CMD_MANUAL_MIXER_OFF,
    CMD_MANUAL_DOOR_OPEN,
    CMD_MANUAL_DOOR_CLOSE,
    CMD_MANUAL_PRESS_FWD,
    CMD_MANUAL_PRESS_REV,
    CMD_MANUAL_PRESS_COAST,
    CMD_MANUAL_CUTTER_EXTEND,
    CMD_MANUAL_CUTTER_RETRACT,
    CMD_MANUAL_CONVEYOR_ON,
    CMD_MANUAL_CONVEYOR_OFF,
    CMD_MANUAL_FLATTENER_EXTEND,
    CMD_MANUAL_FLATTENER_RETRACT,
    CMD_MANUAL_ALL_SAFE,
    CMD_MANUAL_IO_SELFTEST,
    CMD_MANUAL_POLARITY_TOGGLE
} machine_cmd_id_t;

typedef struct {
    machine_cmd_id_t id;
} machine_cmd_t;

typedef struct {
    uint32_t mixing_duration_ms;
    uint32_t door_open_duration_ms;
    uint32_t door_close_duration_ms;
    uint32_t dough_transfer_duration_ms;
    uint32_t press_advancement_duration_ms;
    uint32_t press_hold_duration_ms;
    uint32_t cutter_extend_duration_ms;
    uint32_t cutter_return_duration_ms;
    uint32_t conveyor_duration_ms;
    uint32_t flattener_extend_duration_ms;
    uint32_t flattener_return_duration_ms;
    uint32_t target_quantity;
    bool relay_active_low;
    uint8_t pcf8575_addr;
} machine_settings_t;

typedef struct {
    machine_state_t state;
    machine_state_t state_before_pause;
    bool recovery_unlocked;
    bool auto_active;
    bool manual_allowed;
    uint32_t target_qty;
    uint32_t current_qty;
    uint32_t remaining_ms;
    uint8_t commanded_outputs; /* bit0=R1 ... bit7=R8, 1=ON logical */
    machine_fault_t fault;
    const char *step_name;
} machine_status_t;

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_TYPES_H */
