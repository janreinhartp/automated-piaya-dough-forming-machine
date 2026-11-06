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
| `relay_active_low` | u8 | 1 | 0 | 1 | flag | 1 = active-LOW module |
| `pcf_addr` | u8 | 0 (auto) | 0 or 0x20–0x27 | — | addr | 0 = auto-detect; skip 0x24 |

Defaults are **starting estimates** for commissioning; tune on the real machine and Save.

## Validation rules

- Reject Save if any value outside min/max.
- Reject Start Auto if `target_qty < 1` or any duration critical path is zero where motion is required (`press_adv_ms`, `cut_ext_ms`, etc. must be ≥ min).
- `press_hold_ms` may be 0.
- Changing `relay_active_low` or `pcf_addr` requires safe outputs rewrite immediately after apply.
- Invalid NVS blob → load defaults and raise non-fatal settings warning on HMI.

## NVS key layout

```
namespace: piaya_cfg
  magic      u32   = 0x50494159  /* 'PIAY' */
  version   u16   = 1
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
    bool     relay_active_low;
    uint8_t  pcf8575_addr;
} machine_settings_t;
```

Single global copy owned by settings module; machine_task takes a const snapshot at Start Auto (or rereads under mutex on each step — prefer snapshot at Start for run consistency, allow Settings edit only when IDLE).

## UI behavior

- Edit only when machine IDLE (and not mid-fault clear if required).
- Save → write NVS → toast success/fail.
- Restore defaults → RAM only until Save (or Save immediately after confirm — prefer confirm then write).

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
