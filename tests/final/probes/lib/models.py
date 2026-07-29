from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Sequence


@dataclass
class RuntimeProbe:
    probe_id: str
    source: Path
    note: str
    inputs: Sequence[Path] | None = None
    mixed_clang_inputs: Sequence[Path] | None = None
    extra_differential_compiler: str | None = None
    fisics_args: Sequence[str] | None = None
    fisics_env: Mapping[str, str] | None = None
    clang_args: Sequence[str] | None = None
    clang_env: Mapping[str, str] | None = None
    expected_exit_code: int | None = None
    expected_stdout: str | None = None
    expected_stdout_variants: Sequence[str] | None = None
    expected_stderr: str | None = None
    promoted_test_id: str | None = None
    promotion_disposition: str | None = None


@dataclass
class ObjectProbe:
    probe_id: str
    source: Path
    note: str
    required_exports: Sequence[str]
    allowed_undefined: Sequence[str] = ()
    clang_allowed_undefined: Sequence[str] | None = None
    allowed_relocations: Sequence[str] = ()
    clang_allowed_relocations: Sequence[str] | None = None
    forbidden_instructions: Sequence[str] = ()
    forbid_red_zone: bool = True
    scalar_sse2: bool = False
    fisics_args: Sequence[str] | None = None
    clang_args: Sequence[str] | None = None


@dataclass
class DiagnosticProbe:
    probe_id: str
    source: Path
    note: str
    expect_any_diagnostic: bool = True
    required_substrings: Sequence[str] | None = None
    forbidden_substrings: Sequence[str] | None = None
    inputs: Sequence[Path] | None = None
    fisics_args: Sequence[str] | None = None
    fisics_env: Mapping[str, str] | None = None
    promoted_test_id: str | None = None
    promotion_disposition: str | None = None
    allowed_exit_codes: Sequence[int] = (0, 1)


@dataclass(frozen=True)
class DiagnosticExpectation:
    code: int | None = None
    line: int | None = None
    column: int | None = None
    has_file: bool | None = None
    file: str | None = None
    severity: str | None = None
    stage: str | None = None
    message_substrings: Sequence[str] = ()
    macro_trace: Sequence[Mapping[str, object]] | None = None


@dataclass
class DiagnosticJsonProbe:
    probe_id: str
    source: Path
    note: str
    require_any_diagnostic: bool = True
    expected_codes: Sequence[int] | None = None
    expected_line: int | None = None
    expected_column: int | None = None
    expected_has_file: bool | None = None
    inputs: Sequence[Path] | None = None
    fisics_args: Sequence[str] | None = None
    fisics_env: Mapping[str, str] | None = None
    promoted_test_id: str | None = None
    promotion_disposition: str | None = None
    expected_diagnostics: Sequence[DiagnosticExpectation] | None = None
    allowed_exit_codes: Sequence[int] = (0, 1)
