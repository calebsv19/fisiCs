#!/usr/bin/env python3
"""Square-root diagnostics, real runtime roots and explicit unknown controls."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check(compiler, out):
    manifest = json.loads((ROOT / "tests/final/meta/15-torture-differential-physics-units-sqrt.json").read_text())
    codes = [None, 4103, 4115, 4115, 4103, 4103, 4112, 4113, 4107, None, 4103]
    expected_stdout = (ROOT / "tests/final/expect/15__units_sqrt_valid.stdout").read_text()
    rows = []
    env = {**os.environ, "FISICS_MAX_PROCS": "0"}
    for case, code in zip(manifest["tests"], codes):
        name = case["id"]
        executable, diag = out / name, out / (name + ".json")
        command = [str(compiler), *case["args"], "--emit-diags-json", str(diag),
                   str(ROOT / "tests/final" / case["input"]), "-o", str(executable)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == int(code is not None), (name, result.returncode)
        if code is None:
            run = subprocess.run([str(executable)], text=True, capture_output=True, timeout=10)
            assert run.returncode == 0 and run.stdout == expected_stdout, (name, run)
            executable.unlink()
        else:
            assert not executable.exists(), name
            errors = [d for d in json.loads(diag.read_text())["diagnostics"] if d["severity_name"] == "error"]
            assert len(errors) == 1 and errors[0]["code_id"] == code, (name, errors)
            error = errors[0]
            assert error["line"] > 0 and error["column"] > 0 and error["category_name"] == "extension", error
            if code == 4115:
                details = error["details"]
                expected = [1, 0, 0, 0, 0, 0, 0, 0] if name.endswith("_odd") and "negative" not in name else [0, 0, -1, 0, 0, 0, 0, 0]
                assert details["context"] == "sqrt" and details["lhs_dim"] == expected, details
                assert "rhs_dim" not in details, details
            if name.endswith("_destination"):
                assert error["details"]["lhs_dim"] == [0, 0, 1, 0, 0, 0, 0, 0], error
                assert error["details"]["rhs_dim"] == [1, 0, -1, 0, 0, 0, 0, 0], error
        rows.append({"id": name, "command": command, "exit": result.returncode, "expected_code": code})
    controls = {
        "unknown_input": "#include <math.h>\nvoid f(double x){double t [[fisics::dim(time)]]=sqrt(x);}",
        "pointer_shadow": "#include <math.h>\ndouble custom(double x){return x;} void f(void){double (*sqrt)(double)=custom;double x [[fisics::dim(length)]]=4;double t [[fisics::dim(time)]]=sqrt(x);}",
        "custom_declaration": "double sqrt(double);void f(double x [[fisics::dim(length)]]){double t [[fisics::dim(time)]]=sqrt(x);}",
    }
    for name, source in controls.items():
        src, obj = out / (name + ".c"), out / (name + ".o")
        src.write_text(source + "\n")
        command = [str(compiler), "--overlay=physics-units", "-c", str(src), "-o", str(obj)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == 0 and obj.exists(), name
        obj.unlink()
        rows.append({"id": name, "command": command, "exit": 0, "classification": "unchecked_control"})
    # Model both angle and quoted nested declarations in a system math header.
    for quoted in (False, True):
        name = "quoted_nested_header" if quoted else "nested_system_header"
        headers = out / name
        headers.mkdir()
        (headers / "sqrt_decl.h").write_text("double sqrt(double);\n")
        operand = '\"sqrt_decl.h\"' if quoted else "<sqrt_decl.h>"
        (headers / "math.h").write_text("#include " + operand + "\n")
        src, obj, diag = headers / "caller.c", headers / "caller.o", headers / "diag.json"
        src.write_text("#include <math.h>\nvoid f(double x [[fisics::dim(area)]]){double t [[fisics::dim(time)]]=sqrt(x); }\n")
        command = [str(compiler), "--overlay=physics-units", "-I", str(headers), "--emit-diags-json", str(diag), "-c", str(src), "-o", str(obj)]
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        assert result.returncode == 1, (name, result.returncode, result.stdout)
        assert not obj.exists(), name
        errors = [d for d in json.loads(diag.read_text())["diagnostics"] if d["severity_name"] == "error"]
        assert len(errors) == 1 and errors[0]["code_id"] == 4103, errors
        rows.append({"id": name, "command": command, "exit": result.returncode,
                     "classification": "rejected_as_required"})
    (out / "results.json").write_text(json.dumps({"status": "PASS", "cases": len(rows), "results": rows}, indent=2) + "\n")
    print(f"Units sqrt contracts: {len(rows)} PASS (including 3 unchecked controls)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=False)
        check(args.compiler.resolve(), args.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="fisics-sqrt-") as tmp:
            check(args.compiler.resolve(), Path(tmp))


if __name__ == "__main__":
    main()
