# Project Overview — Automated Piaya Dough Forming Machine

## Objectives

Build an industrial touchscreen control system for an automated piaya dough forming machine on the existing Waveshare **ESP32-S3-Touch-LCD-7B** ESP-IDF demo.

The controller must:

- Run **Manual** and **Automatic** operating modes.
- Sequence mixer, mixer door, press chamber, string cutter, conveyor, and flattener.
- Provide a professional LVGL HMI on the 7-inch RGB touchscreen.
- Use **time-based** motion control (no process/position sensors in v1).
- Preserve the working Waveshare board bring-up (LCD, touch, LVGL port, I2C, IO_EXTENSION).

## Hardware overview

| Item | Role |
|------|------|
| Waveshare ESP32-S3-Touch-LCD-7B | Main HMI/controller |
| Onboard IO_EXTENSION @ I2C `0x24` | Board functions (touch reset, backlight, SD CS, USB/CAN mux) |
| External PCF8575 @ I2C (`0x27`) | 8 digital outputs to relay module inputs |
| 8-channel relay module | Isolates ESP outputs from contactors / valve drivers |
| Pneumatic valves | Mixer door, press (4V220-08), cutter, flattener |
| ASP330F both ends of press cylinder | Speed control + intended mid-stroke hold aid |
| Motors | Mixer and conveyor via appropriately rated contactors/VFD interfaces |

Emergency stop is a **hardwired** circuit that removes power from the entire machine, **including the ESP32**. Recovery after restore is a mandatory HMI gate (no auto-resume).

## Software architecture (summary)

```
app_main
  ├── Board init (NVS, GT911, RGB LCD, LVGL port)   [existing Waveshare path]
  ├── PCF8575 + relay HAL                            [new]
  ├── machine_task (state machine, owns outputs)     [new]
  └── HMI screens via LVGL (Recovery first)          [main/hmi — SquareLine demo removed]
```

Machine sequencing runs in a FreeRTOS task independent of LVGL callbacks. The UI sends commands through a queue and displays a status snapshot.

## Development scope

### In scope (v1)

- Preserve display/touch/LVGL port configuration.
- PCF8575 driver, relay HAL, actuator API, interlocks.
- Explicit machine state machine (idle → mix → transfer → repeating form cycle → complete/fault).
- LVGL screens: Recovery, Dashboard, Manual, Automatic, Settings, History.
- NVS settings and production history.
- Safety: safe boot outputs, press mutual exclusion, mode exclusion, recovery lock, fault display.
- Project documentation under `docs/`.

### Out of scope (v1)

- Process sensors / closed-loop position feedback.
- Soft E-stop input to the ESP32 (power is removed from the MCU).
- Replacing LCD/touch drivers or upgrading LVGL to v9.
- Direct relay coil drive from PCF8575 (must use relay module / drivers).
- Full firmware implementation until architecture approval.

## Design principles

1. Inspect before modifying; do not invent GPIO assignments.
2. Separate UI from machine control.
3. Commanded motion ≠ verified physical position.
4. Safe outputs on boot; no automatic production resume after power loss / E-stop.
5. Keep Waveshare board components intact unless a change is documented and approved.

## Related documents

- [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)
- [IO_MAPPING.md](IO_MAPPING.md)
- [MACHINE_SEQUENCE.md](MACHINE_SEQUENCE.md)
- [LVGL_UI_SPECIFICATION.md](LVGL_UI_SPECIFICATION.md)
- [FIRMWARE_ARCHITECTURE.md](FIRMWARE_ARCHITECTURE.md)
- [SETTINGS_SPECIFICATION.md](SETTINGS_SPECIFICATION.md)
- [SAFETY_AND_FAULT_HANDLING.md](SAFETY_AND_FAULT_HANDLING.md)
- [TEST_PLAN.md](TEST_PLAN.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
