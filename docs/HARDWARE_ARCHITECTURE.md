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

### Onboard IO_EXTENSION

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

Do **not** repurpose EXIO1/2/4/5 for machine relays. R1–R8 are controlled by the external N4D8B08 over RS485.

### ESP GPIO reality

RGB data/control, SDMMC, RS485, TWAI, I2C, and touch INT consume essentially all convenient ESP GPIOs in this demo. **Eight relay channels must not be assigned to invented “free” ESP pins.** Relay control uses the board RS485 interface on TX GPIO16 and RX GPIO15.

## External Eletechsup N4D8B08

| Item | Decision |
|------|----------|
| Purpose | Eight relay outputs for contactor and valve control |
| Transport | Waveshare ESP32 UART RS485: TX GPIO16, RX GPIO15 |
| Protocol | Modbus RTU, 9600 8N1 |
| Slave | ID 1; only RS485 peripheral in v1 |
| Commands | FC06 channel open/close and close-all |
| Direction control | Waveshare SP3485 automatic DE/RE (no GPIO) |

## Relay channels

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
- Firmware issues the N4D8B08 close-all command during initialization before machine control starts.
- Confirm whether the installed N4D8B08 is supplied at 12 V or 24 V as part of electrical integration.

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
ESP32-S3
  ├── I2C GPIO8/9
  │   ├── GT911 @ 0x5D
  │   └── IO_EXTENSION @ 0x24 → board backlight / TP_RST / SD / mux
  └── RS485 TX GPIO16 / RX GPIO15
      └── N4D8B08 slave 1 → relay channels 1..8 → contactors / valves
```

Hardwired E-stop removes power from machine loads **and** the HMI/ESP32 supply path used in this design.

## Related

- [IO_MAPPING.md](IO_MAPPING.md)
- [SAFETY_AND_FAULT_HANDLING.md](SAFETY_AND_FAULT_HANDLING.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
