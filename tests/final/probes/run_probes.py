#!/usr/bin/env python3
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


PROBE_DIR = Path(__file__).resolve().parent
REPO_ROOT = PROBE_DIR.parent.parent.parent
FISICS = REPO_ROOT / "fisics"
COMPILE_TIMEOUT_SEC = 20
RUN_TIMEOUT_SEC = 8


@dataclass
class RuntimeProbe:
    probe_id: str
    source: Path
    note: str


@dataclass
class DiagnosticProbe:
    probe_id: str
    source: Path
    note: str
    required_substrings: Sequence[str] | None = None


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_fnptr_array_call_runtime",
        source=PROBE_DIR / "runtime/04__probe_fnptr_array_call_runtime.c",
        note="function-pointer array call path should match clang runtime result",
    ),
    RuntimeProbe(
        probe_id="04__probe_tag_block_shadow_ok",
        source=PROBE_DIR / "runtime/04__probe_tag_block_shadow_ok.c",
        note="inner-scope struct tag shadowing should compile/run cleanly",
    ),
    RuntimeProbe(
        probe_id="04__probe_deep_declarator_call_only",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_call_only.c",
        note="deep declarator call-only path should compile/run cleanly",
    ),
    RuntimeProbe(
        probe_id="04__probe_deep_declarator_codegen_hang",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_codegen_hang.c",
        note="deep declarator runtime path should compile and run (no codegen hang)",
    ),
    RuntimeProbe(
        probe_id="10__probe_static_function_then_extern_decl_ok",
        source=PROBE_DIR / "runtime/10__probe_static_function_then_extern_decl_ok.c",
        note="file-scope static function followed by extern declaration should remain valid",
    ),
    RuntimeProbe(
        probe_id="11__probe_function_param_function_type_adjust_ok",
        source=PROBE_DIR / "runtime/11__probe_function_param_function_type_adjust_ok.c",
        note="function parameter declared as function type should adjust to pointer and accept matching function argument",
    ),
    RuntimeProbe(
        probe_id="14__probe_unsigned_wrap",
        source=PROBE_DIR / "runtime/14__probe_unsigned_wrap.c",
        note="unsigned wrap behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_nan",
        source=PROBE_DIR / "runtime/14__probe_float_nan.c",
        note="NaN self-inequality should match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_switch_loop_lite",
        source=PROBE_DIR / "runtime/15__probe_switch_loop_lite.c",
        note="loop+switch lowering should match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_switch_loop_mod5",
        source=PROBE_DIR / "runtime/15__probe_switch_loop_mod5.c",
        note="loop+mod+switch lowering should match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_path_decl_nested_runtime",
        source=PROBE_DIR / "runtime/15__probe_path_decl_nested_runtime.c",
        note="function-pointer runtime call path should not crash",
    ),
    RuntimeProbe(
        probe_id="05__probe_typedef_shadow_parenthesized_expr",
        source=PROBE_DIR / "runtime/05__probe_typedef_shadow_parenthesized_expr.c",
        note="parenthesized identifier should win over cast when typedef is shadowed",
    ),
    RuntimeProbe(
        probe_id="05__probe_precedence_runtime",
        source=PROBE_DIR / "runtime/05__probe_precedence_runtime.c",
        note="operator precedence runtime result should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_unsigned_arith_conv_runtime",
        source=PROBE_DIR / "runtime/05__probe_unsigned_arith_conv_runtime.c",
        note="usual arithmetic conversions for signed/unsigned mix should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_runtime.c",
        note="nested ternary associativity/runtime behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_outer_true_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_outer_true_runtime.c",
        note="nested ternary outer-true path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_false_chain_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_false_chain_runtime.c",
        note="nested ternary false-chain path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_short_circuit_and_runtime",
        source=PROBE_DIR / "runtime/05__probe_short_circuit_and_runtime.c",
        note="logical AND short-circuit side effects should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_short_circuit_or_runtime",
        source=PROBE_DIR / "runtime/05__probe_short_circuit_or_runtime.c",
        note="logical OR short-circuit side effects should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_comma_eval_runtime",
        source=PROBE_DIR / "runtime/05__probe_comma_eval_runtime.c",
        note="comma operator evaluation/value behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_vla_sizeof_side_effect_runtime",
        source=PROBE_DIR / "runtime/05__probe_vla_sizeof_side_effect_runtime.c",
        note="sizeof(VLA) should evaluate bound side effects and match clang runtime",
    ),
    RuntimeProbe(
        probe_id="05__probe_ternary_side_effect_runtime",
        source=PROBE_DIR / "runtime/05__probe_ternary_side_effect_runtime.c",
        note="conditional operator branch side effects should match clang runtime",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_deep_false_chain_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_deep_false_chain_runtime.c",
        note="deep nested ternary false-chain path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_compound_literal_array_runtime",
        source=PROBE_DIR / "runtime/05__probe_compound_literal_array_runtime.c",
        note="compound-literal array value/lifetime-in-block behavior should match clang",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_block_extern_initializer_reject",
        source=PROBE_DIR / "diagnostics/04__probe_block_extern_initializer_reject.c",
        note="block-scope extern declaration with initializer should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_extern_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_extern_reject.c",
        note="parameter declaration should reject extern storage class",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_static_nonarray_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_static_nonarray_reject.c",
        note="parameter declaration should reject static when not used in array parameter form",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_void_named_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_void_named_reject.c",
        note="named parameter with type void should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_enum_const_typedef_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_enum_const_typedef_conflict_reject.c",
        note="enumerator should not reuse an existing typedef name in same scope",
    ),
    DiagnosticProbe(
        probe_id="04__probe_enum_const_var_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_enum_const_var_conflict_reject.c",
        note="enumerator should not reuse an existing variable name in same scope",
    ),
    DiagnosticProbe(
        probe_id="04__probe_tag_cross_kind_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_tag_cross_kind_conflict_reject.c",
        note="struct/enum tags sharing one name in same scope should be rejected",
    ),
    DiagnosticProbe(
        probe_id="05__probe_conditional_void_condition_reject",
        source=PROBE_DIR / "diagnostics/05__probe_conditional_void_condition_reject.c",
        note="conditional operator first operand must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_conditional_struct_result_reject",
        source=PROBE_DIR / "diagnostics/05__probe_conditional_struct_result_reject.c",
        note="conditional expression with struct result should be rejected in int return context",
    ),
    DiagnosticProbe(
        probe_id="05__probe_logical_and_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_logical_and_struct_reject.c",
        note="logical && operands must be scalar (struct operand should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_not_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_not_struct_reject.c",
        note="unary ! operand must be scalar (struct operand should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_address_of_rvalue_reject",
        source=PROBE_DIR / "diagnostics/05__probe_address_of_rvalue_reject.c",
        note="address-of operator should reject non-lvalue operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_deref_non_pointer_reject",
        source=PROBE_DIR / "diagnostics/05__probe_deref_non_pointer_reject.c",
        note="dereference operator should reject non-pointer operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_incomplete_type_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_incomplete_type_reject.c",
        note="sizeof should reject incomplete object types",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_function_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_function_reject.c",
        note="sizeof should reject function types",
    ),
    DiagnosticProbe(
        probe_id="05__probe_logical_void_operand_reject",
        source=PROBE_DIR / "diagnostics/05__probe_logical_void_operand_reject.c",
        note="logical operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_relational_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_relational_void_reject.c",
        note="relational operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_ternary_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/05__probe_ternary_struct_condition_reject.c",
        note="conditional operator first operand must be scalar (struct should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_cast_int_to_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_cast_int_to_struct_reject.c",
        note="casts between scalar and non-scalar types should reject",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_void_reject.c",
        note="_Alignof should reject void type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_incomplete_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_incomplete_reject.c",
        note="_Alignof should reject incomplete type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_shift_width_large_reject",
        source=PROBE_DIR / "diagnostics/05__probe_shift_width_large_reject.c",
        note="left/right shift should reject widths >= bit width of promoted lhs",
    ),
    DiagnosticProbe(
        probe_id="05__probe_bitwise_float_reject",
        source=PROBE_DIR / "diagnostics/05__probe_bitwise_float_reject.c",
        note="bitwise operators should reject floating operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_relational_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_relational_struct_reject.c",
        note="relational operators should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_equality_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_equality_struct_reject.c",
        note="equality operators should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_add_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_add_void_reject.c",
        note="arithmetic operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_bitnot_float_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_bitnot_float_reject.c",
        note="unary bitwise-not should reject floating operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_plus_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_plus_struct_reject.c",
        note="unary plus should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_minus_pointer_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_minus_pointer_reject.c",
        note="unary minus should reject pointer operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_void_reject.c",
        note="sizeof should reject void type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_expr_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_expr_reject.c",
        note="_Alignof should reject expression operand in strict C mode",
    ),
    DiagnosticProbe(
        probe_id="07__probe_assign_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_assign_struct_to_int_reject.c",
        note="assignment from struct value to int object should be rejected",
    ),
    DiagnosticProbe(
        probe_id="08__probe_designator_unknown_field_reject",
        source=PROBE_DIR / "diagnostics/08__probe_designator_unknown_field_reject.c",
        note="designated initializer should reject unknown field names",
    ),
    DiagnosticProbe(
        probe_id="09__probe_goto_into_vla_scope_reject",
        source=PROBE_DIR / "diagnostics/09__probe_goto_into_vla_scope_reject.c",
        note="goto into VLA scope should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_float_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_float_condition_reject.c",
        note="switch controlling expression must be integer type (float should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_pointer_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_pointer_condition_reject.c",
        note="switch controlling expression must be integer type (pointer should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_string_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_string_condition_reject.c",
        note="switch controlling expression must be integer type (string should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_double_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_double_condition_reject.c",
        note="switch controlling expression must be integer type (double should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_if_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_if_void_condition_reject.c",
        note="if controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_if_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_if_struct_condition_reject.c",
        note="if controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_while_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_while_void_condition_reject.c",
        note="while controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_do_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_do_struct_condition_reject.c",
        note="do-while controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_for_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_for_void_condition_reject.c",
        note="for controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_for_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_for_struct_condition_reject.c",
        note="for controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="10__probe_block_extern_different_type_reject",
        source=PROBE_DIR / "diagnostics/10__probe_block_extern_different_type_reject.c",
        note="block-scope extern should reject redeclaration with conflicting type",
    ),
    DiagnosticProbe(
        probe_id="11__probe_duplicate_param_name_reject",
        source=PROBE_DIR / "diagnostics/11__probe_duplicate_param_name_reject.c",
        note="duplicate parameter names in one function declarator should be rejected",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_auto_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_auto_reject.c",
        note="function parameter should reject auto storage class",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_void_plus_other_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_void_plus_other_reject.c",
        note="parameter list must reject 'void' combined with additional parameters",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_incomplete_type_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_incomplete_type_reject.c",
        note="function parameter declared with incomplete struct type should be rejected",
    ),
    DiagnosticProbe(
        probe_id="11__probe_return_void_expr_in_int_reject",
        source=PROBE_DIR / "diagnostics/11__probe_return_void_expr_in_int_reject.c",
        note="int-returning function should reject returning void expression",
    ),
    DiagnosticProbe(
        probe_id="12__probe_invalid_shift_width",
        source=PROBE_DIR / "diagnostics/12__probe_invalid_shift_width.c",
        note="should emit diagnostic for negative shift width",
        required_substrings=("Invalid shift width",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_incompatible_ptr_assign",
        source=PROBE_DIR / "diagnostics/12__probe_incompatible_ptr_assign.c",
        note="should emit assignment/pointer incompatibility diagnostic",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_illegal_struct_to_int_cast",
        source=PROBE_DIR / "diagnostics/12__probe_illegal_struct_to_int_cast.c",
        note="should emit invalid cast diagnostic",
        required_substrings=("Invalid cast between non-scalar types",),
    ),
]


