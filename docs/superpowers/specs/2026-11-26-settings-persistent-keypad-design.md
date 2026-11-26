# Settings persistent keypad layout

**Date:** 2026-11-26  
**Status:** Approved for implementation  
**Screen:** `main/hmi/screens/scr_settings.c` (1024×600)

## Problem

The settings screen shows a bottom-docked `lv_keyboard` on field focus. It covers the lower rows (Flattener return, Target qty), so those values cannot be seen while editing.

## Decision

Split the content area:

| Region | Role |
|--------|------|
| Left | Scrollable settings fields + Save / Restore defaults / status |
| Right | Always-visible numeric `lv_keyboard`, idle until a field is focused |

No overlay. Keyboard is never hidden.

## Behavior

1. On screen open, keypad is visible with no bound textarea (idle).
2. Tap a field → bind keypad to that textarea; typing updates the field.
3. Keypad OK/Cancel → unbind textarea only; keypad stays visible and idle.
4. Fields are a single scrollable column so Target qty and Flattener return remain reachable without being covered.

## Non-goals

- Custom drawn numpad widgets
- Changing settings validation / NVS save path
- Changing other HMI screens

## Verification

- Open Settings: keypad visible on the right immediately.
- Scroll left list to Flattener return and Target qty with keypad still visible.
- Edit those fields; values remain visible while typing.
- Save / Restore defaults still work.
