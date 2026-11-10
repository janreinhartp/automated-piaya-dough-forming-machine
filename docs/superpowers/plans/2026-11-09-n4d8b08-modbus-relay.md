# N4D8B08 Modbus Relay Backend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the PCF8575 I2C relay backend with RS485 Modbus RTU control of an Eletechsup N4D8B08, keeping the existing R1–R8 / actuator / machine / HMI APIs.

**Architecture:** Thin `components/n4d8b08` driver (UART + Modbus FC06) behind rewritten `main/machine/relays.*`. Delete `components/pcf8575`. No change to state-machine sequencing.

**Tech Stack:** ESP-IDF ≥ 5.1, ESP32-S3, `driver/uart`, existing FreeRTOS machine task, LVGL HMI.

## Global Constraints

- Transport: Waveshare RS485 UART TX **GPIO16**, RX **GPIO15**, 9600 8N1, slave ID **1**
- Open channel N: FC06 reg `0x000N` data `0x0001`; Close: data `0x0002`; Close all: reg `0x0000` data `0x0008`
- Keep press R3/R4 mutual exclusion in `relays`
- Do not place Modbus I/O inside LVGL callbacks
- Remove PCF8575 completely; remove `relay_active_low` / `pcf8575_addr`
- Preserve Recovery gate and E-stop power-kill model
- Spec: `docs/superpowers/specs/2026-11-09-n4d8b08-modbus-relay-design.md`

## File map

| Path | Responsibility |
|------|----------------|
| Create `components/n4d8b08/modbus_crc.c/.h` | Modbus CRC16 |
| Create `components/n4d8b08/n4d8b08.c/.h` | UART + FC06 open/close/close_all + retries |
| Create `components/n4d8b08/CMakeLists.txt` | Component build |
| Create `components/n4d8b08/test/test_modbus_crc.c` | Host/Unity CRC vectors (or on-target unity) |
| Modify `main/machine/relays.c/.h` | Backend → n4d8b08 |
| Modify `main/machine/machine_types.h` | Settings fields; `FAULT_IO_COMM` |
| Modify `main/machine/settings.c` | Defaults/validation |
| Modify `main/machine/machine_task.c` | Drop polarity; keep self-test |
| Modify `main/hmi/screens/scr_manual.c` | Remove polarity button |
| Modify `main/main.c` | Boot path |
| Modify `main/CMakeLists.txt` | REQUIRES n4d8b08, drop pcf8575 |
| Delete `components/pcf8575/` | Remove |
| Update `docs/*.md` listed in spec §9 | Reflect Modbus backend |

---

### Task 1: Modbus CRC16 helper

**Files:**
- Create: `components/n4d8b08/modbus_crc.h`
- Create: `components/n4d8b08/modbus_crc.c`
- Create: `components/n4d8b08/CMakeLists.txt`
- Create: `components/n4d8b08/test_crc_main.c` (optional host check via `idf.py` or document manual vector)

**Interfaces:**
- Produces: `uint16_t modbus_crc16(const uint8_t *data, size_t len);` — Modbus CRC16, polynomial 0xA001, init 0xFFFF; return value is **already** low-byte-first ordering when stored as little-endian on wire (`crc & 0xFF` then `crc >> 8`)

- [ ] **Step 1: Add header and implementation**

`components/n4d8b08/modbus_crc.h`:
```c
#ifndef MODBUS_CRC_H
#define MODBUS_CRC_H
#include <stddef.h>
#include <stdint.h>
uint16_t modbus_crc16(const uint8_t *data, size_t len);
#endif
```

`components/n4d8b08/modbus_crc.c`:
```c
#include "modbus_crc.h"

uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
```

`components/n4d8b08/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "modbus_crc.c" "n4d8b08.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver)
```
(Note: `n4d8b08.c` is added in Task 2; for Task 1 only, register `modbus_crc.c` first, then extend in Task 2.)

