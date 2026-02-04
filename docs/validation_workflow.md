# Category Validation Workflow

Purpose: Work through each behavior category in order, fixing failures and adding a consolidated test once the category is fully green.

## Workflow (per category)
1) **Test each unvalidated behavior**
   - Update `include/test.txt` with a focused snippet for one behavior.
   - Run `make run`.
   - Record outcome in `FAILING.md` under that category:
     - `[PASS]` for confirmed behavior.
     - `[FAIL]` for broken behavior.
     - `[PARTIAL]` if incomplete/ambiguous.

2) **Fix failures and re‑test**
   - Implement fixes in compiler code.
   - Re-run the same focused `include/test.txt` snippet.
   - Flip `[FAIL]/[PARTIAL]` to `[PASS]` only after a clean `make run`.

3) **Create a category summary test**
   - Once **all** items in the category are `[PASS]`:
     - Add a single consolidated test in `tests/final/` that covers the category’s behaviors.
     - Ensure it is deterministic and easy to maintain.

4) **Run full tests and update goldens**
   - Run `make tests` (or `make final` / `make final-update` as needed).
   - If goldens change, update only the specific files that are affected.

5) **Promote to PASSING.md**
   - Move the validated category summary into `PASSING.md`.
   - Remove temporary per‑item tracking from `FAILING.md` for that category.

## Notes
- Keep `FAILING.md` as the live tracking doc while a category is in progress.
- Only promote a category after every behavior in it is confirmed passing.
- Keep fixes scoped; avoid unrelated refactors during a category pass.
