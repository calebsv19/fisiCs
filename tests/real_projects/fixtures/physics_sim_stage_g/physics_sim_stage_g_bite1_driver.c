#include "physics_sim_stage_g_common.h"

#include "app/sim_runtime_3d_domain.h"
#include "app/sim_runtime_emitter.h"
#include "app/sim_runtime_obstacle.h"

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    AppConfig cfg = {0};
    FluidScenePreset preset = {0};
    SimRuntime3DDomainDesc domain = {0};
    SimRuntimeEmitterResolved emitter = {0};
    SimRuntimeEmitterPlacement3D placement = {0};
    SimRuntimeObstacleContract obstacle = {0};
    SimRuntimeObstacleBounds3D bounds = {0};
    SimRuntime3DVolume volume = {0};
    char canonical[1024];
    uint64_t digest;

    cfg.quality_index = 5;
    cfg.grid_w = 64;
    cfg.grid_h = 32;
    cfg.grid_d = 16;
    preset.domain_width = 4.0f;
    preset.domain_height = 2.0f;
    preset.emitter_count = 1;
    preset.emitters[0] = (FluidEmitter){
        .type = EMITTER_VELOCITY_JET, .position_x = 0.25f, .position_y = 0.75f,
        .position_z = 0.5f, .radius = 0.125f, .strength = 8.0f,
        .dir_x = 0.0f, .dir_y = 3.0f, .dir_z = 4.0f,
        .attached_object = -1, .attached_import = -1
    };
    if (!sim_runtime_3d_domain_desc_from_legacy(&cfg, &preset, &domain)) return 10;
    snprintf(canonical, sizeof(canonical), "domain=%d,%d,%d|cells=%llu|policy=%s|voxel=%.6f",
             domain.grid_w, domain.grid_h, domain.grid_d,
             (unsigned long long)domain.cell_count,
             sim_runtime_3d_depth_policy_label(domain.depth_policy), domain.voxel_size);
    digest = ps_stageg_hash_text(basis, canonical);
    ps_stageg_trace("b1_domain", canonical, digest);

    if (!sim_runtime_emitter_resolve(&preset, 0u, &emitter) ||
        !sim_runtime_emitter_resolve_3d_placement(&domain, &emitter, &placement)) return 11;
    snprintf(canonical, sizeof(canonical),
             "emitter=%s:%s|dir=%.3f,%.3f,%.3f|placement=%d,%d,%d:%d:%d,%d,%d,%d,%d,%d",
             sim_runtime_emitter_source_kind_label(emitter.source_kind),
             sim_runtime_emitter_footprint_kind_label(emitter.primary_footprint),
             emitter.dir_x, emitter.dir_y, emitter.dir_z,
             placement.center_x, placement.center_y, placement.center_z, placement.radius_cells,
             placement.min_x, placement.max_x, placement.min_y, placement.max_y,
             placement.min_z, placement.max_z);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b1_emitter", canonical, digest);

    sim_runtime_obstacle_contract_default(&obstacle);
    if (!sim_runtime_obstacle_domain_face_bounds(&domain, SIM_RUNTIME_BOUNDARY_FACE_MAX_Z, &bounds)) return 12;
    snprintf(canonical, sizeof(canonical), "obstacle=%s:%s|max-z=%d,%d,%d,%d,%d,%d|walls=%d%d%d%d%d%d",
             sim_runtime_obstacle_storage_kind_label(obstacle.storage_kind),
             sim_runtime_obstacle_compatibility_policy_label(obstacle.compatibility_policy),
             bounds.min_x, bounds.max_x, bounds.min_y, bounds.max_y, bounds.min_z, bounds.max_z,
             obstacle.domain_walls_enabled[0], obstacle.domain_walls_enabled[1],
             obstacle.domain_walls_enabled[2], obstacle.domain_walls_enabled[3],
             obstacle.domain_walls_enabled[4], obstacle.domain_walls_enabled[5]);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b1_obstacle", canonical, digest);

    if (!sim_runtime_3d_volume_init(&volume, &domain)) return 13;
    volume.density[sim_runtime_3d_volume_index(&domain, placement.center_x,
                                               placement.center_y, placement.center_z)] = 9.5f;
    snprintf(canonical, sizeof(canonical), "volume=%llu|sample=%.3f",
             (unsigned long long)volume.desc.cell_count,
             volume.density[sim_runtime_3d_volume_index(&domain, placement.center_x,
                                                        placement.center_y, placement.center_z)]);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b1_volume_mutated", canonical, digest);
    sim_runtime_3d_volume_clear(&volume);
    snprintf(canonical, sizeof(canonical), "volume=%llu|sample=%.3f",
             (unsigned long long)volume.desc.cell_count,
             volume.density[sim_runtime_3d_volume_index(&domain, placement.center_x,
                                                        placement.center_y, placement.center_z)]);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b1_volume_cleared", canonical, digest);
    sim_runtime_3d_volume_destroy(&volume);

    snprintf(canonical, sizeof(canonical), "schema=physics_sim_stage_g_bite1_v1\ndigest=%016llx\n",
             (unsigned long long)digest);
    if (!ps_stageg_write_text("runtime_state.canonical", canonical)) return 14;
    ps_stageg_trace("b1_canonical", "artifact=runtime_state.canonical", digest);
    ps_stageg_trace("b1_shutdown", "volume_destroyed=1", digest);
    return 0;
}
