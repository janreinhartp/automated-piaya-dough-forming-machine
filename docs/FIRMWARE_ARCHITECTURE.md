# Firmware Architecture

## Goals

- Modular ESP-IDF application on the existing Waveshare demo.
- Machine control independent of LVGL.
- Hardware abstraction so app code never touches raw relay bits.
- Non-blocking timing (FreeRTOS + elapsed time).
- C preferred for parity with demo; C++ allowed where it does not break component build.

## Preserve vs replace

| Keep unchanged (unless documented) | Removed |
|------------------------------------|---------|
| `components/rgb_lcd_port` | `main/ui` SquareLine demo (2026-11-06) |
| `components/touch` (GT911) | `main/user` (wifi/rs485/can/pwm/sd/password) |
| `components/lvgl_port` | `components/twai`, `usart`, `sd` |
| `components/i2c` | `components/gui_paint`, `fonts`, `image` |
| `components/io_extension` | `components/pcf8575` (2026-11-09; replaced by `n4d8b08`) |
| `sdkconfig.defaults` display/PSRAM baseline | |

## Proposed folder structure

```
components/
  n4d8b08/                 # Modbus RTU driver for N4D8B08
  i2c/ rgb_lcd_port/ touch/ lvgl_port/ io_extension/ ...
main/
  main.c                   # Bring-up orchestration only
  machine/
    machine_sm.c/.h        # State machine
    machine_task.c/.h      # FreeRTOS task + command queue
    actuators.c/.h         # Logical actuator API
    relays.c/.h            # Logical relay HAL + interlocks
    settings.c/.h          # NVS settings
    history.c/.h           # Production history
    safety.c/.h            # Recovery lock, fault helpers
    machine_types.h        # Shared enums/structs
  hmi/
    hmi_init.c/.h
    hmi_theme.c/.h
    hmi_nav.c/.h
    hmi_timers.c/.h
    screens/
      scr_recovery.c
      scr_dashboard.c
      scr_manual.c
      scr_auto.c
      scr_settings.c
      scr_history.c
```

## Module responsibilities

| Module | Responsibility |
|--------|----------------|
| `main.c` | NVS, touch, LCD, LVGL, RS485/N4D8B08 safe init, start `machine_task`, `hmi_init` |
| `n4d8b08` | UART RS485 init; Modbus RTU FC06 open/close and close-all; retries |
| `relays` | Logical R1–R8 → N4D8B08 channels 1–8; force-safe; press mutex at write gate |
| `actuators` | Named APIs: mixer_on/off, door_open/close, press_fwd/rev/coast, … |
| `machine_sm` | Pure-ish transitions given events + elapsed time |
| `machine_task` | Owns queue, tick, calls SM + actuators |
| `settings` | Load/save/defaults/validate |
| `history` | Persist last run stats |
| `safety` | `recovery_unlocked`, fault latch helpers |
| `hmi_*` | Screens; enqueue commands; render snapshot |

## Task architecture

| Task | Priority (indicative) | Notes |
|------|----------------------|-------|
| `lvgl` | 2 (existing) | Unchanged port |
| `machine_task` | 5–6 | Higher than LVGL; owns outputs |
| Demo wifi/pwm | Off in production | |

`machine_task` loop sketch:

1. Receive commands (non-blocking).
2. Read elapsed time.
3. Step state machine.
4. Apply actuator requests through HAL.
5. Publish status snapshot.
6. `vTaskDelay` short period (e.g. 10–20 ms).

## Hardware abstraction

```
UI / SM → actuators_*() → relays_set(logical, on) → n4d8b08_set_channel(channel, open)
```

Application code must not include Modbus frame or N4D8B08 register details outside `components/n4d8b08` and `relays`.

## Data structures (conceptual)

```c
typedef enum {
    MACHINE_IDLE,
    MACHINE_MIXING,
    /* ... full set per MACHINE_SEQUENCE.md ... */
    MACHINE_FAULT
} machine_state_t;

typedef struct {
    machine_state_t state;
    machine_state_t state_before_pause;
    bool recovery_unlocked;
    bool auto_active;
    bool manual_active;
    uint32_t target_qty;
    uint32_t current_qty;
    uint32_t step_deadline_ms;
    uint8_t commanded_outputs; /* bit flags R1..R8 */
    uint32_t fault_code;
} machine_status_t;

typedef enum {
    CMD_RECOVERY_ACK,
    CMD_START_AUTO,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_STOP,
    CMD_MANUAL_*,
    CMD_SETTINGS_RELOAD
} machine_cmd_id_t;
```

Centralized timing/config lives in `machine_settings_t` (see Settings spec).

## Communication

| Path | Mechanism |
|------|-----------|
| HMI → Machine | FreeRTOS queue `machine_cmd_t` |
| Machine → HMI | Lock-free or mutex-protected `machine_status_t` snapshot |
| Settings | NVS; machine reloads on save notify |
| LVGL thread safety | Always `lvgl_port_lock` around LVGL API from non-lvgl tasks |
| Relay transport | Modbus RTU 9600 8N1, slave 1, TX GPIO16 / RX GPIO15 |

## Boot sequence

1. `init_nvs` / settings load  
2. `touch_gt911_init` (brings up I2C + IO_EXTENSION)  
3. `waveshare_esp32_s3_rgb_lcd_init` + backlight on  
4. `lvgl_port_init`  
5. UART RS485 / `n4d8b08_init` + close-all / `relays_force_safe()`
6. Start `machine_task` with `recovery_unlocked = false`  
7. `hmi_init` → show Recovery  

RS485 DE/RE is automatic on the Waveshare SP3485 (ESP TX GPIO16 / RX GPIO15). N4D8B08 FC06 data words use Command<<8|Delay (`0x0100` open, `0x0200` close, `0x0800` close-all).

## Coding rules

- No `delay()` in SM long paths; use deadlines.
- No relay writes in LVGL callbacks.
- No hardcoded timings outside settings defaults table.
- Small functions; one module responsibility.
- Log commanded vs physical honestly in UI strings.

## Related

- [MACHINE_SEQUENCE.md](MACHINE_SEQUENCE.md)
- [IO_MAPPING.md](IO_MAPPING.md)
- [SETTINGS_SPECIFICATION.md](SETTINGS_SPECIFICATION.md)
