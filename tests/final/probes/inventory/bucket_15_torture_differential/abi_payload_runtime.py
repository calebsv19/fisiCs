from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_abi_payload_nested_sret_dispatch',
        source=PROBE_DIR / 'runtime/15__probe_abi_payload_nested_sret_dispatch_main.c',
        note='nested aggregate payload SRet/by-value relay across TUs should preserve union/array fields',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_payload_nested_sret_dispatch_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_payload_nested_sret_dispatch_lib.c',
            PROBE_DIR / 'runtime/15__probe_abi_payload_nested_sret_dispatch_aux.c',
        ],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave128_nested_payload_ring',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave128_nested_payload_ring_main.c',
        note='multi-TU nested ABI payload ring should preserve arrays, unions, and returned aggregate fields through by-value relay and checksum folding',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave128_nested_payload_ring_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave128_nested_payload_ring_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave129_units_tagged_payload_preservation',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave129_units_tagged_payload_preservation_main.c',
        note='multi-TU ABI payload with physics-units-originated fields should preserve double lanes and union words through by-value callback relay and checksum folding',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave129_units_tagged_payload_preservation_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave129_units_tagged_payload_preservation_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave129_nested_callback_ring',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave129_nested_callback_ring_main.c',
        note='multi-TU nested ABI callback ring should preserve array and union fields through by-value return, function-pointer callback, and slot replacement',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave129_nested_callback_ring_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave129_nested_callback_ring_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave130_units_callback_aggregate_preservation',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave130_units_callback_aggregate_preservation_main.c',
        note='physics-units multi-TU ABI callback aggregate should preserve converted double fields plus nested union words through by-value frame returns and callback-selected cell transforms',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave130_units_callback_aggregate_preservation_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave130_units_callback_aggregate_preservation_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave131_units_nested_aggregate_callback_matrix',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave131_units_nested_aggregate_callback_matrix_main.c',
        note='physics-units multi-TU ABI nested aggregate callback matrix should preserve converted double fields, nested array slots, and union payload words through by-value packet returns',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave131_units_nested_aggregate_callback_matrix_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave131_units_nested_aggregate_callback_matrix_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave132_units_envelope_callback_handoff',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave132_units_envelope_callback_handoff_main.c',
        note='physics-units multi-TU ABI envelope callback handoff should preserve converted double fields, nested frame arrays, footer union words, and by-value aggregate returns through callback-selected transforms',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave132_units_envelope_callback_handoff_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave132_units_envelope_callback_handoff_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave133_units_return_storage_matrix',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave133_units_return_storage_matrix.c',
        note='physics-units ABI nested return/storage matrix should preserve converted double fields and nested union payload words through struct returns, array storage, and slot replacement',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_abi_wave133_units_callback_envelope_chain',
        source=PROBE_DIR / 'runtime/15__probe_abi_wave133_units_callback_envelope_chain_main.c',
        note='physics-units multi-TU ABI nested callback envelope chain should preserve converted double fields, nested segment arrays, union payload footers, and by-value aggregate returns through callback-selected transforms',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_abi_wave133_units_callback_envelope_chain_main.c',
            PROBE_DIR / 'runtime/15__probe_abi_wave133_units_callback_envelope_chain_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
