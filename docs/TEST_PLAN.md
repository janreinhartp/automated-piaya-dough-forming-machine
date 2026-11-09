# Test Plan

## Scope

Validate documentation assumptions, then firmware stages after implementation approval. Prefer tests that fail closed (safe outputs).

## 1. Unit testing (host or on-target)

| Area | Tests |
|------|-------|
| State machine | Transition table: Start→mix→…→complete; Pause/Resume preserves remainder; Stop→safe IDLE; qty loop count |
| Settings validation | Min/max reject; defaults load; magic/version mismatch → defaults |
| Relay HAL | Logical R1–R8 map to channels 1–8; press mutex rejects both-open; force_safe closes all |
| Modbus framing | FC06 request/CRC; open `0x0001`; close `0x0002`; close-all register `0x0000` data `0x0008`; retry exhaustion |
| Command queue | Recovery lock rejects Manual/Auto motion cmds |

## 2. Hardware bring-up

| Test | Method | Pass criteria |
|------|--------|---------------|
| Display/touch unchanged | Boot demo path / new HMI | 1024×600, touch aligns |
| RS485 direction bring-up | Check Waveshare silk/schematic and observe traffic | DE/RE behavior (automatic vs GPIO) is documented and requests transmit correctly |
| N4D8B08 communication | Send Modbus RTU requests at 9600 8N1 to slave 1 | Valid response for each channel command |
| Channel mapping | Open/close each channel | Channels 1–8 match R1–R8 and visibly/audibly switch |
| Safe boot | Power cycle | Close-all occurs before any UI Start; all relays remain closed |
| Coil/contactor side | With air/power locked out first | No unintended motion |

## 3. LVGL / HMI

| Test | Pass criteria |
|------|---------------|
| Boot shows Recovery | Cannot open Manual/Auto actuators until ack |
| Ack → Dashboard | Nav works; status updates |
| Settings save/reload | Power cycle retains values |
| History update | After completed auto run |
| No control in callbacks | Code review + stress: UI lag does not freeze outputs mid-Stop |
| Touch targets | Primary buttons usable with work gloves if required |

## 4. Manual operation

| Test | Pass criteria |
|------|---------------|
| Each actuator | Commanded state matches relay |
| Press FWD+REV | Impossible simultaneously |
| Confirm dialogs | Door/press/cutter require confirm |
| Mode exclusion | Auto running blocks Manual |
| Exit Manual with outputs on | Force safe OFF |

## 5. Automatic sequence

| Test | Pass criteria |
|------|---------------|
| Stage A timings | Mixer/door match settings ± tick tolerance |
| Stage B loop | Repeats until target; counter correct |
| Press pulse | R3 duration match; R4 stays OFF during advance |
| Pause | Motors off per policy; Resume continues |
| Stop | Immediate safe outputs; no resume without new Start |
| Complete | Production Complete UI; history written |

Use shortened timings for lab dry-runs without dough.

## 6. Pneumatic / press hold validation (critical)

| Test | Pass criteria |
|------|---------------|
| Timed advance + ASP | Measure creep after pulse under expected load |
| Multi-step accumulation | Steps remain usable for product size |
| Reverse reset | Manual REV returns press safely |
| Fail open documentation | If creep unacceptable → OPEN_DECISIONS escalation (valve change) |

## 7. Safety testing

| Test | Pass criteria |
|------|---------------|
| E-stop hit mid-cycle | All power down including HMI |
| Power restore | Recovery screen; no auto motion; outputs OFF |
| Ack without inspection text | Still requires button (procedural) |
| Unplug RS485 | Retries exhaust; `FAULT_IO_COMM`; no crash or silent continue; UI does not claim verified coil state |
| Illegal dual press command | HAL rejects |

## 8. Soak / robustness

- 100 dry auto cycles at short timings.
- UI navigation stress during run (status only).
- NVS wear: repeated Save ( moderatelys).

## Test evidence

Record: firmware commit, settings used, Modbus slave/baud, confirmed DE/RE behavior, channel map results, press creep measurements, pass/fail, operator initials.

## Related

- [SAFETY_AND_FAULT_HANDLING.md](SAFETY_AND_FAULT_HANDLING.md)
- [MACHINE_SEQUENCE.md](MACHINE_SEQUENCE.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
