# HMI Figma Fidelity Assessment

**Date:** 2026-11-12  
**Figma:** [Piaya Dough Maker](https://www.figma.com/design/u4bmiTusgnb74KnpQu58xt/Piaya-Dough-Maker) (`fileKey=u4bmiTusgnb74KnpQu58xt`)  
**Target:** Waveshare ESP32-S3-Touch-LCD-7B, 1024×600, LVGL 8.4

## Decision

Automatic **option C**: tab = frame `04 Automatic Production`; Start / Run Auto opens run-view based on `Automatic • Idle` family. Evolve under `main/hmi/` (no `components/ui` move).

## Key Figma node IDs

| Frame | Node ID |
|-------|---------|
| 01 Recovery | `2:12000` |
| 02 Dashboard | `7:3750` |
| 03 Manual Control | `7:3754` |
| 04 Automatic Production | `7:3758` |
| Retained machine assembly (in 04) | `7:3926` |
| 05 Machine Settings | `7:3762` |
| 06 Production History | `7:3766` |
| Automatic • Idle (run-view) | `2:12208` |
| Automatic • Mixing | `2:12416` |
| Automatic • Fault | `2:15536` |
| Confirm dialogs | `7:4236`–`7:4240` |

## Current firmware gaps

| Area | Gap |
|------|-----|
| Auto drawing | Text pills only; no assembly image or SM highlight |
| Assets | No `lv_img_dsc_t` drawings; PNG decoder off |
| Settings | Textareas without touch numeric input |
| Fault | `CMD_FAULT_CLEAR` unused by HMI |
| Manual | No `manual_allowed` / fault gating |
| Glyphs | U+2014 / U+B7 missing in Montserrat |
| Preview copy | Figma/firmware still say “not connected” |

## Architecture (implemented)

- `main/hmi/graphics/` — `process_map`, `machine_drawing`, `img_machine_assembly` (RGB565 `lv_img_dsc_t`)
- `main/hmi/screens/scr_auto_run.c` — run-view (option C)
- UI observes `machine_status_get`; commands via `machine_cmd_send` only

## Remaining limitations

- Assembly is a single exported bitmap + station rail overlays (not per-vector SVG parts).
- Figma “PREVIEW” chrome removed; live machine is assumed connected.
- Montserrat still lacks some Unicode; UI uses ASCII `-` / `|`.
- Run-view RESET maps to `CMD_STOP` (or `CMD_FAULT_CLEAR` when faulted); no separate SM reset command.
- Settings numeric `lv_keyboard` covers the nav bar while open (dismiss with Ready/Cancel).
- History Unix timestamp remains `0` until RTC/timekeeping is added to the controller.
