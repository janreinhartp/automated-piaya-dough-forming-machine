# Safety and Fault Handling

## Design constraints (v1)

- No process or position sensors.
- Commanded actuator state ≠ verified mechanical position.
- Timer completion ≠ end-of-stroke confirmation.
- ESP32 is **not** the sole emergency stop mechanism.

## Emergency stop

### Hardware

- E-stop is **hardwired** and removes power from the machine, **including the ESP32/HMI**.
- Contactors, valve supplies, and controller power drop together per the electrical design.
- The MCU does not receive a live E-stop input in v1 (it is unpowered during the event).

### Software implications

- No soft E-stop ISR path required for v1.
- After power returns, treat boot as **untrusted machine state**.
- Never resume an interrupted automatic cycle.

## Recovery after E-stop / power loss

1. Boot with all logical outputs **safe OFF** before enabling control.
2. Discard ephemeral sequence state; do not restore “was mixing / was pressing”.
3. Load settings and history from NVS only.
4. Show **Recovery** screen; set `recovery_unlocked = false`.
5. Block Manual motion and Automatic Start until operator acknowledges inspection.
6. After ack: IDLE, unlocked; operator may Manual-home/clear then Start Auto.

UI checklist should include: E-stop released, air pressure, guards, press position, dough path, no jammed cutter/flattener.

## Safe output initialization

| Step | Action |
|------|--------|
| Before N4D8B08 available | Do not claim machine ready |
| RS485 / N4D8B08 init | Issue close-all before machine control starts |
| On Stop / Fault / mode exit | `relays_force_safe()` |
| On `machine_task` crash path | Best-effort safe write in shutdown hook if practical |

### Press valve

- Never energize R3 and R4 simultaneously (HAL hard check).
- Safe idle: both OFF.
- De-energizing does not guarantee cylinder stop; operator recovery must account for residual pneumatic motion/position.

## Interlocks

| Interlock | Rule |
|-----------|------|
| Press mutex | R3 XOR R4 only |
| Mode exclusion | Manual commands ignored in Auto; Auto Start ignored if Manual latched ON outputs (force safe first) |
| Recovery lock | Start Auto / Manual actuator cmds rejected until ack |
| Settings | Invalid settings block Start Auto |
| Spare | R8 remains OFF |

## Fault conditions (software)

| Code | Condition | Reaction |
|------|-----------|----------|
| `FAULT_IO_COMM` | Modbus timeout, bad response, or exhausted retries | Safe attempt; FAULT state; HMI banner; do not claim verified relay state |
| `FAULT_SETTINGS_INVALID` | Corrupt/out-of-range at Start | Reject start; message |
| `FAULT_INTERNAL` | Illegal state transition | Safe; FAULT |
| `FAULT_WATCHDOG` | If TWDT fires on machine task | Reset path → Recovery on reboot |

**Bring-up check (RS485):** with the N4D8B08 unplugged (or A/B disconnected), an FC06 write must **time out** and raise `FAULT_IO_COMM`. A successful FC06 response must never be a local TX echo (auto DE/RE turnaround echoing the request back). Waveshare DE/RE is automatic (OPEN_DECISIONS H1 closed).

Clearing fault: operator ack on HMI after condition resolved; return IDLE with recovery policy as appropriate (comm faults may require reboot).

## Task watchdog

- Consider enabling ESP Task WDT on `machine_task`.
- LVGL task should not starve machine_task (priority: machine > lvgl).
- Long LVGL work must not hold locks that block safe output updates.

## Reset / power recovery matrix

| Event | Auto-resume? | UI |
|-------|--------------|-----|
| E-stop (power kill) | **No** | Recovery screen |
| Unexpected brownout/reset | **No** | Recovery screen |
| Intentional Stop | N/A | IDLE Dashboard |
| Pause | Resume continues same run | Auto screen |

## Controlled shutdown

Stop command:

1. Enter `MACHINE_STOPPING`.
2. Motors OFF, valves safe.
3. IDLE.
4. Leave press where it is; display note that inspection may be needed before next Start.

## HMI honesty rules

- Labels: “Commanded ON/OFF”, never “At position”.
- Press hold: “Hold timer” not “Position locked” until bench validation promotes wording.
- Fault and Recovery use warn/fault colors per UI spec.

## Future expansion hooks

- N4D8B08 digital inputs may be considered after v1.
- Sensor-based press end stops.
- Dual-channel safety relays independent of ESP32 (already assumed for E-stop power).

## Related

- [MACHINE_SEQUENCE.md](MACHINE_SEQUENCE.md)
- [IO_MAPPING.md](IO_MAPPING.md)
- [TEST_PLAN.md](TEST_PLAN.md)
- [OPEN_DECISIONS.md](OPEN_DECISIONS.md)
