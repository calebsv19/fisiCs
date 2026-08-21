import json
import hashlib
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

from inventory.registry import DIAG_JSON_PROBES, DIAG_PROBES, OBJECT_PROBES, RUNTIME_PROBES

FINAL_ROOT = Path(__file__).resolve().parents[2]
if str(FINAL_ROOT) not in sys.path:
    sys.path.insert(0, str(FINAL_ROOT))

from bin_resolver import stage_bin_copy

from .exec import run_binary, run_cmd
from .models import DiagnosticExpectation
from .selection import parse_probe_filters, probe_selected
from .taxonomy import emit_probe_blocked_classification


PROBE_DIR = Path(__file__).resolve().parent.parent
REPO_ROOT = PROBE_DIR.parent.parent.parent
OS_POLICY_ROOT = REPO_ROOT / "tests" / "os_policy"
if str(OS_POLICY_ROOT) not in sys.path:
    sys.path.insert(0, str(OS_POLICY_ROOT))

from verify_object import verify_object

COMPILE_TIMEOUT_SEC = 20
RUN_TIMEOUT_SEC = 8

_STABLE_ORACLE_INDEX = None
_EXTRA_COMPILER_PATHS = {}


def stable_oracle_index():
    global _STABLE_ORACLE_INDEX
    if _STABLE_ORACLE_INDEX is not None:
        return _STABLE_ORACLE_INDEX

    index_path = FINAL_ROOT / "meta" / "index.json"
    mapping = {}
    if not index_path.is_file():
        _STABLE_ORACLE_INDEX = mapping
        return mapping
    manifest_index = json.loads(index_path.read_text(encoding="utf-8"))
    for manifest_name in manifest_index.get("manifests", []):
        manifest_path = FINAL_ROOT / "meta" / manifest_name
        if not manifest_path.is_file():
            continue
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        for test in manifest.get("tests", []):
            inputs = [test.get("input", ""), *(test.get("inputs") or [])]
            for expectation_path in test.get("expects", []):
                suffix = Path(expectation_path).suffix
                if suffix not in (".diag", ".diagjson", ".pdiag"):
                    continue
                mapping.setdefault(("__id__", test.get("id", ""), suffix), set()).add(
                    expectation_path
                )
                for input_path in inputs:
                    if input_path:
                        mapping.setdefault((input_path, suffix), set()).add(expectation_path)
    _STABLE_ORACLE_INDEX = mapping
    return mapping


def stable_oracle_path_for_source(source, suffix):
    try:
        relative_source = str(source.resolve().relative_to(FINAL_ROOT.resolve()))
    except ValueError:
        return None
    candidates = stable_oracle_index().get((relative_source, suffix), set())
    if len(candidates) != 1:
        return None
    path = FINAL_ROOT / next(iter(candidates))
    return path if path.is_file() else None


def stable_probe_id_variants(probe_id, family):
    variants = {probe_id}
    if "__probe_" not in probe_id:
        return variants
    bucket, tail = probe_id.split("__probe_", 1)
    variants.add(probe_id.replace("__probe_", "__", 1))
    if family == "diagnostic":
        variants.add(f"{bucket}__diag__{tail}")
    elif family == "diagnostic-json":
        variants.add(f"{bucket}__diagjson__{tail}")
        if tail.startswith("diagjson_"):
            semantic_tail = tail[len("diagjson_"):]
            variants.add(f"{bucket}__diagjson__{semantic_tail}")
            variants.add(f"{bucket}__line_directive_{semantic_tail}")
    return variants


def stable_oracle_path_for_probe(probe_id, family, source, suffix):
    # A unique source owner is stronger than heuristic probe-id variants. Some
    # older case and probe-promotion lanes intentionally share normalized ID
    # tails while retaining different source locations and columns.
    source_path = stable_oracle_path_for_source(source, suffix)
    if source_path is not None:
        return source_path
    index = stable_oracle_index()
    id_candidates = set()
    for variant in stable_probe_id_variants(probe_id, family):
        id_candidates.update(index.get(("__id__", variant, suffix), set()))
    if len(id_candidates) == 1:
        path = FINAL_ROOT / next(iter(id_candidates))
        if path.is_file():
            return path
    return None


