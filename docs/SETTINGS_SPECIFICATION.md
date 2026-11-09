# Settings Specification

## Storage

| Item | Value |
|------|-------|
| Backend | ESP-IDF **NVS** |
| Namespace | `piaya_cfg` |
| Load | At boot before machine_task accepts Start |
| Save | Explicit Save on Settings screen |
| Defaults | `settings_restore_defaults()` + optional Save |

History uses a separate namespace `piaya_hist` (see History section).

## Parameters

| Key | Type | Default | Min | Max | Unit | Description |
|-----|------|---------|-----|-----|------|-------------|
| `mix_ms` | u32 | 60000 | 1000 | 3600000 | ms | Mixer run time |
| `door_open_ms` | u32 | 3000 | 200 | 60000 | ms | Door extend time |
| `door_close_ms` | u32 | 3000 | 200 | 60000 | ms | Door close/retract wait |
| `xfer_ms` | u32 | 2000 | 0 | 60000 | ms | Dough transfer dwell (door open) |
| `press_adv_ms` | u32 | 500 | 50 | 10000 | ms | Press forward pulse |
| `press_hold_ms` | u32 | 500 | 0 | 30000 | ms | Commanded hold after pulse |
| `cut_ext_ms` | u32 | 800 | 50 | 30000 | ms | Cutter extend |
| `cut_ret_ms` | u32 | 800 | 50 | 30000 | ms | Cutter return wait |
| `conv_ms` | u32 | 2000 | 100 | 120000 | ms | Conveyor run |
| `flat_ext_ms` | u32 | 1000 | 50 | 30000 | ms | Flattener extend |
| `flat_ret_ms` | u32 | 1000 | 50 | 30000 | ms | Flattener return wait |
| `target_qty` | u32 | 50 | 1 | 100000 | count | Production target |
| `modbus_slave_id` | u8 | 1 | 1 | 47 | ID | N4D8B08 Modbus slave ID |
| `modbus_baud` | u32 | 9600 | — | — | baud | N4D8B08 UART baud rate; 8N1; one of 2400, 4800, 9600, 19200 |

Defaults are **starting estimates** for commissioning; tune on the real machine and Save.
The Modbus keys may be stored in NVS; schema version 2 defaults are slave 1 and 9600 baud. There is no relay polarity setting.

## Validation rules

- Reject Save if any value outside min/max.
- Reject Start Auto if `target_qty < 1` or any duration critical path is zero where motion is required (`press_adv_ms`, `cut_ext_ms`, etc. must be ≥ min).
- `press_hold_ms` may be 0.
- `modbus_slave_id` must be 1–47 (default 1).
- `modbus_baud` must be one of 2400, 4800, 9600, or 19200 (default 9600). It is not a continuous min/max range.
- Validate Modbus settings before initializing the relay backend.
- Invalid NVS blob → load defaults and raise non-fatal settings warning on HMI.

## NVS key layout

```
namespace: piaya_cfg
  magic      u32   = 0x50494159  /* 'PIAY' */
  version   u16   = 2
  ...keys above...
```

Bump `version` on breaking layout changes; migration or reset to defaults.

## Runtime structure

```c
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
    uint8_t  modbus_slave_id;
    uint32_t modbus_baud;
} machine_settings_t;
```

Single global copy owned by settings module; machine_task takes a const snapshot at Start Auto (or rereads under mutex on each step — prefer snapshot at Start for run consistency, allow Settings edit only when IDLE).

## UI behavior

- Edit only when machine IDLE (and not mid-fault clear if required).
- Save → write NVS → toast success/fail.
- Restore defaults → RAM only until Save (or Save immediately after confirm — prefer confirm then write).
- Do not expose a relay polarity control. Keep the I/O self-test for R1–R8.

## Production history (related persistence)

Namespace `piaya_hist`:

| Key | Type | Description |
|-----|------|-------------|
| `last_qty` | u32 | Last completed run quantity |
| `total_cycles` | u32 | Lifetime forming cycles |
| `last_duration_ms` | u32 | Last run duration |
| `last_unix` | u32 | Timestamp if clock available; else 0 |

## Related

- [MACHINE_SEQUENCE.md](MACHINE_SEQUENCE.md)
- [LVGL_UI_SPECIFICATION.md](LVGL_UI_SPECIFICATION.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
