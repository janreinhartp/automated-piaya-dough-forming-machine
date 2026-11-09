# N4D8B08 Modbus Relay Backend — Design Spec

**Date:** 2026-11-09  
**Status:** Approved for implementation planning  
**Replaces:** PCF8575 I2C relay expander backend

## 1. Problem

PCF8575 I2C bit-banged control of a generic opto relay module was unreliable (address drift, write NACKs, ON without OFF). The installed hardware is an **Eletechsup N4D8B08**: an **RS485 Modbus RTU** 8-relay board. Driving it through PCF8575 is the wrong interface.

## 2. Goals

- Control logical outputs R1–R8 via N4D8B08 over Waveshare board RS485.
- Keep existing actuator API, machine state machine, and HMI command paths.
- Remove PCF8575 from the project.
- Preserve safe boot, press mutual exclusion, recovery lock, and E-stop power-kill model.

## 3. Non-goals

- Multi-slave Modbus networks.
- Generic third-party Modbus master library.
- Using N4D8B08 digital inputs in v1 (may be added later).
- Changing pneumatic/process sequence logic.

## 4. Locked decisions

| Item | Decision |
|------|----------|
| Transport | ESP32 UART RS485 (Waveshare pins: TX GPIO16, RX GPIO15) |
| Device | Eletechsup N4D8B08 only RS485 peripheral |
| Protocol | Modbus RTU, **9600 8N1**, slave ID **1** |
| Architecture | Thin N4D8B08 driver behind existing `relays` HAL (Approach 1) |
| PCF8575 | Remove driver and all call sites |

## 5. Architecture

```text
UI / machine_sm
  → actuators_*()
    → relays_set / relays_force_safe
      → n4d8b08_set_channel / n4d8b08_close_all
        → modbus_rtu_fc06 (UART RS485)
```

### New / changed modules

| Module | Action |
|--------|--------|
| `components/n4d8b08/` | **Add** — init UART, FC06 open/close, close-all, retries |
| `main/machine/relays.*` | **Rewrite** backend to N4D8B08; keep R1–R8 + press mutex |
| `components/pcf8575/` | **Delete** |
| `main/main.c` | Init RS485/N4D8B08 instead of PCF8575 |
| Actuators / SM / HMI screens | Keep APIs; drop polarity flip UI |

### Channel map

| Logical | N4D8B08 channel | Register |
|---------|-----------------|----------|
| R1 Mixer | 1 | 0x0001 |
| R2 Door | 2 | 0x0002 |
| R3 Press FWD | 3 | 0x0003 |
| R4 Press REV | 4 | 0x0004 |
| R5 Cutter | 5 | 0x0005 |
| R6 Conveyor | 6 | 0x0006 |
| R7 Flattener | 7 | 0x0007 |
| R8 Spare | 8 | 0x0008 |

### Modbus commands (module docs)

| Action | FC | Reg | Data |
|--------|----|-----|------|
| Open channel N | 06 | 0x000N | 0x0001 |
| Close channel N | 06 | 0x000N | 0x0002 |
| Close all | 06 | 0x0000 | 0x0008 |

CRC16 Modbus, low byte first. Inter-frame quiet time respected before each request. Optional later: FC03 read status for HMI verification.

## 6. Boot and safety

1. NVS / settings load  
2. Touch + LCD + LVGL (unchanged)  
3. UART RS485 init → `n4d8b08_init` → **close all**  
4. `machine_task` start with recovery lock  
5. Recovery HMI gate  

On Stop / safe / fault exit: close all (or close each channel).  
Press R3/R4 never both open.  
Comm failure: retry 2–3 times; then `FAULT_IO_COMM`; do not claim verified coil state.  
E-stop remains hardwired full power kill including ESP32.

## 7. Settings

- Remove: `pcf8575_addr`, `relay_active_low`  
- Optional NVS: `modbus_slave_id` (default 1), `modbus_baud` (default 9600)  
- HMI: remove “FLIP RELAY POLARITY”; keep “I/O SELF-TEST R1–R8”

## 8. Testing

- Bench: open/close each channel; hear/see relays; close-all on boot  
- Mutual exclusion: cannot open R3 and R4 together  
- Unplug RS485: fault path, no crash  
- Manual + short auto dry-run with safe loads  

## 9. Docs to update on implementation

`IO_MAPPING.md`, `HARDWARE_ARCHITECTURE.md`, `FIRMWARE_ARCHITECTURE.md`, `OPEN_DECISIONS.md`, `SAFETY_AND_FAULT_HANDLING.md`, `TEST_PLAN.md`, `SETTINGS_SPECIFICATION.md`, `PROJECT_OVERVIEW.md`

## 10. Open items (non-blocking)

- Confirm Waveshare RS485 DE/RE control (auto vs GPIO) against board silk/schematic during bring-up.  
- N4D8B08 supply 12 V vs 24 V is electrical only.  

## Spec self-review

- No TBD placeholders without an open item.  
- Consistent with Approach 1 and approved sections.  
- Scope is single backend swap; sequence/UI unchanged.  
- N4D8B08 open/close values explicitly `0x0001`/`0x0002` (not generic coil 0/1).