def stable_text_identity_markers(probe_id, source):
    path = stable_oracle_path_for_probe(probe_id, "diagnostic", source, ".diag")
    if not path:
        return ()
    markers = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith(("Error at (", "Warning at (")) and "): " in line:
            marker = line.split("): ", 1)[1].strip()
        elif line.startswith(("Error: ", "Warning: ")):
            marker = line.split(": ", 1)[1].strip()
            # Stable text expectations use checkout-relative source paths, while
            # probe inventory paths are absolute. Keep the diagnostic identity
            # but discard that non-semantic path suffix.
            if " at tests/final/" in marker:
                marker = marker.split(" at tests/final/", 1)[0].rstrip()
        else:
            continue
        if marker and marker not in markers:
            markers.append(marker)
    if markers:
        return tuple(markers)
    parser_path = stable_oracle_path_for_probe(
        probe_id, "diagnostic", source, ".pdiag"
    )
    if parser_path:
        for raw_line in parser_path.read_text(encoding="utf-8").splitlines():
            fields = dict(
                field.split("=", 1)
                for field in raw_line.split()
                if "=" in field
            )
            line = fields.get("line")
            if line:
                # Parser-only stable oracles expose code/location rather than
                # the human message. Match the path-independent source line in
                # the legacy text stream.
                marker = f"@diagnostic-line:{line}"
                if marker not in markers:
                    markers.append(marker)
    return tuple(markers)


def stable_json_identity_expectations(probe_id, source):
    path = stable_oracle_path_for_probe(
        probe_id, "diagnostic-json", source, ".diagjson"
    )
    if not path:
        return ()
    payload = json.loads(path.read_text(encoding="utf-8"))
    expectations = []
    for item in payload.get("diagnostics", []):
        expectations.append(
            DiagnosticExpectation(
                code=int(item["code"]) if "code" in item else None,
                line=int(item["line"]) if "line" in item else None,
                column=int(item["column"]) if "column" in item else None,
                has_file=bool(item.get("has_file", False)),
                file=str(item["file"]) if "file" in item else None,
                severity=str(item["severity_name"]) if "severity_name" in item else None,
                stage=str(item["stage"]) if "stage" in item else None,
                message_substrings=(str(item["message"]),) if "message" in item else (),
                macro_trace=tuple(item["macro_trace"]) if "macro_trace" in item else None,
            )
        )
    return tuple(expectations)


def compile_env(overrides):
    env = os.environ.copy()
    env.setdefault("FISICS_MAX_PROCS", "0")
    if overrides:
        for key, value in overrides.items():
            env[str(key)] = str(value)
    return env


def compile_output_substrings(out, required_substrings=None, forbidden_substrings=None):
    lowered = out.lower()
    if required_substrings:
        for needle in required_substrings:
            if needle.startswith("@diagnostic-line:"):
                line = needle.split(":", 1)[1]
                if not any(
                    candidate.lower() in lowered
                    for candidate in (f":{line} (", f"at line {line}", f"({line}:")
                ):
                    return False, f"expected diagnostic source line missing ({line})"
                continue
            if needle.lower() not in lowered:
                return False, f"expected output substring missing ({needle})"
    if forbidden_substrings:
        for needle in forbidden_substrings:
            if needle.lower() in lowered:
                return False, f"unexpected output substring present ({needle})"
    return True, ""


def missing_probe_inputs(sources):
    return [source for source in sources if not source.is_file()]


def llvm_inspection_tools():
    llvm_config = shutil.which("llvm-config")
    if not llvm_config:
        return None, None, "llvm-config not found"
    exit_code, output, timed_out = run_cmd(
        [llvm_config, "--bindir"], COMPILE_TIMEOUT_SEC
    )
    if timed_out:
        return None, None, "llvm-config --bindir timeout"
    if exit_code != 0:
        return None, None, f"llvm-config --bindir failed ({exit_code})"
    bindir = Path(output.strip())
    readobj = bindir / "llvm-readobj"
    objdump = bindir / "llvm-objdump"
    if not readobj.is_file() or not objdump.is_file():
        return None, None, f"LLVM inspection tools missing under {bindir}"
    return readobj, objdump, ""


