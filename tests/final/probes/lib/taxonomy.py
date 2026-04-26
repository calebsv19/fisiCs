from .models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe

ProbeLike = RuntimeProbe | DiagnosticProbe | DiagnosticJsonProbe


PROBE_OWNER_LANES = {
    "01": "translation-phases",
    "02": "lexer",
    "03": "preprocessor",
    "04": "declarations",
    "05": "expressions",
    "06": "lvalues-rvalues",
    "07": "types-conversions",
    "08": "initializers-layout",
    "09": "statements-controlflow",
    "10": "scopes-linkage",
    "11": "functions-calls",
    "12": "diagnostics-recovery",
    "13": "codegen-ir",
    "14": "runtime-surface",
    "15": "torture-differential",
}


def probe_owner_lane(probe_id):
    return PROBE_OWNER_LANES.get(probe_id.split("__", 1)[0], "probe-harness")


def classify_probe_trust_layer(probe_family, owner_lane, input_count):
    if probe_family == "runtime":
        return "Layer E"
    if input_count > 1 or owner_lane == "scopes-linkage":
        return "Layer D"
    return "Layer B"


def classify_blocked_probe(probe: ProbeLike, probe_family, summary):
    owner_lane = probe_owner_lane(probe.probe_id)
    input_count = len(probe.inputs) if probe.inputs else 1
    trust_layer = classify_probe_trust_layer(probe_family, owner_lane, input_count)
    lowered = summary.lower()

    if probe_family == "runtime":
        if lowered.startswith("fisics runtime timeout"):
            return ("runtime_crash", "critical", trust_layer, owner_lane)
        if lowered.startswith("runtime mismatch vs"):
            return ("runtime_mismatch", "critical", trust_layer, owner_lane)
        if lowered.startswith("fisics compile timeout") or lowered.startswith("fisics compile failed"):
            return ("ir_or_codegen_fail", "high", trust_layer, owner_lane)
        if lowered.startswith("clang ") or lowered.startswith("gcc "):
            return ("harness_error", "medium", trust_layer, owner_lane)
        return ("harness_error", "medium", trust_layer, owner_lane)

    if probe_family == "diagnostic":
        return ("wrong_diagnostics", "medium", trust_layer, owner_lane)

    if probe_family == "diagnostic-json":
        if lowered.startswith("compile timeout"):
            return ("ir_or_codegen_fail", "high", trust_layer, owner_lane)
        return ("wrong_diagnostics", "medium", trust_layer, owner_lane)

    return ("harness_error", "medium", trust_layer, owner_lane)


def emit_probe_blocked_classification(probe: ProbeLike, probe_family, summary):
    failure_kind, severity, trust_layer, owner_lane = classify_blocked_probe(
        probe, probe_family, summary
    )
    print(
        "         classification: "
        f"failure_kind={failure_kind} severity={severity} "
        f"source_lane=probe trust_layer={trust_layer} owner_lane={owner_lane} "
        "raw_status=BLOCKED"
    )
