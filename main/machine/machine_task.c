#include "machine_task.h"

#include "actuators.h"
#include "machine_sm.h"
#include "relays.h"
#include "safety.h"
#include "settings.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "machine_task";

static QueueHandle_t s_cmd_q;
static SemaphoreHandle_t s_status_mux;
static machine_sm_t s_sm;
static machine_status_t s_status;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void publish_status(void)
{
    xSemaphoreTake(s_status_mux, portMAX_DELAY);
    s_status.state = s_sm.state;
    s_status.state_before_pause = s_sm.state_before_pause;
    s_status.recovery_unlocked = safety_is_recovery_unlocked();
    s_status.auto_active = s_sm.auto_active;
    s_status.manual_allowed = safety_is_recovery_unlocked() && !s_sm.auto_active &&
                              s_sm.state != MACHINE_FAULT;
    s_status.target_qty = s_sm.run_cfg.target_quantity;
    s_status.current_qty = s_sm.current_qty;
    s_status.remaining_ms = (s_sm.step_deadline_ms > now_ms()) ? (s_sm.step_deadline_ms - now_ms()) : 0;
    s_status.commanded_outputs = relays_get_commanded_mask();
    s_status.fault = s_sm.fault;
    s_status.step_name = machine_sm_step_name(s_sm.state);
    xSemaphoreGive(s_status_mux);
}

/**
 * Returns ESP_OK on success, ESP_ERR_NOT_SUPPORTED if the command was rejected
 * by guards / unknown (not an I/O fault), otherwise the actuator/Modbus error.
 */
