# I/O Mapping

## Logical output allocation

| Logical ID | Component | Function | Energized meaning | Safe idle |
|------------|-----------|----------|-------------------|-----------|
| R1 | Mixer motor | ON/OFF | Motor run command asserted | OFF |
| R2 | Mixer door | Extend | Door open (cylinder extend) | OFF (spring return closed) |
| R3 | Press chamber | Forward | Advance press | OFF |
| R4 | Press chamber | Reverse | Retract / reset press | OFF |
| R5 | String cutter | Extend | Cutter out | OFF (spring return) |
| R6 | Conveyor motor | ON/OFF | Conveyor run command | OFF |
| R7 | Flattener | Extend | Flattener down/out | OFF (spring return) |
| R8 | Spare | Reserved | — | OFF |

### Interlocks (firmware)

- R3 and R4 are **mutually exclusive**; never both ON.
- Manual commands rejected while Automatic is active (and vice versa) except Stop / acknowledged Recovery flow.
- Spare R8 remains OFF unless explicitly assigned later.

## Proposed PCF8575 mapping (tentative)

| PCF8575 | Bit | Relay | Notes |
|---------|-----|-------|-------|
| Port0 | 0 | R1 | Mixer |
| Port0 | 1 | R2 | Door |
| Port0 | 2 | R3 | Press FWD |
| Port0 | 3 | R4 | Press REV |
| Port0 | 4 | R5 | Cutter |
| Port0 | 5 | R6 | Conveyor |
| Port0 | 6 | R7 | Flattener |
| Port0 | 7 | R8 | Spare |
| Port1 | 0–7 | — | Reserved (future inputs) |

**Unresolved until wiring confirmation:** bit order vs silkscreen IN1–IN8; may reverse if module wiring differs. HAL must map logical R1–R8, not raw bits, throughout the application.

## I2C allocation

| Device | Address | Pins | Role |
|--------|---------|------|------|
| I2C master | — | SDA GPIO8, SCL GPIO9 | Shared bus |
| GT911 | `0x5D` (`0x14` backup) | same | Touch |
| IO_EXTENSION | `0x24` | same | Board EXIO / PWM / ADC |
| PCF8575 | **Auto-detect `0x20`..`0x27`** (skip `0x24`) | same | Relay outputs |

### Address selection

A0–A2 jumpers select `0x20`..`0x27`. Firmware **auto-detects** at boot (skips board IO_EXTENSION `0x24`). Observed so far: `0x27`, then `0x21` after rewiring — jumpers unstable until fixed. Must not collide with `0x5D` / `0x14` (GT911).

## Signal polarity

| Setting | Behavior |
|---------|----------|
| Confirmed | **Active-LOW**: logical ON → drive expander bit LOW |
| Alternate | Active-HIGH still supported via NVS `relay_active_low` if hardware changes |
| Config | Default `relay_active_low = 1` (see Settings) |

**Confirmed by operator (2026-11-06):** module is active-LOW.

### Power-up note

PCF8575 ports typically read/write high after reset (quasi-bidirectional pull-ups). With active-LOW relays this usually means all relays OFF at power-up — desirable. Active-HIGH modules may briefly ON until firmware writes safe state; mitigate with hardware if needed.

## Board EXIO (do not steal for R1–R8)

| EXIO | Assignment |
|------|------------|
| 1 | TP_RST |
| 2 | Backlight |
| 3 | LCD reset (board) |
| 4 | SD CS |
| 5 | USB/CAN select |
| 0, 6, 7 | Free for future non-relay uses only if needed |

## ESP32 GPIO (claimed by demo — do not reassign without analysis)

| GPIO | Function |
|------|----------|
| 0,1,2,3,5,7,10,14,17,18,21,38–42,45–48 | RGB |
| 4 | Touch INT |
| 6 | LED |
| 8,9 | I2C |
| 11–13 | SDMMC |
| 15,16 | RS485 |
| 19,20 | TWAI |

## Electrical requirements

- Galvanic isolation between MCU side and power/pneumatic side (relay module optos or equivalent).
- Common GND reference only where the module datasheet requires it for IN pins.
- Solenoid supply and motor power on separate suitably fused circuits.
- E-stop cuts machine power including ESP32 (see Safety doc).

## Unresolved pin / wiring assignments

| Item | Status |
|------|--------|
| PCF8575 7-bit address | **Confirmed `0x27` (I2C scan)** |
| Relay active level | **Confirmed active-LOW** |
| Port0 bit ↔ IN1..IN8 order | Tentative; confirm |
| Contactor/VFD input wiring for R1/R6 | Mechanical/electrical design TBD |
| Valve coil voltage (12/24 V) | Confirm per purchased valves |
| E-stop power interrupt topology | Confirm which rails drop |

## Related

- [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
