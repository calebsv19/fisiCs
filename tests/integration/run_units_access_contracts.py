#!/usr/bin/env python3
"""Exact physical access diagnostics and the unannotated-state boundary."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check(compiler, out):
    source = ROOT / "tests/final/cases/15__units_access_dimensions.c"
    expected_codes = [None, 4103, 4103, 4112, 4113, 4101, 4107, 4107]
    rows = []
    env = {**os.environ, "FISICS_MAX_PROCS": "0"}
    for variant, code in enumerate(expected_codes):
        name = f"access_{variant}"
        executable, diag = out / name, out / (name + ".json")
        command = [str(compiler), "--overlay=physics-units", f"-DFAIL_KIND={variant}",
                   "--emit-diags-json", str(diag), str(source), "-o", str(executable)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == int(code is not None), (name, result.returncode)
        if code is None:
            run = subprocess.run([str(executable)], text=True, capture_output=True, timeout=10)
            assert run.returncode == 0 and run.stdout == "2.500000 1.250000\n", run
            executable.unlink()
        else:
            assert not executable.exists(), name
            errors = [d for d in json.loads(diag.read_text())["diagnostics"] if d["severity_name"] == "error"]
            assert len(errors) == 1 and errors[0]["code_id"] == code, (name, errors)
            error = errors[0]
            assert error["line"] > 0 and error["column"] > 0 and error["category_name"] == "extension", error
            if 1 <= variant <= 5:
                details = error["details"]
                acceleration, time = [1, 0, -2, 0, 0, 0, 0, 0], [0, 0, 1, 0, 0, 0, 0, 0]
                expected, actual = (acceleration, time) if variant in (1, 5) else (time, acceleration)
                assert details["lhs_dim"] == expected and details["rhs_dim"] == actual, details
                context = "argument" if variant == 3 else "return" if variant == 4 else "addition" if variant == 5 else "assignment"
                assert details["context"] == context, details
        rows.append({"id": name, "command": command, "exit": result.returncode, "expected_code": code})
    # Same stored-field mutation, with no physical annotation on that storage:
    # acceptance here is an explicit checking gap, never compatibility proof.
    source = ROOT / "tests/fixtures/solver_physics/translation.c"
    executable = out / "unannotated_state"
    command = [str(compiler), "--overlay=physics-units", "-DBAD_RAW_FIELD", str(source), "-o", str(executable)]
    result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
    (out / "unannotated_state.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0 and executable.exists(), result
    executable.unlink()
    rows.append({"id": "unannotated_state", "command": command, "exit": 0, "classification": "unchecked_control"})
    (out / "results.json").write_text(json.dumps({"status": "PASS", "cases": len(rows), "results": rows}, indent=2) + "\n")
    print(f"Units access contracts: {len(rows)} PASS (including 1 unchecked control)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=False)
        check(args.compiler.resolve(), args.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="fisics-access-") as tmp:
            check(args.compiler.resolve(), Path(tmp))


if __name__ == "__main__":
    main()
