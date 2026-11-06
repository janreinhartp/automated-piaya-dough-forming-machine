#ifndef MACHINE_TASK_H
#define MACHINE_TASK_H

#include "esp_err.h"
#include "machine_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t machine_task_start(void);
bool machine_cmd_send(machine_cmd_id_t id);
void machine_status_get(machine_status_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_TASK_H */
