"""Verify non-recording runs preserve timing inputs and report child results."""
import json
import csv
from make_final_timing_common import CSV_COLUMNS
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

RUNNER = Path(__file__).with_name("run_make_final_monitored.py")

class NonRecordingTests(unittest.TestCase):
    def test_success_and_failure_preserve_existing_and_absent_history(self):
        for existing in (False, True):
            for exit_code in (0, 7):
                with self.subTest(existing=existing, exit_code=exit_code), tempfile.TemporaryDirectory() as tmp:
                    root = Path(tmp)
                    csv_path = root / "history.csv"
                    notes = root / "notes.md"
                    baseline = root / "baseline.json"
                    if existing:
                        with csv_path.open("w", newline="") as stream:
                            writer = csv.DictWriter(stream, fieldnames=CSV_COLUMNS)
                            writer.writeheader()
                            row = {key: "1" for key in CSV_COLUMNS}
                            row.update(timestamp_utc="2026-01-01T00:00:00Z", git_head="fixture", tag="test", last_run_status="ok")
                            writer.writerow(row)
                        notes.write_text("preserve notes\n")
                        baseline.write_text('{"preserve": true}\n')
                    before = {p: p.read_bytes() if p.exists() else None for p in (csv_path, notes, baseline)}
                    proc = subprocess.run([sys.executable, str(RUNNER), "--no-record-history",
                        "--csv", str(csv_path), "--notes", str(notes), "--baseline-json", str(baseline),
                        "--runs-root", str(root / "runs"), "--min-first-check-seconds", "1",
                        "--first-check-buffer-seconds", "0", "--poll-seconds", "1", "--",
                        sys.executable, "-c", f"print('PASS fixture'); raise SystemExit({exit_code})"],
                        capture_output=True, text=True)
                    self.assertEqual(proc.returncode, exit_code, proc.stdout + proc.stderr)
                    self.assertEqual(before, {p: p.read_bytes() if p.exists() else None for p in before})
                    statuses = list((root / "runs").glob("*/status.json"))
                    self.assertEqual(len(statuses), 1)
                    status = json.loads(statuses[0].read_text())
                    self.assertEqual(status["state"], "completed" if exit_code == 0 else "failed")
                    self.assertEqual(status["exit_code"], exit_code)

if __name__ == "__main__":
    unittest.main()
