from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


@dataclass
class RuntimeProbe:
    probe_id: str
    source: Path
    note: str
    inputs: Sequence[Path] | None = None
    extra_differential_compiler: str | None = None


@dataclass
class DiagnosticProbe:
    probe_id: str
    source: Path
    note: str
    expect_any_diagnostic: bool = True
    required_substrings: Sequence[str] | None = None
    inputs: Sequence[Path] | None = None


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
