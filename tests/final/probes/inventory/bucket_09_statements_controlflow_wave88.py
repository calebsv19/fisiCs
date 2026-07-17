from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='09__probe_wave88_duff_do_while_fallthrough',
        source=PROBE_DIR / 'runtime/09__probe_wave88_duff_do_while_fallthrough.c',
        note='wave88 strict: canonical Duff dispatch enters a do/while body and preserves ordered case fallthrough across loop iterations',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_duff_do_while_fallthrough',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_switch_case_nested_for_targets',
        source=PROBE_DIR / 'runtime/09__probe_wave88_switch_case_nested_for_targets.c',
        note='wave88 strict: switch dispatch into a for body keeps continue on the loop latch and break on the loop exit',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_switch_case_nested_for_targets',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_switch_case_nested_while_reentry',
        source=PROBE_DIR / 'runtime/09__probe_wave88_switch_case_nested_while_reentry.c',
        note='wave88 strict: switch dispatch into a while body preserves continue re-entry and the innermost-loop break target',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_switch_case_nested_while_reentry',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_structural_switch_exhaustive_return',
        source=PROBE_DIR / 'runtime/09__probe_wave88_structural_switch_exhaustive_return.c',
        note='wave88 review regression: exhaustive nested-case/default dispatch proves structural switch return flow',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_structural_switch_exhaustive_return',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_switch_nested_if_condition_bypass',
        source=PROBE_DIR / 'runtime/09__probe_wave88_switch_nested_if_condition_bypass.c',
        note='wave88 review regression: direct case dispatch into an if body bypasses the if condition side effect',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_switch_nested_if_condition_bypass',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_switch_nested_for_initializer_bypass',
        source=PROBE_DIR / 'runtime/09__probe_wave88_switch_nested_for_initializer_bypass.c',
        note='wave88 review regression: direct case dispatch into a for body bypasses the for initializer side effect',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_switch_nested_for_initializer_bypass',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave88_switch_noncompound_body',
        source=PROBE_DIR / 'runtime/09__probe_wave88_switch_noncompound_body.c',
        note='wave88 review regression: C99 switch accepts a directly labeled non-compound statement body',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave88_switch_noncompound_body',
    ),
]


DIAG_PROBES = []
DIAG_JSON_PROBES = []