static esp_err_t handle_manual(machine_cmd_id_t id)
{
    if (!safety_is_recovery_unlocked() || s_sm.auto_active || s_sm.state == MACHINE_FAULT) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    switch (id) {
    case CMD_MANUAL_MIXER_ON: return actuators_mixer(true);
    case CMD_MANUAL_MIXER_OFF: return actuators_mixer(false);
    case CMD_MANUAL_DOOR_OPEN: return actuators_door_extend(true);
    case CMD_MANUAL_DOOR_CLOSE: return actuators_door_extend(false);
    case CMD_MANUAL_PRESS_FWD: return actuators_press_forward(true);
    case CMD_MANUAL_PRESS_REV: return actuators_press_reverse(true);
    case CMD_MANUAL_PRESS_COAST: return actuators_press_coast();
    case CMD_MANUAL_CUTTER_EXTEND: return actuators_cutter_extend(true);
    case CMD_MANUAL_CUTTER_RETRACT: return actuators_cutter_extend(false);
    case CMD_MANUAL_CONVEYOR_ON: return actuators_conveyor(true);
    case CMD_MANUAL_CONVEYOR_OFF: return actuators_conveyor(false);
    case CMD_MANUAL_FLATTENER_EXTEND: return actuators_flattener_extend(true);
    case CMD_MANUAL_FLATTENER_RETRACT: return actuators_flattener_extend(false);
    case CMD_MANUAL_ALL_SAFE: return actuators_all_safe();
    case CMD_MANUAL_IO_SELFTEST: {
        ESP_LOGI(TAG, "I/O self-test: energize R1..R8 one-by-one (listen for clicks)");
        esp_err_t first_err = ESP_OK;
        for (int i = 0; i < RELAY_COUNT; i++) {
            uint8_t mask = (uint8_t)(1u << i);
            ESP_LOGI(TAG, "self-test R%d logical ON", i + 1);
            esp_err_t e = relays_apply_mask(mask);
            if (e != ESP_OK) {
                first_err = e;
                ESP_LOGE(TAG, "self-test R%d failed: %s", i + 1, esp_err_to_name(e));
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        esp_err_t safe_err = actuators_all_safe();
        if (first_err == ESP_OK) {
            first_err = safe_err;
        }
        if (first_err == ESP_OK) {
            ESP_LOGI(TAG, "I/O self-test done (all safe)");
        }
        return first_err;
    }
    default: return ESP_ERR_NOT_SUPPORTED;
    }
}

static void handle_cmd(const machine_cmd_t *cmd)
{
    uint32_t t = now_ms();

    switch (cmd->id) {
    case CMD_RECOVERY_ACK:
        safety_set_recovery_unlocked(true);
        if (s_sm.state == MACHINE_FAULT) {
            /* keep fault until explicit clear */
        } else if (s_sm.state == MACHINE_COMPLETED) {
            s_sm.state = MACHINE_IDLE;
        }
        break;

    case CMD_FAULT_CLEAR:
        if (s_sm.state == MACHINE_FAULT) {
            /* Only leave FAULT once the safe-state write is confirmed; otherwise stay faulted. */
            esp_err_t err = actuators_all_safe();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "fault clear refused: all-safe failed (%s)", esp_err_to_name(err));
                break;
            }
            s_sm.fault = FAULT_NONE;
            s_sm.state = MACHINE_IDLE;
        }
        break;

    case CMD_SETTINGS_RELOAD:
        break;

    case CMD_START_AUTO: {
        if (!safety_is_recovery_unlocked()) {
            ESP_LOGW(TAG, "start blocked: recovery locked");
            break;
        }
        if (s_sm.auto_active || s_sm.state == MACHINE_FAULT) {
            break;
        }
        if (machine_sm_handle_io_error(&s_sm, actuators_all_safe())) {
            break; /* FAULT_IO_COMM: cannot confirm safe state before start */
        }
        machine_settings_t cfg;
        settings_get(&cfg);
        if (!settings_validate(&cfg)) {
            machine_sm_enter_fault(&s_sm, FAULT_SETTINGS_INVALID);
            break;
        }
        if (!machine_sm_start_auto(&s_sm, &cfg, t)) {
            machine_sm_enter_fault(&s_sm, FAULT_SETTINGS_INVALID);
        }
        break;
    }

    case CMD_PAUSE:
        machine_sm_pause(&s_sm, t);
        break;

    case CMD_RESUME:
        machine_sm_resume(&s_sm, t);
        break;

    case CMD_STOP:
        machine_sm_stop(&s_sm);
        break;

    default:
        {
            esp_err_t err = handle_manual(cmd->id);
            if (err == ESP_ERR_NOT_SUPPORTED) {
                ESP_LOGW(TAG, "manual command %d rejected", (int)cmd->id);
            } else if (err != ESP_OK) {
                ESP_LOGE(TAG, "manual command %d failed: %s", (int)cmd->id, esp_err_to_name(err));
                machine_sm_handle_io_error(&s_sm, err);
            }
        }
        break;
    }
}

static void machine_task_fn(void *arg)
{
    (void)arg;
    machine_cmd_t cmd;

    while (1) {
        while (xQueueReceive(s_cmd_q, &cmd, 0) == pdTRUE) {
            handle_cmd(&cmd);
        }

        machine_sm_tick(&s_sm, now_ms());
        publish_status();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t machine_task_start(void)
{
    safety_init();
    machine_sm_reset(&s_sm);

    s_cmd_q = xQueueCreate(16, sizeof(machine_cmd_t));
    s_status_mux = xSemaphoreCreateMutex();
    if (s_cmd_q == NULL || s_status_mux == NULL) {
        return ESP_ERR_NO_MEM;
    }

    publish_status();

    BaseType_t ok = xTaskCreate(machine_task_fn, "machine_task", 6 * 1024, NULL, 6, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "started");
    return ESP_OK;
}

bool machine_cmd_send(machine_cmd_id_t id)
{
    if (s_cmd_q == NULL) {
        return false;
    }
    machine_cmd_t cmd = {.id = id};
    return xQueueSend(s_cmd_q, &cmd, pdMS_TO_TICKS(50)) == pdTRUE;
}

void machine_status_get(machine_status_t *out)
{
    if (out == NULL || s_status_mux == NULL) {
        return;
    }
    xSemaphoreTake(s_status_mux, portMAX_DELAY);
    *out = s_status;
    xSemaphoreGive(s_status_mux);
}
