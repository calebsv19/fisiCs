import json
import shutil
import tempfile
from pathlib import Path

from inventory.registry import DIAG_JSON_PROBES, DIAG_PROBES, RUNTIME_PROBES

from .exec import run_binary, run_cmd
from .selection import parse_probe_filters, probe_selected
from .taxonomy import emit_probe_blocked_classification


PROBE_DIR = Path(__file__).resolve().parent.parent
REPO_ROOT = PROBE_DIR.parent.parent.parent
FISICS = REPO_ROOT / "fisics"
COMPILE_TIMEOUT_SEC = 20
RUN_TIMEOUT_SEC = 8


def run_runtime_probe(probe, clang_path):
    with tempfile.TemporaryDirectory(prefix=f"probe-{probe.probe_id}-") as tmp:
        tmp_dir = Path(tmp)
        fisics_exe = tmp_dir / "fisics.out"
        clang_exe = tmp_dir / "clang.out"
        sources = list(probe.inputs) if probe.inputs else [probe.source]
        mixed_clang_inputs = list(probe.mixed_clang_inputs) if probe.mixed_clang_inputs else []

        mixed_clang_objects = []
        for src in mixed_clang_inputs:
            obj_path = tmp_dir / f"{src.stem}.clang.o"
            clang_obj_exit, clang_obj_out, clang_obj_timeout = run_cmd(
                [clang_path or "clang", "-std=c99", "-O0", "-c", str(src), "-o", str(obj_path)],
                COMPILE_TIMEOUT_SEC,
            )
            if clang_obj_timeout:
                return (
                    "BLOCKED",
                    f"clang object compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                    clang_obj_out.strip(),
                )
            if clang_obj_exit != 0:
                return (
                    "BLOCKED",
                    f"clang object compile failed ({clang_obj_exit})",
                    clang_obj_out.strip(),
                )
            mixed_clang_objects.append(obj_path)

        fisics_compile_exit, fisics_compile_out, fisics_compile_timeout = run_cmd(
            [str(FISICS)] + [str(src) for src in sources] + [str(obj) for obj in mixed_clang_objects] + ["-o", str(fisics_exe)],
            COMPILE_TIMEOUT_SEC,
        )
        if fisics_compile_timeout:
            return (
                "BLOCKED",
                f"fisics compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                fisics_compile_out.strip(),
            )
        if fisics_compile_exit != 0:
            return (
                "BLOCKED",
                f"fisics compile failed ({fisics_compile_exit})",
                fisics_compile_out.strip(),
            )

        fisics_run_exit, fisics_stdout, fisics_stderr, fisics_run_timeout = run_binary(
            fisics_exe, RUN_TIMEOUT_SEC
        )
        if fisics_run_timeout:
            return (
                "BLOCKED",
                f"fisics runtime timeout ({RUN_TIMEOUT_SEC}s)",
                "",
            )

        if not clang_path:
            return (
                "SKIP",
                "clang not found; differential unavailable",
                f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
            )

        clang_compile_exit, clang_compile_out, clang_compile_timeout = run_cmd(
            [clang_path, "-std=c99", "-O0"] + [str(src) for src in sources + mixed_clang_inputs] + ["-o", str(clang_exe)],
            COMPILE_TIMEOUT_SEC,
        )
        if clang_compile_timeout:
            return (
                "BLOCKED",
                f"clang compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                clang_compile_out.strip(),
            )
        if clang_compile_exit != 0:
            return (
                "BLOCKED",
                f"clang compile failed ({clang_compile_exit})",
                clang_compile_out.strip(),
            )

        clang_run_exit, clang_stdout, clang_stderr, clang_run_timeout = run_binary(
            clang_exe, RUN_TIMEOUT_SEC
        )
        if clang_run_timeout:
            return (
                "BLOCKED",
                f"clang runtime timeout ({RUN_TIMEOUT_SEC}s)",
                "",
            )

        same = (
            fisics_run_exit == clang_run_exit
            and fisics_stdout == clang_stdout
            and fisics_stderr == clang_stderr
        )

        if same and probe.extra_differential_compiler:
            extra_compiler_name = probe.extra_differential_compiler
            extra_compiler_path = shutil.which(extra_compiler_name)
            if not extra_compiler_path:
                return (
                    "SKIP",
                    f"{extra_compiler_name} not found; extra differential unavailable",
                    f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
                )

            extra_exe = tmp_dir / f"{extra_compiler_name}.out"
            extra_compile_exit, extra_compile_out, extra_compile_timeout = run_cmd(
                [extra_compiler_path, "-std=c99", "-O0"]
                + [str(src) for src in sources + mixed_clang_inputs]
                + ["-o", str(extra_exe)],
                COMPILE_TIMEOUT_SEC,
            )
            if extra_compile_timeout:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                    extra_compile_out.strip(),
                )
            if extra_compile_exit != 0:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} compile failed ({extra_compile_exit})",
                    extra_compile_out.strip(),
                )

            extra_run_exit, extra_stdout, extra_stderr, extra_run_timeout = run_binary(
                extra_exe, RUN_TIMEOUT_SEC
            )
            if extra_run_timeout:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} runtime timeout ({RUN_TIMEOUT_SEC}s)",
                    "",
                )

            extra_same = (
                fisics_run_exit == extra_run_exit
                and fisics_stdout == extra_stdout
                and fisics_stderr == extra_stderr
            )
            if not extra_same:
                detail = (
                    f"fisics(exit={fisics_run_exit}, out={fisics_stdout.strip()}, err={fisics_stderr.strip()}) "
                    f"vs {extra_compiler_name}(exit={extra_run_exit}, out={extra_stdout.strip()}, err={extra_stderr.strip()})"
                )
                return (
                    "BLOCKED",
                    f"runtime mismatch vs {extra_compiler_name}",
                    detail,
                )

            return (
                "RESOLVED",
                f"matches clang+{extra_compiler_name} runtime behavior",
                f"stdout={fisics_stdout.strip()}",
            )

        if same:
            return (
                "RESOLVED",
                "matches clang runtime behavior",
                f"stdout={fisics_stdout.strip()}",
            )

        detail = (
            f"fisics(exit={fisics_run_exit}, out={fisics_stdout.strip()}, err={fisics_stderr.strip()}) "
            f"vs clang(exit={clang_run_exit}, out={clang_stdout.strip()}, err={clang_stderr.strip()})"
        )
        return ("BLOCKED", "runtime mismatch vs clang", detail)


