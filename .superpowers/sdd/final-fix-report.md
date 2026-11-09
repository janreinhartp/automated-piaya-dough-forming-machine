
# Final whole-branch fix pass (Important findings) - N4D8B08 Modbus relay

Branch: feat/n4d8b08-modbus-relay. Design truth: "Comm failure: retry 2-3 times; then FAULT_IO_COMM; do not claim verified coil state."

## 1. FAULT_IO_COMM on Modbus/relay failure
- `machine_sm_apply_outputs` now returns `esp_err_t` (first failure via ACC macro; all outputs still attempted).
- New `machine_sm_handle_io_error(sm, err)`: non-OK and not already `MACHINE_FAULT` -> `machine_sm_enter_fault(FAULT_IO_COMM)`. If already FAULT: log only.
- `machine_sm_enter_fault` logs (does not recurse) if apply_outputs fails in FAULT.
- All SM callers (start_auto, pause, resume, stop, enter(), COMPLETED, FLATTENER_RETRACTING->COUNTER_UPDATE) go through `apply_or_fault`.
- machine_task: CMD_START_AUTO checks pre-start `actuators_all_safe()` (fault on failure; also ignores start while in FAULT, which previously reset the fault); CMD_FAULT_CLEAR refuses to leave FAULT unless `actuators_all_safe()` returns OK; manual path `handle_manual` now returns `esp_err_t` (ESP_ERR_NOT_SUPPORTED = guard rejection, warn only; any other error = log + FAULT_IO_COMM); IO self-test aborts on first failure and reports it.
- Driver already retries 3x; no extra retry at SM layer.

## 2. Redundant Modbus traffic
- `relays_set`: skips Modbus when verified commanded state already matches; opposite press channel closed only if it is commanded ON (or state unverified). Added `s_unverified` flag (set on any failed write; cleared by successful close_all) which disables skipping so a lost response cannot hide a real coil state.
- `actuators_press_forward/reverse`: removed redundant opposite-close (relays_set already does it).
- Deadline arming after apply: NOT changed (would need a clock inside machine_sm.c / larger SM rewrite). Remaining gap: step_deadline is computed from tick `now_ms` before apply; slow Modbus (up to 3 retries x timeout) shortens the effective step. Skip-if-unchanged reduces typical traffic (most states re-command many unchanged outputs).

## 3. Honest commanded state
- `relays_force_safe`: clears `s_logical_on` only if `n4d8b08_close_all` succeeded; on failure mask unchanged, error logged, unverified set.
- `relays_apply_mask`: if close_all fails, returns immediately with mask untouched; mask bits set only after each successful channel write.

## 4. Docs
- SAFETY_AND_FAULT_HANDLING.md: bring-up check (unplug -> timeout, no local TX echo as FC06 success; DE/RE open).
- OPEN_DECISIONS.md H1: same note.

## Test evidence
- `idf.py build` (ESP-IDF v5.4.3, esp32s3, C:\esp\v5.4.3): full rebuild succeeded, exit 0; 16_LVGL_UI.bin 0x8fe50 bytes (93% free).
- No hardware/bench tests (out of scope). No unit test harness exists for machine_sm.
- Note: `build/` is tracked in git and was modified by the build; not committed.

## Remaining gaps
- Deadlines not re-armed after apply (see 2).
- Per-state output ordering in apply_outputs (e.g. enabling an output before closing others) unchanged.
- Bench: confirm timeout on unplug, DE/RE, channel map.
