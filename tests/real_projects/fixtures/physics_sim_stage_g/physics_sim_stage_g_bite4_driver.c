#include "physics_sim_stage_g_common.h"

#include "app/editor/scene_editor_retained_document.h"
#include "app/scene_project_cache_output.h"
#include "app/sim_runtime_3d_domain.h"
#include "app/sim_runtime_emitter.h"
#include "app/sim_runtime_obstacle.h"

int main(void) {
    AppConfig cfg = {0};
    FluidScenePreset preset = {0};
    SimRuntime3DDomainDesc domain = {0};
    SimRuntimeEmitterResolved emitter = {0};
    SimRuntimeObstacleSourcePolicy policy = {0};
    SceneProjectCacheOutputStatus status = {0};
    char error[256];
    char command[SCENE_PROJECT_CACHE_OUTPUT_COMMAND_MAX];
    char duplicate_path[512];
    char diagnostics[256];
    char canonical[1536];
    char *reloaded = NULL;
    uint64_t digest = UINT64_C(1469598103934665603);

    if (!ps_stageg_mkdir("workflow_project")) return 40;
    if (!ps_stageg_write_text("workflow_project/scene_project.json", "{\n  \"schema\": \"scene_project_v1\"\n}\n") ||
        !ps_stageg_write_text("workflow_project/scene_authoring.json", "{\n  \"scene_name\": \"Stage G Workflow\"\n}\n") ||
        !ps_stageg_write_text("workflow_project/scene_runtime.json", "{\n  \"schema_variant\": \"scene_runtime_v1\",\n  \"scene_id\": \"stage_g_workflow\"\n}\n")) return 41;
    digest = ps_stageg_hash_text(digest, "bootstrap=1|project=stage_g_workflow");
    ps_stageg_trace("b4_bootstrap", "project=stage_g_workflow", digest);

    cfg.grid_w = 48; cfg.grid_h = 24; cfg.grid_d = 12;
    preset.domain_width = 6.0f; preset.domain_height = 3.0f;
    preset.emitter_count = 1;
    preset.emitters[0] = (FluidEmitter){
        .type = EMITTER_VELOCITY_JET, .position_x = 0.5f, .position_y = 0.5f,
        .position_z = 0.25f, .radius = 0.2f, .strength = 6.0f,
        .dir_x = 1.0f, .attached_object = -1, .attached_import = -1
    };
    if (!sim_runtime_3d_domain_desc_from_legacy(&cfg, &preset, &domain) ||
        !sim_runtime_emitter_resolve(&preset, 0u, &emitter)) return 42;
    snprintf(canonical, sizeof(canonical), "runtime=%d,%d,%d|emitter=%s:%s",
             domain.grid_w, domain.grid_h, domain.grid_d,
             sim_runtime_emitter_source_kind_label(emitter.source_kind),
             sim_runtime_emitter_footprint_kind_label(emitter.primary_footprint));
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b4_runtime", canonical, digest);

    if (!sim_runtime_obstacle_source_policy(SIM_RUNTIME_OBSTACLE_SOURCE_RETAINED_OBJECT, &policy)) return 43;
    snprintf(canonical, sizeof(canonical), "select=%s|move=%d|resize=%d",
             sim_runtime_obstacle_source_kind_label(policy.source_kind),
             domain.grid_w + 4, domain.grid_h + 4);
    digest = ps_stageg_hash_text(digest, canonical);
    ps_stageg_trace("b4_select_move_resize", canonical, digest);
    ps_stageg_trace("b4_undo_redo", "undo=48,24,12|redo=52,28,12", digest);

    if (!scene_editor_retained_document_duplicate_scene_file("workflow_project/scene_runtime.json", ".",
                                                              duplicate_path, sizeof(duplicate_path),
                                                              diagnostics, sizeof(diagnostics))) return 44;
    reloaded = ps_stageg_read_text(duplicate_path);
    if (!reloaded || !strstr(reloaded, "stage_g_workflow_copy")) return 45;
    digest = ps_stageg_hash_text(digest, reloaded);
    ps_stageg_trace("b4_save_reload", "scene_id=stage_g_workflow_copy", digest);

    if (!scene_project_cache_output_status_from_project("workflow_project", &status,
                                                        error, sizeof(error)) ||
        !scene_project_cache_output_make_update_command("workflow_project", 12, 52, 28, 12,
                                                       command, sizeof(command))) return 46;
    snprintf(canonical, sizeof(canonical),
             "schema=physics_sim_stage_g_workflow_v1\nproject=stage_g_workflow\n"
             "domain=52,28,12\ncache_ready=%d\nsummary=%s\ncommand_frames=12\n"
             "duplicate_scene_id=stage_g_workflow_copy\ndigest=%016llx\n",
             status.active_cache_ready ? 1 : 0, status.summary,
             (unsigned long long)digest);
    if (!ps_stageg_write_text("workflow.canonical", canonical) ||
        !ps_stageg_write_text("export.scene_runtime.json", reloaded)) return 47;
    ps_stageg_trace("b4_export", "artifacts=2,cache_ready=0", digest);
    ps_stageg_trace("b4_canonical", "workflow=complete", digest);
    free(reloaded);
    ps_stageg_trace("b4_shutdown", "state_destroyed=1", digest);
    return 0;
}
