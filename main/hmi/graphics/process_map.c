#include "process_map.h"
#include <stddef.h>

const char *hmi_station_label(hmi_station_id_t id)
{
    static const char *labels[HMI_STATION_COUNT] = {
        "01 MIXER", "02 TRANSFER", "03 PRESS", "04 CUTTER", "05 CONVEYOR", "06 FLATTENER"
    };
    if ((int)id < 0 || id >= HMI_STATION_COUNT) {
        return "?";
    }
    return labels[id];
}

static int station_rank(machine_state_t state)
{
    switch (state) {
    case MACHINE_MIXING:
    case MACHINE_MIXING_COMPLETE:
        return 0;
    case MACHINE_DOOR_OPENING:
    case MACHINE_DOUGH_TRANSFER:
    case MACHINE_DOOR_CLOSING:
        return 1;
    case MACHINE_PRESS_ADVANCING:
    case MACHINE_PRESS_HOLDING:
        return 2;
    case MACHINE_CUTTER_EXTENDING:
    case MACHINE_CUTTER_RETRACTING:
        return 3;
    case MACHINE_CONVEYOR_RUNNING:
        return 4;
    case MACHINE_FLATTENER_EXTENDING:
    case MACHINE_FLATTENER_RETRACTING:
    case MACHINE_COUNTER_UPDATE:
        return 5;
    case MACHINE_COMPLETED:
        return 6;
    default:
        return -1;
    }
}

void hmi_process_map_from_status(const machine_status_t *st, hmi_station_visual_t out[HMI_STATION_COUNT])
{
    for (int i = 0; i < HMI_STATION_COUNT; i++) {
        out[i] = HMI_ST_VIS_PEND;
    }
    if (st == NULL) {
        return;
    }

    if (st->fault != FAULT_NONE || st->state == MACHINE_FAULT) {
        for (int i = 0; i < HMI_STATION_COUNT; i++) {
            out[i] = HMI_ST_VIS_FAULT;
        }
        return;
    }
    if (st->state == MACHINE_STOPPING) {
        for (int i = 0; i < HMI_STATION_COUNT; i++) {
            out[i] = HMI_ST_VIS_STOP;
        }
        return;
    }
    if (st->state == MACHINE_COMPLETED) {
        for (int i = 0; i < HMI_STATION_COUNT; i++) {
            out[i] = HMI_ST_VIS_DONE;
        }
        return;
    }

    machine_state_t ref = st->state;
    bool paused = (st->state == MACHINE_PAUSED);
    if (paused) {
        ref = st->state_before_pause;
    }

    int rank = station_rank(ref);
    if (rank < 0) {
        return;
    }

    for (int i = 0; i < HMI_STATION_COUNT; i++) {
        if (i < rank) {
            out[i] = HMI_ST_VIS_DONE;
        } else if (i == rank) {
            out[i] = paused ? HMI_ST_VIS_PAUSE : HMI_ST_VIS_EXEC;
        } else {
            out[i] = HMI_ST_VIS_PEND;
        }
    }
}
