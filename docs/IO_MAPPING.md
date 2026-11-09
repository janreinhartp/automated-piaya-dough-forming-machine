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

## N4D8B08 relay channel mapping

| Logical | N4D8B08 channel | Register | Load |
|---------|-----------------|----------|------|
| R1 | 1 | `0x0001` | Mixer |
| R2 | 2 | `0x0002` | Door |
| R3 | 3 | `0x0003` | Press FWD |
| R4 | 4 | `0x0004` | Press REV |
| R5 | 5 | `0x0005` | Cutter |
| R6 | 6 | `0x0006` | Conveyor |
| R7 | 7 | `0x0007` | Flattener |
| R8 | 8 | `0x0008` | Spare |

The relay HAL maps logical R1–R8 to channels 1–8. N4D8B08 commands use Modbus FC06: data `0x0001` opens a channel, `0x0002` closes it, and register `0x0000` with data `0x0008` closes all channels.

## I2C allocation

| Device | Address | Pins | Role |
|--------|---------|------|------|
| I2C master | — | SDA GPIO8, SCL GPIO9 | Shared bus |
| GT911 | `0x5D` (`0x14` backup) | same | Touch |
| IO_EXTENSION | `0x24` | same | Board EXIO / PWM / ADC |

## RS485 allocation

| Device | Protocol | Pins | Configuration |
|--------|----------|------|---------------|
| N4D8B08 | Modbus RTU, 8N1 | TX GPIO16, RX GPIO15 | Slave 1, 9600 baud |

Relay polarity is not configurable. Firmware uses the N4D8B08 protocol's explicit open and close commands and issues close-all during initialization before machine control starts.

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

- Use the N4D8B08 relay contacts only within their electrical ratings.
- Solenoid supply and motor power on separate suitably fused circuits.
- E-stop cuts machine power including ESP32 (see Safety doc).
- Confirm whether the Waveshare RS485 interface controls DE/RE automatically or needs a GPIO during bring-up.

## Unresolved pin / wiring assignments

| Item | Status |
|------|--------|
| RS485 DE/RE control | Open: confirm auto-direction vs GPIO from board silk/schematic during bring-up |
| N4D8B08 supply voltage | Confirm 12 V vs 24 V as an electrical integration item |
| Contactor/VFD input wiring for R1/R6 | Mechanical/electrical design TBD |
| Valve coil voltage (12/24 V) | Confirm per purchased valves |
| E-stop power interrupt topology | Confirm which rails drop |

## Related

- [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
