#!/usr/bin/env python3
"""Check declared result boundaries through the real multi-TU driver."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check(compiler, out):
    manifest = json.loads((ROOT / "tests/final/meta/15-torture-differential-physics-units-returns.json").read_text())
    rows = []
    env = {**os.environ, "FISICS_MAX_PROCS": "0"}
    for case in manifest["tests"]:
        name = case["id"]
        executable = out / name
        diag = out / (name + ".json")
        command = [str(compiler), *case["args"], "--emit-diags-json", str(diag),
                   *(str(ROOT / "tests/final" / p) for p in case["inputs"]), "-o", str(executable)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == case["expect_compile_exit"], (name, result.returncode)
        expected = (4103 if "consumer" in name or "assignment" in name else
                    4113 if "definition" in name else 2000 if "contract_" in name else None)
        if expected is not None:
            assert not executable.exists(), name
            errors = [d for d in json.loads(diag.read_text())["diagnostics"] if d["severity_name"] == "error"]
            assert len(errors) == 1 and errors[0]["code_id"] == expected, (name, errors)
        else:
            run = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
            assert run.returncode == 0 and run.stdout == "6.0\n", (name, run)
            executable.unlink()
        rows.append({"id": name, "command": command, "exit": result.returncode, "expected_code": expected})
    # A genuinely unannotated external result remains unknown. Deliberately
    # accepting it is an unchecked control, never proof of physical validity.
    fixtures = ROOT / "tests/fixtures/solver_physics"
    for reverse in (False, True):
        sources = [fixtures / "interface.c", fixtures / "helper.c"]
        if reverse:
            sources.reverse()
        name = "unknown_reverse" if reverse else "unknown_forward"
        executable = out / name
        command = [str(compiler), "--overlay=physics-units", "-DOMIT_RESULT_CONTRACT", "-DBAD_RETURN_CONSUMER",
                   *(str(p) for p in sources), "-o", str(executable)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == 0 and executable.exists(), name
        executable.unlink()
        rows.append({"id": name, "command": command, "exit": 0, "classification": "unchecked_control"})
    (out / "results.json").write_text(json.dumps({"status": "PASS", "cases": len(rows), "results": rows}, indent=2) + "\n")
    print(f"Units return contracts: {len(rows)} PASS (including 2 unchecked controls)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=False)
        check(compiler, args.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="fisics-returns-") as scratch:
            check(compiler, Path(scratch))


if __name__ == "__main__":
    main()
