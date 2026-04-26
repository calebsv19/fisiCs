from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_corpus_external_compile_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_compile_smoke.c',
        note='external-corpus styled deterministic parser/normalizer should compile/run and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_parser_fragment_a_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_parser_fragment_a_smoke.c',
        note='external-corpus parser fragment A should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_parser_fragment_b_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_parser_fragment_b_smoke.c',
        note='external-corpus parser fragment B should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_parser_fragment_c_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_parser_fragment_c_smoke.c',
        note='external-corpus parser fragment C should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_parser_fragment_d_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_parser_fragment_d_smoke.c',
        note='external-corpus parser fragment D should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_a_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_a_smoke.c',
        note='map_forge reduced app fragment A corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_b_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_b_smoke.c',
        note='map_forge reduced app fragment B corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_c_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_c_smoke.c',
        note='map_forge reduced app fragment C corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_d_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_d_smoke.c',
        note='map_forge reduced app fragment D corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_e_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_e_smoke.c',
        note='map_forge reduced app fragment E corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_corpus_reduction_replay_hash',
        source=PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_main.c',
        note='multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_main.c', PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_f_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_f_smoke.c',
        note='map_forge reduced app fragment F corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_g_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_g_smoke.c',
        note='map_forge reduced app fragment G corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_corpus_reduction_replay_hash_ii',
        source=PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_main.c',
        note='second multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_main.c', PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_h_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_h_smoke.c',
        note='map_forge reduced app fragment H corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_i_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_i_smoke.c',
        note='map_forge reduced app fragment I corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_corpus_reduction_replay_hash_iii',
        source=PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_main.c',
        note='third multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_main.c', PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_j_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_j_smoke.c',
        note='map_forge reduced app fragment J corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_k_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_k_smoke.c',
        note='map_forge reduced app fragment K corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_corpus_reduction_replay_hash_iv',
        source=PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_main.c',
        note='fourth multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_main.c', PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_l_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_l_smoke.c',
        note='map_forge reduced app fragment L corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_realproj_map_forge_app_fragment_m_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_realproj_map_forge_app_fragment_m_smoke.c',
        note='map_forge reduced app fragment M corpus lane should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_corpus_reduction_replay_hash_v',
        source=PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_v_main.c',
        note='fifth multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_v_main.c', PROBE_DIR / 'runtime/15__probe_multitu_corpus_reduction_replay_hash_v_lib.c'],
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