def run_diag_probe(probe):
    sources = list(probe.inputs) if probe.inputs else [probe.source]
    with tempfile.TemporaryDirectory(prefix=f"probe-diag-{probe.probe_id}-") as tmp:
        cmd = [str(FISICS)] + [str(src) for src in sources]
        if len(sources) > 1:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diag.out")]
        _, out, _ = run_cmd(cmd, COMPILE_TIMEOUT_SEC)
    if probe.required_substrings:
        lowered = out.lower()
        for needle in probe.required_substrings:
            if needle.lower() in lowered:
                return ("RESOLVED", f"diagnostic now emitted ({needle})", "")
        return ("BLOCKED", "expected diagnostic substring missing", "")

    has_diag = "Error at (" in out or "Error:" in out or ": error:" in out
    if probe.expect_any_diagnostic:
        if has_diag:
            return ("RESOLVED", "diagnostic now emitted", "")
        return ("BLOCKED", "diagnostic missing", "")
    if has_diag:
        return ("BLOCKED", "unexpected diagnostic emitted", "")
    return ("RESOLVED", "no diagnostic emitted (expected for this lane)", "")


def run_diag_json_probe(probe):
    with tempfile.TemporaryDirectory(prefix=f"probe-diagjson-{probe.probe_id}-") as tmp:
        json_path = Path(tmp) / "diags.json"
        sources = list(probe.inputs) if probe.inputs else [probe.source]
        cmd = [str(FISICS), "--emit-diags-json", str(json_path)] + [str(src) for src in sources]
        if len(sources) > 1:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diagjson.out")]
        exit_code, out, timed_out = run_cmd(
            cmd,
            COMPILE_TIMEOUT_SEC,
        )
        if timed_out:
            return ("BLOCKED", f"compile timeout ({COMPILE_TIMEOUT_SEC}s)", "")
        if not json_path.exists():
            if exit_code != 0:
                return ("BLOCKED", f"compile exited {exit_code}", out.strip())
            return ("BLOCKED", "diagnostics JSON file missing", "")
        try:
            data = json.loads(json_path.read_text(encoding="utf-8"))
        except Exception as exc:
            return ("BLOCKED", f"diagnostics JSON unreadable ({exc})", "")
        diagnostics = data.get("diagnostics", [])
        if probe.require_any_diagnostic:
            if diagnostics:
                if probe.expected_codes:
                    actual_codes = [int(item.get("code", -1)) for item in diagnostics]
                    missing_codes = [code for code in probe.expected_codes if code not in actual_codes]
                    if missing_codes:
                        return ("BLOCKED", f"diagnostics JSON missing expected code(s): {missing_codes}", "")
                if probe.expected_line is not None:
                    actual_lines = [int(item.get("line", -1)) for item in diagnostics]
                    if probe.expected_line not in actual_lines:
                        return ("BLOCKED", f"diagnostics JSON missing expected line {probe.expected_line}", "")
                if probe.expected_column is not None:
                    actual_columns = [int(item.get("column", -1)) for item in diagnostics]
                    if probe.expected_column not in actual_columns:
                        return (
                            "BLOCKED",
                            f"diagnostics JSON missing expected column {probe.expected_column}",
                            "",
                        )
                if probe.expected_has_file is not None:
                    actual_has_file = [bool(item.get("has_file", False)) for item in diagnostics]
                    if probe.expected_has_file not in actual_has_file:
                        return (
                            "BLOCKED",
                            f"diagnostics JSON missing expected has_file={probe.expected_has_file}",
                            "",
                        )
                return ("RESOLVED", f"diagnostics JSON has {len(diagnostics)} item(s)", "")
            return ("BLOCKED", "diagnostics JSON unexpectedly empty", "")
        return ("RESOLVED", "diagnostics JSON exported", "")


