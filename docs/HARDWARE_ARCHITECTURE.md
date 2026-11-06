# Hardware Architecture

## Controller — Waveshare ESP32-S3-Touch-LCD-7B

| Property | Value (from demo) |
|----------|-------------------|
| MCU | ESP32-S3 |
| ESP-IDF | ≥ 5.1.0 |
| Flash | 16 MB, QIO, 120 MHz |
| PSRAM | Octal, 120 MHz (framebuffers in PSRAM) |
| CPU | 240 MHz |
| Display | ST7262 RGB parallel, **1024 × 600**, 16 bpp |
| Touch | GT911, I2C addr `0x5D` (backup `0x14`) |
| I2C bus | GPIO **8** (SDA), GPIO **9** (SCL), 400 kHz, `I2C_NUM_0` |
| LVGL | Vendored **8.4.0** via `components/lvgl_port` |

### Onboard IO_EXTENSION (not PCF8575)

| Item | Value |
|------|-------|
| Address | `0x24` |
| API | `components/io_extension` |
| EXIO1 | Touch reset |
| EXIO2 | Backlight / DISP_EN |
| EXIO3 | LCD reset (defined; usage limited) |
| EXIO4 | SD CS |
| EXIO5 | USB vs CAN mux |
| EXIO0 / EXIO6 / EXIO7 | Appears unused (~3 free lines only) |

Do **not** repurpose EXIO1/2/4/5 for machine relays. Prefer external PCF8575 for R1–R8.

### ESP GPIO reality

RGB data/control, SDMMC, RS485, TWAI, I2C, and touch INT consume essentially all convenient ESP GPIOs in this demo. **Eight relay channels must not be assigned to invent “free” ESP pins.** Expansion is via I2C (PCF8575).

## External PCF8575

| Item | Decision |
|------|----------|
| Purpose | Digital outputs to 8-channel relay module inputs |
| Bus | Same I2C0 (GPIO 8/9) |
| Address | Auto-detect `0x20`..`0x27` (A0–A2 jumpers; skip board `0x24`) |
| Forbidden addresses | `0x24` (IO_EXTENSION), `0x5D` / `0x14` (GT911) |
| Port0 | Tentative: bits 0–7 → R1–R8 |
| Port1 | Reserved for future inputs |
| Behavior | Quasi-bidirectional; write full port images; power-up tends high |

PCF8575 outputs must **not** drive relay coils directly. Drive opto-isolated relay module inputs (or equivalent driver stage) only.

## Relay module (8-channel)

| Channel | Logical load |
|---------|----------------|
| R1 | Mixer motor command (to contactor/VFD run input) |
| R2 | Mixer door extend solenoid |
| R3 | Press forward solenoid |
| R4 | Press reverse solenoid |
| R5 | Cutter extend solenoid |
| R6 | Conveyor motor command |
| R7 | Flattener extend solenoid |
| R8 | Spare |

### Electrical rules

- Relay PCB contacts are for **control signals**, not raw motor current.
- Mixer/conveyor: use contactors or VFD digital inputs sized for the motor.
- Solenoids: ensure relay/driver ratings match coil voltage/current and expected cycle rate.
- Polarity: software-configurable; **default active-LOW**; bench-confirm.
- Active-LOW + PCF8575 power-up high generally maps to OFF — still validate.

## Pneumatic valves

| Function | Valve | Notes |
|----------|-------|-------|
| Mixer door | 4V210-08 (single solenoid) | Spring return; de-energize → retract |
| Press chamber | **4V220-08** (double solenoid) | Mutual exclusion R3/R4; detented 5/2 |
| String cutter | 4V210-08 | Spring return |
| Flattener | 4V210-08 | Spring return |

### Press cylinder accessories

- **ASP330F** (or ASP330F-01-06S class) on **both** cylinder ports: speed control + pilot-check style locking aid.
- Incremental advance: timed **Forward** pulse (R3), then both solenoids OFF; do not reverse each cycle.
- **Bench validation required:** confirm mid-stroke hold under real load. Theory note: 4V220 is 5/2 detented (no closed center); hold depends on the as-built pneumatic behavior observed on the machine.

## Motors

| Motor | Control path |
|-------|----------------|
| Mixer | R1 → contactor/VFD |
| Conveyor | R6 → contactor/VFD |

Never switch motor mains through undersized module contacts.

## Electrical interfaces (summary)

```
ESP32-S3 I2C (8/9)
  ├── GT911 @ 0x5D
  ├── IO_EXTENSION @ 0x24  → board backlight / TP_RST / SD / mux
  └── PCF8575 @ 0x27 → relay IN1..IN8 → contactors / valve drivers
```

Hardwired E-stop removes power from machine loads **and** the HMI/ESP32 supply path used in this design.

## Related

- [IO_MAPPING.md](IO_MAPPING.md)
- [SAFETY_AND_FAULT_HANDLING.md](SAFETY_AND_FAULT_HANDLING.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
