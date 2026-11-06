# Piaya Dough Forming Machine HMI — Design Spec

**Date:** 2026-11-06  
**Status:** Architecture documented; firmware implementation gated on explicit approval  
**Basis:** Waveshare ESP32-S3-Touch-LCD-7B ESP-IDF demo in workspace root

## 1. Problem

Control an automated piaya dough forming machine (mixer, door, press, cutter, conveyor, flattener) from a 7-inch industrial LVGL HMI without process sensors, using time-based sequencing, while preserving a working Waveshare display/touch stack.

## 2. Constraints

- Do not replace RGB LCD, GT911, or `lvgl_port` integration.
- ESP GPIOs are essentially fully consumed; expand via I2C.
- Onboard IO_EXTENSION @ `0x24` is not a PCF8575 and is reserved for board functions.
- E-stop removes power from the entire machine including the ESP32.
- No automatic resume after power loss / E-stop.

## 3. Locked decisions

| Topic | Decision |
|-------|----------|
| I/O expansion | External **PCF8575** on I2C GPIO 8/9; addr `0x27` (I2C scan) |
| Relays | 8-ch module; logical R1–R8; polarity configurable, default active-LOW |
| Press valve | Keep **4V220-08** + **ASP330F** both ends; timed Forward pulse; bench-validate hold |
| E-stop | Hardwired full power kill including MCU |
| Recovery UX | Dedicated **Recovery** screen; Manual/Auto blocked until acknowledge |
| App strategy | Preserve board layer; replace SquareLine demo application UI |
| LVGL | Stay on vendored **8.4.0**, 1024×600 |

## 4. Architecture

### 4.1 Layers

1. **Board** — existing touch, RGB LCD, IO_EXTENSION, I2C, LVGL port  
2. **HAL** — `pcf8575` → `relays` → `actuators`  
3. **Control** — `machine_task` + state machine + settings/history/safety  
4. **HMI** — Recovery, Dashboard, Manual, Auto, Settings, History  

Machine control never runs inside LVGL event callbacks. Commands use a FreeRTOS queue; status uses a snapshot.

### 4.2 Boot

NVS → GT911 → RGB LCD → backlight → LVGL → PCF8575 safe OFF → `machine_task` (recovery locked) → HMI Recovery screen.

### 4.3 Automatic sequence

Stage A: mix → door open → transfer → door close → init counter.  
Stage B loop: press pulse → hold timer → cut → convey → flatten → count until target → complete/safe.

Pause freezes step remainder with motors off and spring-return valves de-energized; press solenoids OFF. Stop forces safe idle without auto-homing the press.

### 4.4 Safety

- Safe output image on boot and Stop/Fault.  
- Press R3/R4 mutual exclusion in HAL.  
- Mode exclusion and recovery lock.  
- UI distinguishes commanded vs verified motion.  
- Press hold wording remains cautious until bench sign-off (OPEN M1).

## 5. Deliverables

### Documentation (this phase)

Canonical docs under `docs/`:

- PROJECT_OVERVIEW.md  
- HARDWARE_ARCHITECTURE.md  
- IO_MAPPING.md  
- MACHINE_SEQUENCE.md  
- LVGL_UI_SPECIFICATION.md  
- FIRMWARE_ARCHITECTURE.md  
- SETTINGS_SPECIFICATION.md  
- SAFETY_AND_FAULT_HANDLING.md  
- TEST_PLAN.md  
- OPEN_DECISIONS.md  

### Firmware (after approval only)

Implementation order:

1. PCF8575 + relay HAL + safe boot  
2. Actuator API + interlocks  
3. Machine state machine  
4. Settings NVS  
5. HMI shell + Recovery  
6. Remaining screens  
7. History  
8. Integration tests  
9. Demo UI / unused peripheral modules removed from tree (2026-11-06)

## 6. Open risks

See `docs/OPEN_DECISIONS.md`. Highest risk: **M1** press mid-hold with 4V220 + ASP under load. Escalation path: 5/3 closed-center valve without redesigning the rest of the stack.

## 7. Approval gate

Firmware implementation must not begin until the project owner explicitly approves this architecture and the `docs/` set.

## Spec self-review

- No TBD placeholders left without an OPEN_DECISIONS entry.  
- Press hold documented as commanded + validation required (consistent across Safety, Sequence, Open Decisions).  
- Scope is one product: HMI + sequencer on Waveshare board; not a multi-repo platform.  
- Ambiguity on relay polarity resolved via configurable default + bench confirm.
