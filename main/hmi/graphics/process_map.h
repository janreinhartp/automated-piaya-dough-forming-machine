#ifndef HMI_PROCESS_MAP_H
#define HMI_PROCESS_MAP_H

#include "machine_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HMI_STATION_MIXER = 0,
    HMI_STATION_TRANSFER,
    HMI_STATION_PRESS,
    HMI_STATION_CUTTER,
    HMI_STATION_CONVEYOR,
    HMI_STATION_FLATTENER,
    HMI_STATION_COUNT
} hmi_station_id_t;

typedef enum {
    HMI_ST_VIS_PEND = 0,
    HMI_ST_VIS_EXEC,
    HMI_ST_VIS_DONE,
    HMI_ST_VIS_PAUSE,
    HMI_ST_VIS_FAULT,
    HMI_ST_VIS_STOP
} hmi_station_visual_t;

const char *hmi_station_label(hmi_station_id_t id);
void hmi_process_map_from_status(const machine_status_t *st, hmi_station_visual_t out[HMI_STATION_COUNT]);

#ifdef __cplusplus
}
#endif

#endif /* HMI_PROCESS_MAP_H */