def object_contract(probe, *, clang_reference=False):
    allowed_undefined = probe.allowed_undefined
    allowed_relocations = probe.allowed_relocations
    if clang_reference and probe.clang_allowed_undefined is not None:
        allowed_undefined = probe.clang_allowed_undefined
    if clang_reference and probe.clang_allowed_relocations is not None:
        allowed_relocations = probe.clang_allowed_relocations
    return {
        "required_exports": list(probe.required_exports),
        "allowed_undefined": list(allowed_undefined),
        "allowed_relocations": list(allowed_relocations),
        "forbidden_instructions": list(probe.forbidden_instructions),
        "forbid_red_zone": probe.forbid_red_zone,
    }


def verify_object_section_policy(probe, object_path, readobj):
    exit_code, output, timed_out = run_cmd(
        [str(readobj), "--sections", str(object_path)], COMPILE_TIMEOUT_SEC
    )
    if timed_out:
        raise RuntimeError("llvm-readobj --sections timeout")
    if exit_code != 0:
        raise RuntimeError(f"llvm-readobj --sections failed ({exit_code})")

    sections = {}
    for block in re.findall(r"  Section \{\n(.*?)\n  \}", output, re.DOTALL):
        name_match = re.search(r"^    Name: ([^\s(]+)", block, re.MULTILINE)
        if not name_match:
            continue
        flags = set(re.findall(r"\bSHF_[A-Z_]+\b", block))
        section_name = name_match.group(1)
        sections[section_name] = flags
        if "SHF_WRITE" in flags and "SHF_EXECINSTR" in flags:
            raise RuntimeError(
                f"writable executable section rejected: {section_name}"
            )

    for section_name, required_flags in (probe.required_section_flags or {}).items():
        actual_flags = sections.get(section_name)
        if actual_flags is None:
            raise RuntimeError(f"required section missing: {section_name}")
        missing_flags = sorted(set(required_flags) - actual_flags)
        if missing_flags:
            raise RuntimeError(
                f"section {section_name} missing required flags: {missing_flags}"
            )
    for section_prefix, required_flags in (
        probe.required_section_prefix_flags or {}
    ).items():
        matches = [
            (section_name, flags)
            for section_name, flags in sections.items()
            if section_name.startswith(section_prefix)
        ]
        if not matches:
            raise RuntimeError(f"required section prefix missing: {section_prefix}")
        if not any(set(required_flags).issubset(flags) for _, flags in matches):
            observed = {
                section_name: sorted(flags) for section_name, flags in matches
            }
            raise RuntimeError(
                f"section prefix {section_prefix} missing required flags: "
                f"required={sorted(required_flags)} observed={observed}"
            )
    return sections