def run_cmd(cmd, timeout_sec):
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or "") + (exc.stderr or "")
        return 124, out, True
    return proc.returncode, proc.stdout, False


def run_binary(path, timeout_sec):
    try:
        proc = subprocess.run(
            [str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        out = exc.stdout or ""
        err = exc.stderr or ""
        return 124, out, err, True
    return proc.returncode, proc.stdout, proc.stderr, False


def run_runtime_probe(probe, clang_path):
    with tempfile.TemporaryDirectory(prefix=f"probe-{probe.probe_id}-") as tmp:
        tmp_dir = Path(tmp)
        fisics_exe = tmp_dir / "fisics.out"
        clang_exe = tmp_dir / "clang.out"

        fisics_compile_exit, fisics_compile_out, fisics_compile_timeout = run_cmd(
            [str(FISICS), str(probe.source), "-o", str(fisics_exe)],
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
            [clang_path, "-std=c99", "-O0", str(probe.source), "-o", str(clang_exe)],
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
    _, out, _ = run_cmd([str(FISICS), str(probe.source)], COMPILE_TIMEOUT_SEC)
    if probe.required_substrings:
        lowered = out.lower()
        for needle in probe.required_substrings:
            if needle.lower() in lowered:
                return ("RESOLVED", f"diagnostic now emitted ({needle})", "")
        return ("BLOCKED", "expected diagnostic substring missing", "")

    has_diag = "Error at (" in out or "Error:" in out or ": error:" in out
    if has_diag:
        return ("RESOLVED", "diagnostic now emitted", "")
    return ("BLOCKED", "diagnostic missing", "")


def main():
    if not FISICS.exists():
        print(f"fisics binary not found at {FISICS}")
        return 1

    clang_path = shutil.which("clang")
    print("Probe Runner")
    print(f"fisics: {FISICS}")
    print(f"clang: {clang_path or 'not found'}")
    print("")

    blocked = 0
    resolved = 0
    skipped = 0

    print("[runtime probes]")
    for probe in RUNTIME_PROBES:
        status, summary, detail = run_runtime_probe(probe, clang_path)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
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
        status, summary, detail = run_diag_probe(probe)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print(f"Summary: blocked={blocked}, resolved={resolved}, skipped={skipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
