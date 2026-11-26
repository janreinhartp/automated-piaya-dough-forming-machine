# Settings persistent keypad — implementation plan

> **For agentic workers:** Complete tasks in order. One file: `main/hmi/screens/scr_settings.c`.

**Goal:** Left scrollable settings + always-visible right numeric keypad (never overlays fields).

**Spec:** `docs/superpowers/specs/2026-11-26-settings-persistent-keypad-design.md`

## Task 1: Relayout settings screen

- [x] Narrow left list (~640×340), single-column fields
- [x] Dock `lv_keyboard` in right panel; never hide; idle until focus
- [x] Keep Save / Restore defaults / status under the left list

## Verification

- [ ] Keypad visible on open without tapping a field
- [ ] Scroll to Target qty / Flattener return with keypad still visible
- [ ] Edit those fields; Save still works