def run_object_probe(probe, clang_path, fisics_bin, readobj, objdump):
    if not probe.source.is_file():
        return ("BLOCKED", "probe input missing", str(probe.source))
    if probe.expected_source_sha256 is not None:
        actual_source_sha256 = hashlib.sha256(probe.source.read_bytes()).hexdigest()
        if actual_source_sha256 != probe.expected_source_sha256:
            return (
                "BLOCKED",
                "probe source receipt mismatch",
                f"expected={probe.expected_source_sha256} actual={actual_source_sha256}",
            )
    with tempfile.TemporaryDirectory(prefix=f"probe-object-{probe.probe_id}-") as tmp:
        tmp_dir = Path(tmp)
        fisics_a = tmp_dir / "fisics-a.o"
        fisics_b = tmp_dir / "fisics-b.o"
        clang_a = tmp_dir / "clang-a.o"
        clang_b = tmp_dir / "clang-b.o"
        fisics_base = [
            str(fisics_bin),
            "--target",
            "x86_64-unknown-none",
            *(str(arg) for arg in (probe.fisics_args or [])),
            "-c",
            str(probe.source),
        ]
        fisics_env = compile_env(None)
        for output in (fisics_a, fisics_b):
            exit_code, compile_output, timed_out = run_cmd(
                [*fisics_base, "-o", str(output)],
                COMPILE_TIMEOUT_SEC,
                env=fisics_env,
            )
            if timed_out:
                return (
                    "BLOCKED",
                    f"fisics compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                    compile_output.strip(),
                )
            if exit_code != 0:
                return (
                    "BLOCKED",
                    f"fisics compile failed ({exit_code})",
                    compile_output.strip(),
                )
        if fisics_a.read_bytes() != fisics_b.read_bytes():
            return (
                "BLOCKED",
                "fisics object mismatch across identical replay",
                "",
            )
        if probe.expected_fisics_object_sha256 is not None:
            actual_object_sha256 = hashlib.sha256(fisics_a.read_bytes()).hexdigest()
            if actual_object_sha256 != probe.expected_fisics_object_sha256:
                return (
                    "BLOCKED",
                    "fisiCs object receipt mismatch",
                    f"expected={probe.expected_fisics_object_sha256} actual={actual_object_sha256}",
                )
        try:
            fisics_facts = verify_object(
                object_path=fisics_a,
                readobj=readobj,
                objdump=objdump,
                contract=object_contract(probe),
            )
            verify_object_section_policy(probe, fisics_a, readobj)
        except Exception as exc:
            if (
                probe.expected_policy_rejection is not None
                and probe.expected_policy_rejection in str(exc)
            ):
                return (
                    "RESOLVED",
                    "fisiCs object reaches the expected P4 policy rejection",
                    f"policy_rejection={exc}",
                )
            return ("BLOCKED", "fisics object mismatch", str(exc))

        if probe.expected_policy_rejection is not None:
            return (
                "BLOCKED",
                "fisiCs object unexpectedly passed P4 policy",
                f"expected_policy_rejection={probe.expected_policy_rejection}",
            )

        if not clang_path:
            return (
                "SKIP",
                "clang not found; object reference unavailable",
                f"fisics object bytes={len(fisics_a.read_bytes())}",
            )
        clang_base = [
            clang_path,
            "--target=x86_64-unknown-none",
            "-std=c99",
            "-O0",
            "-ffreestanding",
            "-fno-builtin",
            "-fno-stack-protector",
            "-fno-pic",
            "-fno-pie",
            "-fno-asynchronous-unwind-tables",
            "-mno-red-zone",
            "-mno-mmx",
            "-msse2" if probe.scalar_sse2 else "-mno-sse",
            *(str(arg) for arg in (probe.clang_args or [])),
            "-c",
            str(probe.source),
        ]
        for output in (clang_a, clang_b):
            exit_code, compile_output, timed_out = run_cmd(
                [*clang_base, "-o", str(output)], COMPILE_TIMEOUT_SEC
            )
            if timed_out:
                return (
                    "BLOCKED",
                    f"clang compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                    compile_output.strip(),
                )
            if exit_code != 0:
                return (
                    "BLOCKED",
                    f"clang compile failed ({exit_code})",
                    compile_output.strip(),
                )
        if clang_a.read_bytes() != clang_b.read_bytes():
            return (
                "BLOCKED",
                "clang object mismatch across identical replay",
                "",
            )
        try:
            clang_facts = verify_object(
                object_path=clang_a,
                readobj=readobj,
                objdump=objdump,
                contract=object_contract(probe, clang_reference=True),
            )
            verify_object_section_policy(probe, clang_a, readobj)
        except Exception as exc:
            return ("BLOCKED", "clang object mismatch", str(exc))

        detail = (
            f"fisics_bytes={len(fisics_a.read_bytes())} "
            f"clang_bytes={len(clang_a.read_bytes())} "
            f"exports={','.join(fisics_facts['required_exports'])} "
            f"undefined={','.join(fisics_facts['undefined_global_symbols']) or '<none>'} "
            f"relocations={','.join(fisics_facts['relocations']) or '<none>'}"
        )
        if (
            fisics_facts["undefined_global_symbols"]
            != clang_facts["undefined_global_symbols"]
        ):
            detail += (
                " reference_undefined="
                + (",".join(clang_facts["undefined_global_symbols"]) or "<none>")
            )
        return (
            "RESOLVED",
            "deterministic fisiCs/Clang objects satisfy contract",
            detail,
        )


def compile_exit_allowed(exit_code, allowed_exit_codes):
    return exit_code in tuple(allowed_exit_codes or ())


