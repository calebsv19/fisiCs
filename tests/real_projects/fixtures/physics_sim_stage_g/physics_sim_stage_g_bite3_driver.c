#include "physics_sim_stage_g_common.h"

#include "app/sim_runtime_3d_domain.h"
#include "app/sim_runtime_emitter.h"
#include "app/sim_runtime_obstacle.h"

static uint64_t trace_domain(uint64_t digest, const char *checkpoint,
                             AppConfig *cfg, FluidScenePreset *preset) {
    SimRuntime3DDomainDesc domain = {0};
    char detail[256];
    if (!sim_runtime_3d_domain_desc_from_legacy(cfg, preset, &domain)) return 0u;
    snprintf(detail, sizeof(detail), "request=%d,%d,%d|applied=%d,%d,%d|policy=%s",
             cfg->grid_w, cfg->grid_h, cfg->grid_d, domain.grid_w, domain.grid_h,
             domain.grid_d, sim_runtime_3d_depth_policy_label(domain.depth_policy));
    digest = ps_stageg_hash_text(digest, detail);
    ps_stageg_trace(checkpoint, detail, digest);
    return digest;
}

int main(void) {
    AppConfig cfg = {0};
    FluidScenePreset preset = {0};
    SimRuntime3DDomainDesc domain = {0};
    SimRuntimeEmitterResolved emitter = {0};
    SimRuntimeEmitterPlacement3D placement = {0};
    SimRuntimeObstacleBounds3D bounds = {0};
    char canonical[512];
    uint64_t digest = UINT64_C(1469598103934665603);

    cfg.grid_w = 64; cfg.grid_h = 32; cfg.grid_d = 16;
    preset.domain_width = 4.0f; preset.domain_height = 2.0f;
    digest = trace_domain(digest, "b3_begin", &cfg, &preset);
    if (!digest) return 30;
    cfg.grid_w = 32; cfg.grid_h = 64; cfg.grid_d = 12;
    digest = trace_domain(digest, "b3_update", &cfg, &preset);
    if (!digest) return 31;
    cfg.grid_w = 64; cfg.grid_h = 32; cfg.grid_d = 16;
    digest = trace_domain(digest, "b3_reverse", &cfg, &preset);
    if (!digest) return 32;

    if (!sim_runtime_3d_domain_desc_from_legacy(&cfg, &preset, &domain)) return 33;
    preset.emitter_count = 1;
    preset.emitters[0] = (FluidEmitter){
        .type = EMITTER_DENSITY_SOURCE, .position_x = 1.25f, .position_y = -0.25f,
        .position_z = 50.0f, .radius = 5.0f, .strength = 1.0f,
        .attached_object = -1, .attached_import = -1
    };
    if (!sim_runtime_emitter_resolve(&preset, 0u, &emitter) ||
        !sim_runtime_emitter_resolve_3d_placement(&domain, &emitter, &placement)) return 34;
    snprintf(canonical, sizeof(canonical), "center=%d,%d,%d|bounds=%d,%d,%d,%d,%d,%d",
             placement.center_x, placement.center_y, placement.center_z,
             placement.min_x, placement.max_x, placement.min_y, placement.max_y,
             placement.min_z, placement.max_z);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b3_clamped", canonical, digest);

    if (sim_runtime_emitter_resolve(&preset, 1u, &emitter)) return 35;
    if (sim_runtime_obstacle_domain_face_bounds(&domain,
            (SimRuntimeBoundaryFace)SIM_RUNTIME_BOUNDARY_FACE_COUNT, &bounds)) return 36;
    digest = ps_stageg_hash_text(digest, "invalid_emitter=0|invalid_face=0");
    ps_stageg_trace("b3_invalid", "invalid_emitter=0,invalid_face=0", digest);

    snprintf(canonical, sizeof(canonical), "schema=physics_sim_stage_g_bite3_v1\ndigest=%016llx\n",
             (unsigned long long)digest);
    if (!ps_stageg_write_text("interaction.canonical", canonical)) return 37;
    ps_stageg_trace("b3_canonical", "artifact=interaction.canonical", digest);
    ps_stageg_trace("b3_shutdown", "interaction_active=0", digest);
    return 0;
}
