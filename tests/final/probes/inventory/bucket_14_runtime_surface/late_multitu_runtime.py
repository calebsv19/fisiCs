from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


def _probe(probe_id: str, *files: str) -> RuntimeProbe:
    inputs = [PROBE_DIR / f"runtime/{name}" for name in files]
    return RuntimeProbe(
        probe_id=probe_id,
        source=inputs[0],
        note="late promoted multi-TU runtime surface lane should match clang runtime behavior",
        inputs=inputs,
    )


RUNTIME_PROBES = [
    _probe("14__probe_multitu_token_generation_reimport_wrap_latest_braid", "14__probe_multitu_token_generation_reimport_wrap_latest_braid_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_reclaim_latest_lattice", "14__probe_multitu_token_reserve_shadow_reclaim_latest_lattice_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_wrap_shadow_spoke", "14__probe_multitu_token_generation_latest_wrap_shadow_spoke_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reclaim_shadow_latest_braid", "14__probe_multitu_token_reserve_reclaim_shadow_latest_braid_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_reimport_latest_shadow_weave", "14__probe_multitu_token_generation_reimport_latest_shadow_weave_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_reclaim_spine", "14__probe_multitu_token_reserve_latest_shadow_reclaim_spine_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_latest_reseed_mesh", "14__probe_multitu_token_generation_shadow_latest_reseed_mesh_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_latest_reimport_weave", "14__probe_multitu_token_reserve_shadow_latest_reimport_weave_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_shadow_reimport_braid", "14__probe_multitu_token_generation_latest_shadow_reimport_braid_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_reclaim_latest_mesh", "14__probe_multitu_token_reserve_shadow_reclaim_latest_mesh_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_wrap_latest_shadow_weave", "14__probe_multitu_token_generation_wrap_latest_shadow_weave_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_abort_weave", "14__probe_multitu_token_reserve_latest_shadow_abort_weave_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_reimport_latest_spine", "14__probe_multitu_token_generation_shadow_reimport_latest_spine_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_spoke", "14__probe_multitu_token_reserve_reimport_shadow_latest_spoke_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_reimport_latest_shadow_spoke", "14__probe_multitu_token_generation_reimport_latest_shadow_spoke_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_reclaim_latest_arc", "14__probe_multitu_token_reserve_shadow_reclaim_latest_arc_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_wrap_shadow_fanout", "14__probe_multitu_token_generation_latest_wrap_shadow_fanout_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_reimport_mesh", "14__probe_multitu_token_reserve_latest_shadow_reimport_mesh_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_wrap_latest_braid", "14__probe_multitu_token_generation_shadow_wrap_latest_braid_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_lattice", "14__probe_multitu_token_reserve_reimport_shadow_latest_lattice_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_shadow_reimport_fanout", "14__probe_multitu_token_generation_latest_shadow_reimport_fanout_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_reclaim_latest_weave", "14__probe_multitu_token_reserve_shadow_reclaim_latest_weave_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_reimport_wrap_latest_spine", "14__probe_multitu_token_generation_reimport_wrap_latest_spine_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_braid", "14__probe_multitu_token_reserve_reimport_shadow_latest_braid_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_latest_wrap_mesh", "14__probe_multitu_token_generation_shadow_latest_wrap_mesh_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_abort_lattice", "14__probe_multitu_token_reserve_latest_shadow_abort_lattice_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_wrap_shadow_arc", "14__probe_multitu_token_generation_latest_wrap_shadow_arc_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_weave", "14__probe_multitu_token_reserve_reimport_shadow_latest_weave_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_latest_wrap_spoke", "14__probe_multitu_token_generation_shadow_latest_wrap_spoke_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_abort_mesh", "14__probe_multitu_token_reserve_latest_shadow_abort_mesh_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_reimport_wrap_latest_fanout", "14__probe_multitu_token_generation_reimport_wrap_latest_fanout_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_spine", "14__probe_multitu_token_reserve_reimport_shadow_latest_spine_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_shadow_wrap_weave", "14__probe_multitu_token_generation_latest_shadow_wrap_weave_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_reclaim_mesh", "14__probe_multitu_token_reserve_latest_shadow_reclaim_mesh_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_reimport_latest_shadow_braid", "14__probe_multitu_token_generation_reimport_latest_shadow_braid_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_shadow_latest_fanout", "14__probe_multitu_token_reserve_reimport_shadow_latest_fanout_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_latest_wrap_arc", "14__probe_multitu_token_generation_shadow_latest_wrap_arc_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_abort_spoke", "14__probe_multitu_token_reserve_latest_shadow_abort_spoke_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_shadow_wrap_lattice", "14__probe_multitu_token_generation_latest_shadow_wrap_lattice_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_reimport_shadow_weave", "14__probe_multitu_token_reserve_latest_reimport_shadow_weave_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_latest_shadow_reimport_mesh", "14__probe_multitu_token_generation_latest_shadow_reimport_mesh_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_latest_shadow_abort_arc", "14__probe_multitu_token_reserve_latest_shadow_abort_arc_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_wrap_latest_shadow_lattice", "14__probe_multitu_token_generation_wrap_latest_shadow_lattice_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_reimport_latest_shadow_fanout", "14__probe_multitu_token_reserve_reimport_latest_shadow_fanout_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
    _probe("14__probe_multitu_token_generation_shadow_reimport_latest_weave", "14__probe_multitu_token_generation_shadow_reimport_latest_weave_main.c", "14__probe_multitu_token_generation_stale_drop_lib.c"),
    _probe("14__probe_multitu_token_reserve_shadow_reclaim_latest_spine", "14__probe_multitu_token_reserve_shadow_reclaim_latest_spine_main.c", "14__probe_multitu_token_reserve_commit_abort_lib.c"),
]