def diagnostic_record_matches(item, expectation):
    if expectation.code is not None and int(item.get("code", -1)) != expectation.code:
        return False
    if expectation.line is not None and int(item.get("line", -1)) != expectation.line:
        return False
    if expectation.column is not None and int(item.get("column", -1)) != expectation.column:
        return False
    if expectation.has_file is not None and bool(item.get("has_file", False)) != expectation.has_file:
        return False
    if expectation.file is not None:
        actual_file = str(item.get("file", ""))
        if actual_file != expectation.file and not actual_file.endswith(expectation.file):
            return False
    if expectation.severity is not None:
        actual_severity = str(item.get("severity_name", item.get("severity", "")))
        if actual_severity != expectation.severity:
            return False
    if expectation.stage is not None and str(item.get("stage", "")) != expectation.stage:
        return False
    message = str(item.get("message", ""))
    if not all(marker in message for marker in expectation.message_substrings):
        return False
    if expectation.macro_trace is not None:
        actual_trace = item.get("macro_trace")
        if not isinstance(actual_trace, list) or len(actual_trace) != len(expectation.macro_trace):
            return False
        for actual_frame, expected_frame in zip(actual_trace, expectation.macro_trace):
            if not isinstance(actual_frame, dict):
                return False
            for key, expected_value in expected_frame.items():
                actual_value = actual_frame.get(key)
                if key == "file":
                    actual_file = str(actual_value or "")
                    expected_file = str(expected_value)
                    if actual_file != expected_file and not actual_file.endswith(expected_file):
                        return False
                elif actual_value != expected_value:
                    return False
    return True


def match_expected_diagnostics(diagnostics, expectations):
    unmatched = set(range(len(diagnostics)))
    for expectation in expectations:
        match_index = next(
            (
                index
                for index in sorted(unmatched)
                if diagnostic_record_matches(diagnostics[index], expectation)
            ),
            None,
        )
        if match_index is None:
            return False
        unmatched.remove(match_index)
    return True


def mixed_object_path(directory, source, ordinal):
    """Return a per-input object path even when source basenames collide."""
    return Path(directory) / f"{ordinal:03d}_{Path(source).stem}.clang.o"


def runtime_stdout_oracle_mismatch(probe, actual_stdout):
    if probe.expected_stdout is not None:
        if probe.expected_stdout_variants is not None:
            return "invalid probe contract: exact and variant stdout are both set"
        if actual_stdout != probe.expected_stdout:
            return (
                f"stdout expected={probe.expected_stdout.strip()} "
                f"actual={actual_stdout.strip()}"
            )
        return None
    if (
        probe.expected_stdout_variants is not None
        and actual_stdout not in tuple(probe.expected_stdout_variants)
    ):
        expected = " | ".join(
            value.strip() for value in probe.expected_stdout_variants
        )
        return f"stdout expected one of=[{expected}] actual={actual_stdout.strip()}"
    return None


def differential_executable_name(compiler_name):
    return f"{Path(compiler_name).name}.out"


def versioned_gcc_candidates(bin_dir):
    """Return GCC driver binaries without selecting gcc-ar/nm/ranlib helpers."""
    return sorted(
        candidate
        for candidate in Path(bin_dir).glob("gcc-*")
        if candidate.is_file() and re.fullmatch(r"gcc-[0-9]+", candidate.name)
    )


def resolve_extra_differential_compiler(compiler_name):
    cached = _EXTRA_COMPILER_PATHS.get(compiler_name)
    if cached is not None:
        return cached

    resolved = shutil.which(compiler_name)
    if compiler_name != "gcc" or not resolved:
        _EXTRA_COMPILER_PATHS[compiler_name] = resolved
        return resolved

    configured = os.environ.get("FISICS_GNU_GCC", "").strip()
    if configured and Path(configured).is_file():
        _EXTRA_COMPILER_PATHS[compiler_name] = configured
        return configured

    exit_code, version, timed_out = run_cmd([resolved, "--version"], COMPILE_TIMEOUT_SEC)
    if not timed_out and exit_code == 0 and "Apple clang" in version:
        candidates = versioned_gcc_candidates("/opt/homebrew/opt/gcc/bin")
        if candidates:
            resolved = str(candidates[-1])

    _EXTRA_COMPILER_PATHS[compiler_name] = resolved
    return resolved


