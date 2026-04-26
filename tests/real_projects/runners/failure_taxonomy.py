from __future__ import annotations

from typing import Any


def classify_real_project_blocker(
    stage_key: str, row: dict[str, Any]
) -> dict[str, str]:
    parity = str(row.get("parity") or "blocker")
    fisics = row.get("fisics") or {}
    failure_phase = str(fisics.get("failure_phase") or "")
    failure_class = str(fisics.get("failure_class") or "")

    failure_kind, severity = _canonical_blocker_kind(
        stage_key=stage_key,
        failure_phase=failure_phase,
        failure_class=failure_class,
    )
    return {
        "failure_kind": failure_kind,
        "severity": severity,
        "source_lane": "real_project",
        "trust_layer": "Layer F",
        "owner_lane": _owner_lane(stage_key),
        "raw_status": parity,
        "raw_detail": _raw_detail(stage_key, failure_phase, failure_class),
    }


def format_real_project_blocker_line(stage_key: str, row: dict[str, Any]) -> str:
    classification = row.get("blocker_classification")
    if not isinstance(classification, dict):
        classification = classify_real_project_blocker(stage_key, row)

    subject_key = "source" if stage_key == "A_tu_compile" else "target"
    subject_value = str(row.get(subject_key) or "<unknown>")
    fisics = row.get("fisics") or {}
    failure_phase = str(fisics.get("failure_phase") or "")
    failure_class = str(fisics.get("failure_class") or "")

    detail_parts = [f"{subject_key}={subject_value}", f"parity={row.get('parity', '<unknown>')}"]
    if failure_phase:
        detail_parts.append(f"failure_phase={failure_phase}")
    if failure_class:
        detail_parts.append(f"failure_class={failure_class}")

    return (
        f"BLOCKER [{subject_key}={subject_value}] "
        f"[failure_kind={classification['failure_kind']} "
        f"severity={classification['severity']} "
        f"source_lane={classification['source_lane']} "
        f"trust_layer={classification['trust_layer']} "
        f"owner_lane={classification['owner_lane']} "
        f"raw_status={classification['raw_status']}]: "
        + " ".join(detail_parts)
    )


def _canonical_blocker_kind(
    *, stage_key: str, failure_phase: str, failure_class: str
) -> tuple[str, str]:
    if stage_key == "A_tu_compile":
        return _map_compile_class(failure_class)

    if failure_phase == "compile":
        return _map_compile_class(failure_class)
    if failure_phase == "link":
        return ("link_fail", "high")
    if failure_phase == "runtime":
        if failure_class in ("timeout", "runtime_crash"):
            return ("runtime_crash", "critical")
        return ("runtime_mismatch", "critical")
    if failure_phase == "golden":
        return ("runtime_mismatch", "critical")

    return ("harness_error", "medium")


def _map_compile_class(failure_class: str) -> tuple[str, str]:
    if failure_class == "parser":
        return ("parser_fail", "high")
    if failure_class == "semantic":
        return ("semantic_reject", "high")
    if failure_class in ("codegen", "timeout"):
        return ("ir_or_codegen_fail", "high")
    if failure_class == "link":
        return ("link_fail", "high")
    return ("harness_error", "medium")


def _owner_lane(stage_key: str) -> str:
    return {
        "A_tu_compile": "realproj-stage-a",
        "B_link_subset": "realproj-stage-b",
        "C_full_build": "realproj-stage-c",
        "D_runtime_smoke": "realproj-stage-d",
        "E_golden_behavior": "realproj-stage-e",
    }.get(stage_key, "realproj-harness")


def _raw_detail(stage_key: str, failure_phase: str, failure_class: str) -> str:
    if stage_key == "A_tu_compile":
        return f"failure_class={failure_class or 'unknown'}"
    detail_parts: list[str] = []
    if failure_phase:
        detail_parts.append(f"failure_phase={failure_phase}")
    if failure_class:
        detail_parts.append(f"failure_class={failure_class}")
    if not detail_parts:
        return "failure_detail=unknown"
    return " ".join(detail_parts)
