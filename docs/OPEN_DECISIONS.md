# Open Decisions

Items that are intentionally unresolved or require bench/operator confirmation before commissioning. Firmware architecture can proceed with documented defaults.

## Hardware

| ID | Topic | Current assumption | Needs |
|----|-------|--------------------|-------|
| H1 | PCF8575 7-bit address | **Auto-detect**; jumpers flaky (`0x27` then `0x21`) | Stabilize A0–A2, then note final addr |
| H2 | Relay module polarity | **Active-LOW confirmed** | Done |
| H3 | Port0 bit order vs IN1–IN8 | Bit0=R1 … Bit7=R8 | Confirm wiring |
| H4 | Valve coil voltage | Likely 24 V DC (unconfirmed) | Nameplate check |
| H5 | Mixer/conveyor interface | Contactor or VFD DI via relay | Electrical design |
| H6 | Relay duty vs solenoid rate | Module adequate for lab | Check contact rating vs cycle rate |
| H7 | E-stop power topology | Kills ESP32 + loads | Confirm which rails drop |
| H8 | Board EXIO0/6/7 use | Unused | Keep free unless needed |

## Mechanical / pneumatic

| ID | Topic | Current assumption | Risk |
|----|-------|--------------------|------|
| M1 | Press mid-hold with **4V220-08 + ASP330F** | Timed pulse then both solenoids OFF holds step (per operator video) | 5/2 detented valve may leave one side pressurized; **creep possible** |
| M2 | Incremental repeatability without sensors | Time + ASP sufficient for product | Dough load variation |
| M3 | End-of-batch press reset | Manual reverse when needed | Not automated in Stage B |
| M4 | Door/cutter/flattener spring return times | Settings mins | Tune on machine |

**Escalation for M1:** If bench shows unacceptable creep, switch press valve to **5/3 closed-center** (e.g. 4V230C class) and extend HAL with Center/Hold command — architecture otherwise unchanged.

## Electrical uncertainties

| ID | Topic | Notes |
|----|-------|-------|
| E1 | Isolation between PCF8575 and relay IN | Follow module datasheet (VCC/JD-VCC jumpers) |
| E2 | Shared grounds | Avoid ground loops with motor VFDs |
| E3 | Inrush on ESP32 supply after E-stop restore | Ensure clean reset |

## Software / product

| ID | Topic | Current assumption |
|----|-------|--------------------|
| S1 | Demo Wi-Fi/SD/RS485/CAN | **Removed** from tree (2026-11-06) |
| S2 | RTC for history timestamps | Best-effort; may be `0` without SNTP/RTC |
| S3 | Language | English v1 |
| S4 | SquareLine deletion | **Removed** `main/ui` + demo `main/user` (2026-11-06) |
| S5 | C vs C++ for new modules | Prefer C to match demo |

## Operator confirmations required

1. Final PCF8575 address and relay polarity.  
2. Accept Recovery-screen workflow (option B locked).  
3. Bench sign-off on press hold behavior (M1).  
4. Contactor/VFD wiring diagram before energizing motors.  
5. Explicit **architecture approval** before firmware implementation phase.

## Decision log

| Date | Decision |
|------|----------|
| 2026-11-06 | External PCF8575 on I2C 8/9 (not onboard IO_EXTENSION for R1–R8) |
| 2026-11-06 | Keep 4V220-08 + ASP330F; timed pulse advance; validate hold on bench |
| 2026-11-06 | Relay polarity configurable; default active-LOW |
| 2026-11-06 | E-stop kills whole machine including ESP32 |
| 2026-11-06 | Dedicated Recovery screen before Manual/Auto |
| 2026-11-06 | Preserve Waveshare LCD/touch/LVGL; replace application UI layer |
| 2026-11-06 | Architecture **approved**; firmware implementation started |
| 2026-11-06 | PCF8575 address initially assumed 0x20; relay polarity **active-LOW** confirmed |
| 2026-11-06 | I2C scan found PCF8575 at **0x27** (with 0x24 IO_EXTENSION, 0x5D GT911); firmware default updated |
| 2026-11-06 | Mechanical completion pending operator update before press-hold / full machine commissioning |
| 2026-11-06 | Stripped unused demo: `main/ui`, `main/user`, and components twai/usart/sd/gui_paint/fonts/image |

## Related

- [IO_MAPPING.md](IO_MAPPING.md)
- [HARDWARE_ARCHITECTURE.md](HARDWARE_ARCHITECTURE.md)
- [SAFETY_AND_FAULT_HANDLING.md](SAFETY_AND_FAULT_HANDLING.md)