def run_runtime_probe(probe, clang_path, fisics_bin):
    with tempfile.TemporaryDirectory(prefix=f"probe-{probe.probe_id}-") as tmp:
        tmp_dir = Path(tmp)
        fisics_exe = tmp_dir / "fisics.out"
        clang_exe = tmp_dir / "clang.out"
        sources = list(probe.inputs) if probe.inputs else [probe.source]
        mixed_clang_inputs = list(probe.mixed_clang_inputs) if probe.mixed_clang_inputs else []
        missing_inputs = missing_probe_inputs(sources + mixed_clang_inputs)
        if missing_inputs:
            return (
                "BLOCKED",
                "probe input missing",
                ", ".join(str(path) for path in missing_inputs),
            )
        fisics_cmd = [str(fisics_bin)] + [str(arg) for arg in (probe.fisics_args or [])]
        fisics_env = compile_env(probe.fisics_env)
        clang_cmd = [clang_path or "clang", "-std=c99", "-O0"] + [str(arg) for arg in (probe.clang_args or [])]
        clang_env = compile_env(probe.clang_env)

        mixed_clang_objects = []
        for mixed_index, src in enumerate(mixed_clang_inputs):
            obj_path = mixed_object_path(tmp_dir, src, mixed_index)
            clang_obj_exit, clang_obj_out, clang_obj_timeout = run_cmd(
                clang_cmd + ["-c", str(src), "-o", str(obj_path)],
                COMPILE_TIMEOUT_SEC,
                env=clang_env,
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
            fisics_cmd + [str(src) for src in sources] + [str(obj) for obj in mixed_clang_objects] + ["-o", str(fisics_exe)],
            COMPILE_TIMEOUT_SEC,
            env=fisics_env,
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

        oracle_mismatches = []
        if (
            probe.expected_exit_code is not None
            and fisics_run_exit != probe.expected_exit_code
        ):
            oracle_mismatches.append(
                f"exit expected={probe.expected_exit_code} actual={fisics_run_exit}"
            )
        fisics_stdout_mismatch = runtime_stdout_oracle_mismatch(
            probe, fisics_stdout
        )
        if fisics_stdout_mismatch:
            oracle_mismatches.append(fisics_stdout_mismatch)
        if (
            probe.expected_stderr is not None
            and fisics_stderr != probe.expected_stderr
        ):
            oracle_mismatches.append(
                f"stderr expected={probe.expected_stderr.strip()} "
                f"actual={fisics_stderr.strip()}"
            )
        if oracle_mismatches:
            return (
                "BLOCKED",
                "runtime oracle mismatch for fisics",
                "; ".join(oracle_mismatches),
            )

        if not clang_path:
            return (
                "SKIP",
                "clang not found; differential unavailable",
                f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
            )

        clang_compile_exit, clang_compile_out, clang_compile_timeout = run_cmd(
            [clang_path, "-std=c99", "-O0"] + [str(arg) for arg in (probe.clang_args or [])] + [str(src) for src in sources + mixed_clang_inputs] + ["-o", str(clang_exe)],
            COMPILE_TIMEOUT_SEC,
            env=clang_env,
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

        clang_oracle_mismatches = []
        if (
            probe.expected_exit_code is not None
            and clang_run_exit != probe.expected_exit_code
        ):
            clang_oracle_mismatches.append(
                f"exit expected={probe.expected_exit_code} actual={clang_run_exit}"
            )
        clang_stdout_mismatch = runtime_stdout_oracle_mismatch(
            probe, clang_stdout
        )
        if clang_stdout_mismatch:
            clang_oracle_mismatches.append(clang_stdout_mismatch)
        if (
            probe.expected_stderr is not None
            and clang_stderr != probe.expected_stderr
        ):
            clang_oracle_mismatches.append(
                f"stderr expected={probe.expected_stderr.strip()} "
                f"actual={clang_stderr.strip()}"
            )
        if clang_oracle_mismatches:
            return (
                "BLOCKED",
                "runtime oracle mismatch for clang",
                "; ".join(clang_oracle_mismatches),
            )

        same = (
            fisics_run_exit == clang_run_exit
            and fisics_stdout == clang_stdout
            and fisics_stderr == clang_stderr
        )

        if same and probe.extra_differential_compiler:
            extra_compiler_name = probe.extra_differential_compiler
            extra_compiler_path = resolve_extra_differential_compiler(extra_compiler_name)
            if not extra_compiler_path:
                return (
                    "SKIP",
                    f"{extra_compiler_name} not found; extra differential unavailable",
                    f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
                )

            extra_exe = tmp_dir / differential_executable_name(extra_compiler_name)
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


def run_diag_probe(probe, fisics_bin):
    sources = list(probe.inputs) if probe.inputs else [probe.source]
    missing_inputs = missing_probe_inputs(sources)
    if missing_inputs:
        return (
            "BLOCKED",
            "probe input missing",
            ", ".join(str(path) for path in missing_inputs),
        )
    with tempfile.TemporaryDirectory(prefix=f"probe-diag-{probe.probe_id}-") as tmp:
        cmd = [str(fisics_bin)] + [str(arg) for arg in (probe.fisics_args or [])] + [str(src) for src in sources]
        env = compile_env(probe.fisics_env)
        disable_codegen = str(env.get("DISABLE_CODEGEN", "")).strip() not in ("", "0")
        if len(sources) > 1 and not disable_codegen:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diag.out")]
        exit_code, out, timed_out = run_cmd(cmd, COMPILE_TIMEOUT_SEC, env=env)
    if timed_out:
        return ("BLOCKED", f"compile timeout ({COMPILE_TIMEOUT_SEC}s)", out.strip())
    if not compile_exit_allowed(exit_code, probe.allowed_exit_codes):
        return ("BLOCKED", f"unexpected compile exit {exit_code}", out.strip())
    required_substrings = probe.required_substrings
    if probe.expect_any_diagnostic and not required_substrings:
        required_substrings = stable_text_identity_markers(probe.probe_id, probe.source)
    substring_ok, substring_detail = compile_output_substrings(
        out,
        required_substrings=required_substrings,
        forbidden_substrings=probe.forbidden_substrings,
    )
    if not substring_ok:
        return ("BLOCKED", substring_detail, "")

    has_diag = (
        "Error at (" in out or
        "Warning at (" in out or
        "Error:" in out or
        "Warning:" in out or
        ": error:" in out or
        ": warning:" in out
    )
    if probe.expect_any_diagnostic:
        if not required_substrings:
            return ("BLOCKED", "positive diagnostic probe lacks identity markers", "")
        if has_diag:
            if required_substrings:
                return ("RESOLVED", "diagnostic now emitted with required output markers", "")
            return ("RESOLVED", "diagnostic now emitted", "")
        return ("BLOCKED", "diagnostic missing", "")
    if exit_code != 0:
        return ("BLOCKED", f"unexpected compile exit {exit_code}", out.strip())
    if has_diag:
        return ("BLOCKED", "unexpected diagnostic emitted", "")
    if probe.required_substrings:
        return ("RESOLVED", "required output markers present without diagnostics", "")
    return ("RESOLVED", "no diagnostic emitted (expected for this lane)", "")


def run_diag_json_probe(probe, fisics_bin):
    with tempfile.TemporaryDirectory(prefix=f"probe-diagjson-{probe.probe_id}-") as tmp:
        json_path = Path(tmp) / "diags.json"
        sources = list(probe.inputs) if probe.inputs else [probe.source]
        missing_inputs = missing_probe_inputs(sources)
        if missing_inputs:
            return (
                "BLOCKED",
                "probe input missing",
                ", ".join(str(path) for path in missing_inputs),
            )
        cmd = [str(fisics_bin)] + [str(arg) for arg in (probe.fisics_args or [])] + ["--emit-diags-json", str(json_path)] + [str(src) for src in sources]
        env = compile_env(probe.fisics_env)
        disable_codegen = str(env.get("DISABLE_CODEGEN", "")).strip() not in ("", "0")
        if len(sources) > 1 and not disable_codegen:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diagjson.out")]
        exit_code, out, timed_out = run_cmd(
            cmd,
            COMPILE_TIMEOUT_SEC,
            env=env,
        )
        if timed_out:
            return ("BLOCKED", f"compile timeout ({COMPILE_TIMEOUT_SEC}s)", "")
        if not compile_exit_allowed(exit_code, probe.allowed_exit_codes):
            return ("BLOCKED", f"unexpected compile exit {exit_code}", out.strip())
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
                expected_diagnostics = probe.expected_diagnostics
                if not expected_diagnostics and not probe.expected_codes:
                    expected_diagnostics = stable_json_identity_expectations(
                        probe.probe_id, probe.source
                    )
                if expected_diagnostics:
                    if not match_expected_diagnostics(diagnostics, expected_diagnostics):
                        return ("BLOCKED", "diagnostics JSON missing atomic expected record(s)", "")
                elif probe.expected_codes:
                    actual_codes = [int(item.get("code", -1)) for item in diagnostics]
                    remaining_codes = list(actual_codes)
                    for code in probe.expected_codes:
                        if code not in remaining_codes:
                            return ("BLOCKED", f"diagnostics JSON missing expected code {code}", "")
                        remaining_codes.remove(code)
                    matching_records = [
                        item for item in diagnostics if int(item.get("code", -1)) in probe.expected_codes
                    ]
                    if probe.expected_line is not None:
                        matching_records = [
                            item for item in matching_records
                            if int(item.get("line", -1)) == probe.expected_line
                        ]
                    if probe.expected_column is not None:
                        matching_records = [
                            item for item in matching_records
                            if int(item.get("column", -1)) == probe.expected_column
                        ]
                    if probe.expected_has_file is not None:
                        matching_records = [
                            item for item in matching_records
                            if bool(item.get("has_file", False)) == probe.expected_has_file
                        ]
                    if any(
                        value is not None
                        for value in (
                            probe.expected_line,
                            probe.expected_column,
                            probe.expected_has_file,
                        )
                    ) and not matching_records:
                        return ("BLOCKED", "diagnostics JSON fields do not match one expected-code record", "")
                else:
                    return ("BLOCKED", "positive diagnostics JSON probe lacks identity expectations", "")
                return ("RESOLVED", f"diagnostics JSON has {len(diagnostics)} item(s)", "")
            return ("BLOCKED", "diagnostics JSON unexpectedly empty", "")
        if exit_code != 0:
            return ("BLOCKED", f"unexpected compile exit {exit_code}", out.strip())
        if diagnostics:
            return ("BLOCKED", f"diagnostics JSON unexpectedly has {len(diagnostics)} item(s)", "")
        return ("RESOLVED", "diagnostics JSON exported empty", "")


def main():
    try:
        staged_bin = stage_bin_copy("./fisics", REPO_ROOT, prefix="probe-fisics-")
    except FileNotFoundError as exc:
        print(str(exc))
        return 1

    clang_path = shutil.which("clang")
    print("Probe Runner")
    print(f"fisics_source: {staged_bin.resolved_path}")
    print(f"fisics: {staged_bin.staged_path}")
    if staged_bin.used_fallback:
        print("fisics_resolution: using numbered sibling fallback")
    else:
        print("fisics_resolution: using requested repo-root binary")
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

    try:
        print("[runtime probes]")
        for probe in RUNTIME_PROBES:
            if not probe_selected(probe.probe_id, filters):
                continue
            selected += 1
            status, summary, detail = run_runtime_probe(probe, clang_path, staged_bin.staged_path)
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
        print("[object probes]")
        readobj, objdump, tool_error = llvm_inspection_tools()
        for probe in OBJECT_PROBES:
            if not probe_selected(probe.probe_id, filters):
                continue
            selected += 1
            if tool_error:
                status, summary, detail = (
                    "BLOCKED",
                    "LLVM inspection tools unavailable",
                    tool_error,
                )
            else:
                status, summary, detail = run_object_probe(
                    probe,
                    clang_path,
                    staged_bin.staged_path,
                    readobj,
                    objdump,
                )
            print(f"{status:8s} {probe.probe_id} - {summary}")
            print(f"         note: {probe.note}")
            if status == "BLOCKED":
                emit_probe_blocked_classification(probe, "object", summary)
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
            status, summary, detail = run_diag_probe(probe, staged_bin.staged_path)
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
            status, summary, detail = run_diag_json_probe(probe, staged_bin.staged_path)
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
        return 1 if blocked else 0
    finally:
        staged_bin.cleanup()