Temporary Task-1 CMakeLists:
```cmake
idf_component_register(SRCS "modbus_crc.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 2: Verify CRC against known vector**

Known: request bytes `01 06 00 01 00 01` → CRC16 = `0xCA19` → wire order `19 CA`.

Add a one-shot check in a comment or temporary log in a later task; for now run a tiny host compile if available, or assert in `n4d8b08_init` once:

```c
static const uint8_t v[] = {0x01, 0x06, 0x00, 0x01, 0x00, 0x01};
assert(modbus_crc16(v, 6) == 0xCA19);
```

- [ ] **Step 3: Commit**

```bash
git add components/n4d8b08/modbus_crc.c components/n4d8b08/modbus_crc.h components/n4d8b08/CMakeLists.txt
git commit -m "feat(n4d8b08): add Modbus CRC16 helper"
```

---

### Task 2: N4D8B08 UART + FC06 driver

**Files:**
- Create: `components/n4d8b08/n4d8b08.h`
- Create: `components/n4d8b08/n4d8b08.c`
- Modify: `components/n4d8b08/CMakeLists.txt`

**Interfaces:**
- Consumes: `modbus_crc16`
- Produces:
  - `esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud);`
  - `esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open);`
  - `esp_err_t n4d8b08_close_all(void);`
  - `bool n4d8b08_is_ready(void);`

- [ ] **Step 1: Write public header**

```c
#ifndef N4D8B08_H
#define N4D8B08_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define N4D8B08_UART_NUM      UART_NUM_1
#define N4D8B08_TX_GPIO       16
#define N4D8B08_RX_GPIO       15
#define N4D8B08_DEFAULT_SLAVE 1
#define N4D8B08_DEFAULT_BAUD  9600

esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud);
esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open);
esp_err_t n4d8b08_close_all(void);
bool n4d8b08_is_ready(void);
#endif
```

- [ ] **Step 2: Implement driver**

`n4d8b08.c` outline (full implementation required):

```c
#include "n4d8b08.h"
#include "modbus_crc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "n4d8b08";
static uint8_t s_slave = N4D8B08_DEFAULT_SLAVE;
static bool s_ready;
static SemaphoreHandle_t s_mutex;