def main():
    if not FISICS.exists():
        print(f"fisics binary not found at {FISICS}")
        return 1

    clang_path = shutil.which("clang")
    print("Probe Runner")
    print(f"fisics: {FISICS}")
    print(f"clang: {clang_path or 'not found'}")
    filters = parse_probe_filters()
    if filters:
        print(f"probe_filter: {', '.join(filters)}")
    else:
        print("probe_filter: <none>")
    print("")

    blocked = 0
    resolved = 0
    skipped = 0
    selected = 0

    print("[runtime probes]")
    for probe in RUNTIME_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        selected += 1
        status, summary, detail = run_runtime_probe(probe, clang_path)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if status == "BLOCKED":
            emit_probe_blocked_classification(probe, "runtime", summary)
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print("[diagnostic probes]")
    for probe in DIAG_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        selected += 1
        status, summary, detail = run_diag_probe(probe)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if status == "BLOCKED":
            emit_probe_blocked_classification(probe, "diagnostic", summary)
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print("[diagnostic-json probes]")
    for probe in DIAG_JSON_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        selected += 1
        status, summary, detail = run_diag_json_probe(probe)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if status == "BLOCKED":
            emit_probe_blocked_classification(probe, "diagnostic-json", summary)
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    if filters and selected == 0:
        print("")
        print(f"error: no probes selected (filter={', '.join(filters)})")
        return 1

    print("")
    print(f"Summary: blocked={blocked}, resolved={resolved}, skipped={skipped}")
    return 0