static esp_err_t fc06(uint16_t reg, uint16_t value)
{
    uint8_t req[8];
    req[0] = s_slave;
    req[1] = 0x06;
    req[2] = (uint8_t)(reg >> 8);
    req[3] = (uint8_t)(reg & 0xFF);
    req[4] = (uint8_t)(value >> 8);
    req[5] = (uint8_t)(value & 0xFF);
    uint16_t crc = modbus_crc16(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    /* 3.5 char quiet time @9600 ≈ 4ms */
    vTaskDelay(pdMS_TO_TICKS(5));
    uart_flush_input(N4D8B08_UART_NUM);
    int w = uart_write_bytes(N4D8B08_UART_NUM, (const char *)req, 8);
    if (w != 8) return ESP_FAIL;
    (void)uart_wait_tx_done(N4D8B08_UART_NUM, pdMS_TO_TICKS(100));

    uint8_t resp[8];
    int r = uart_read_bytes(N4D8B08_UART_NUM, resp, 8, pdMS_TO_TICKS(200));
    if (r != 8) {
        ESP_LOGW(TAG, "FC06 timeout/short read %d (reg=0x%04X val=0x%04X)", r, reg, value);
        return ESP_ERR_TIMEOUT;
    }
    uint16_t rcrc = modbus_crc16(resp, 6);
    if (resp[6] != (rcrc & 0xFF) || resp[7] != (rcrc >> 8)) {
        return ESP_ERR_INVALID_CRC;
    }
    /* Expect echo of request for FC06 */
    for (int i = 0; i < 6; i++) {
        if (resp[i] != req[i]) return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

static esp_err_t fc06_retry(uint16_t reg, uint16_t value)
{
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < 3; i++) {
        err = fc06(reg, value);
        if (err == ESP_OK) return ESP_OK;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return err;
}

esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud)
{
    s_slave = slave_id ? slave_id : N4D8B08_DEFAULT_SLAVE;
    if (baud == 0) baud = N4D8B08_DEFAULT_BAUD;
    if (s_mutex == NULL) s_mutex = xSemaphoreCreateMutex();

    uart_config_t cfg = {
        .baud_rate = (int)baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(N4D8B08_UART_NUM, 256, 256, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(N4D8B08_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(N4D8B08_UART_NUM, N4D8B08_TX_GPIO, N4D8B08_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    s_ready = true;
    esp_err_t err = n4d8b08_close_all();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "close_all at init failed: %s (module may be unpowered)", esp_err_to_name(err));
        /* Still mark ready so HMI can retry; machine treats failures per command */
    }
    ESP_LOGI(TAG, "ready slave=%u baud=%lu TX=%d RX=%d", s_slave, (unsigned long)baud,
             N4D8B08_TX_GPIO, N4D8B08_RX_GPIO);
    return ESP_OK;
}

esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open)
{
    if (!s_ready || channel_1_to_8 < 1 || channel_1_to_8 > 8) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = fc06_retry(channel_1_to_8, open ? 0x0001 : 0x0002);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t n4d8b08_close_all(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = fc06_retry(0x0000, 0x0008);
    xSemaphoreGive(s_mutex);
    return err;
}

bool n4d8b08_is_ready(void) { return s_ready; }
```

Update CMakeLists to include `n4d8b08.c` and `REQUIRES driver`.

- [ ] **Step 3: Build**

```bash
idf.py build
```
Expected: build succeeds (or only fails on unrelated issues).

- [ ] **Step 4: Commit**

```bash
git add components/n4d8b08/
git commit -m "feat(n4d8b08): UART Modbus FC06 driver for relay board"
```

---

### Task 3: Rewrite relays HAL

**Files:**
- Modify: `main/machine/relays.h`
- Modify: `main/machine/relays.c`
- Modify: `main/CMakeLists.txt` (REQUIRES)

**Interfaces:**
- Consumes: `n4d8b08_init`, `n4d8b08_set_channel`, `n4d8b08_close_all`, `n4d8b08_is_ready`
- Produces:
  - `esp_err_t relays_init(uint8_t slave_id, uint32_t baud);`
  - `esp_err_t relays_force_safe(void);`
  - `esp_err_t relays_set(relay_id_t id, bool logical_on);`
  - `bool relays_get(relay_id_t id);`
  - `uint8_t relays_get_commanded_mask(void);`
  - `esp_err_t relays_apply_mask(uint8_t logical_on_mask);`
  - Remove: `relays_set_polarity`

- [ ] **Step 1: Replace `relays.h` API**

```c
esp_err_t relays_init(uint8_t slave_id, uint32_t baud);
esp_err_t relays_force_safe(void);
esp_err_t relays_set(relay_id_t id, bool logical_on);
bool relays_get(relay_id_t id);
uint8_t relays_get_commanded_mask(void);
esp_err_t relays_apply_mask(uint8_t logical_on_mask);
```

- [ ] **Step 2: Rewrite `relays.c`**

- Drop `#include "pcf8575.h"` and polarity logic.
- `#include "n4d8b08.h"`.
- `relays_init`: call `n4d8b08_init(slave_id, baud)` then `relays_force_safe()`.
- `push_hw` for single bit: map `id` 0..7 → channel `id+1`; call `n4d8b08_set_channel`.
- `relays_force_safe`: clear logical mask; `n4d8b08_close_all()`.
- `relays_apply_mask`: for each bit difference vs previous, set channel; or close_all then open bits that are set (simpler, slightly slower — acceptable).
- Keep press mutex logic.

Recommended `relays_apply_mask` for self-test:
```c
esp_err_t relays_apply_mask(uint8_t logical_on_mask)
{
    // mutex, reject R3+R4 both
    esp_err_t err = n4d8b08_close_all();
    s_logical_on = 0;
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (logical_on_mask & (1u << i)) {
            err = n4d8b08_set_channel((uint8_t)(i + 1), true);
            if (err != ESP_OK) break;
            s_logical_on |= (uint8_t)(1u << i);
        }
    }
    return err;
}
```

- [ ] **Step 3: Update `main/CMakeLists.txt` REQUIRES**

Replace `pcf8575` with `n4d8b08`. Keep `i2c` (touch still needs it).

- [ ] **Step 4: Commit**

```bash
git add main/machine/relays.c main/machine/relays.h main/CMakeLists.txt
git commit -m "feat(relays): switch backend from PCF8575 to N4D8B08 Modbus"
```

---

### Task 4: Settings, types, machine_task, HMI cleanup

**Files:**
- Modify: `main/machine/machine_types.h`
- Modify: `main/machine/settings.c`
- Modify: `main/machine/machine_task.c`
- Modify: `main/hmi/screens/scr_manual.c`

**Interfaces:**
- Settings struct fields: `uint8_t modbus_slave_id;` `uint32_t modbus_baud;` — remove `relay_active_low`, `pcf8575_addr`
- Fault: rename/add `FAULT_IO_COMM` (replace `FAULT_PCF8575_COMM`)
- Remove `CMD_MANUAL_POLARITY_TOGGLE` handling and Manual button

- [ ] **Step 1: Update `machine_types.h` settings + fault + remove polarity cmd**

```c
typedef enum {
    FAULT_NONE = 0,
    FAULT_IO_COMM,
    FAULT_SETTINGS_INVALID,
    FAULT_INTERNAL
} machine_fault_t;
```

In `machine_settings_t`:
```c
uint8_t modbus_slave_id;   /* default 1 */
uint32_t modbus_baud;      /* default 9600 */
```

Remove `CMD_MANUAL_POLARITY_TOGGLE` from enum.

- [ ] **Step 2: Update `settings.c` defaults/validation**

```c
out->modbus_slave_id = 1;
out->modbus_baud = 9600;
```
Validate slave 1–47; baud one of 2400/4800/9600/19200.

Bump NVS `version` to `2` so old blobs with PCF fields reset to defaults.

- [ ] **Step 3: Fix `machine_task.c`**

- Remove polarity toggle case and `relays_set_polarity` calls in `CMD_SETTINGS_RELOAD`.
- Self-test remains using `relays_apply_mask`.

- [ ] **Step 4: Fix `scr_manual.c`**

Remove `ACT_POL` / “FLIP RELAY POLARITY” button.

- [ ] **Step 5: Commit**

```bash
git add main/machine/machine_types.h main/machine/settings.c main/machine/machine_task.c main/hmi/screens/scr_manual.c
git commit -m "refactor: drop PCF polarity settings; add Modbus slave/baud config"
```

---

### Task 5: Boot path + delete PCF8575

**Files:**
- Modify: `main/main.c`
- Delete: `components/pcf8575/` (all files)
- Modify: any remaining `#include "pcf8575.h"` (should be none)

- [ ] **Step 1: Rewrite relay init in `main.c`**

```c
esp_lcd_touch_handle_t tp_handle = touch_gt911_init();
DEV_I2C_Scan(); /* still useful for GT911/IO_EXTENSION */

esp_lcd_panel_handle_t panel_handle = waveshare_esp32_s3_rgb_lcd_init();
wavesahre_rgb_lcd_bl_on();
ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

esp_err_t io_err = relays_init(cfg.modbus_slave_id, cfg.modbus_baud);
if (io_err != ESP_OK) {
    ESP_LOGE(TAG, "N4D8B08/relay init failed (%s) — HMI will run; outputs may be unavailable",
             esp_err_to_name(io_err));
}
```

Remove PCF-specific comments.

- [ ] **Step 2: Delete `components/pcf8575/`**

```bash
# PowerShell
Remove-Item -Recurse -Force components/pcf8575
```

- [ ] **Step 3: Build and flash**

```bash
idf.py build flash monitor
```
Expected boot log: `n4d8b08: ready slave=1 baud=9600 ...` and close-all attempt.

- [ ] **Step 4: Bench Manual Mixer ON/OFF**

Expected: relay channel 1 clicks on/off; serial shows no FC06 timeouts when wired.

- [ ] **Step 5: Commit**

```bash
git add -A main/main.c
git add -u components/pcf8575
git commit -m "feat: boot N4D8B08 over RS485; remove PCF8575 component"
```

---

### Task 6: Update project docs

**Files:**
- Modify: `docs/IO_MAPPING.md`
- Modify: `docs/HARDWARE_ARCHITECTURE.md`
- Modify: `docs/FIRMWARE_ARCHITECTURE.md`
- Modify: `docs/OPEN_DECISIONS.md`
- Modify: `docs/SAFETY_AND_FAULT_HANDLING.md`
- Modify: `docs/TEST_PLAN.md`
- Modify: `docs/SETTINGS_SPECIFICATION.md`
- Modify: `docs/PROJECT_OVERVIEW.md`

- [ ] **Step 1: Replace PCF8575 sections with N4D8B08 Modbus RS485** (slave 1, 9600, GPIO15/16, channel map, remove polarity).

- [ ] **Step 2: Decision log entry** dated 2026-11-09 for backend swap.

- [ ] **Step 3: Commit**

```bash
git add docs/
git commit -m "docs: document N4D8B08 Modbus relay backend"
```

---

## Plan self-review

**Spec coverage:** HAL (§5) Tasks 2–3; boot/safety (§6) Tasks 3+5; settings (§7) Task 4; testing (§8) Steps in 5; docs (§9) Task 6; PCF removal Task 5. DE/RE open item remains bring-up note in docs.

**Placeholders:** None intentional.

**Type consistency:** `relays_init(uint8_t slave_id, uint32_t baud)` used in main/settings; `n4d8b08_set_channel(1..8, bool open)`; fault `FAULT_IO_COMM`.

---

## Execution handoff

Plan complete and saved to `docs/superpowers/plans/2026-11-09-n4d8b08-modbus-relay.md`. Two execution options:

**1. Subagent-Driven (recommended)** — fresh subagent per task, review between tasks  

**2. Inline Execution** — execute tasks in this session with checkpoints  

Which approach?
