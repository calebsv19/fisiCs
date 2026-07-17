#include "physics_sim_stage_g_common.h"

#include "app/editor/scene_editor_retained_document.h"

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    const char *runtime_dir = "runtime_scenes";
    const char *source_dir = "runtime_scenes/source_scene";
    const char *source_path = "runtime_scenes/source_scene/scene_runtime.json";
    const char *authoring_path = "runtime_scenes/source_scene/scene_authoring.json";
    const char *runtime_json = "{\n  \"schema\": \"scene_runtime_v1\",\n  \"scene_id\": \"stage_g_source\",\n  \"gravity\": [0, -9.81, 0]\n}\n";
    char duplicate_path[512];
    char diagnostics[256];
    char save_path[512];
    char name[128];
    char canonical[1024];
    char *reloaded = NULL;
    uint64_t digest;

    if (!ps_stageg_mkdir(runtime_dir) || !ps_stageg_mkdir(source_dir)) return 20;
    if (!ps_stageg_write_text(source_path, runtime_json) ||
        !ps_stageg_write_text(authoring_path, "{\n  \"scene_name\": \"Stage G Source\"\n}\n")) return 21;
    scene_editor_retained_document_name_from_path(source_path, "fallback", name, sizeof(name));
    digest = ps_stageg_hash_text(basis, name);
    ps_stageg_trace("b2_mutated", "scene=source_scene", digest);

    if (!scene_editor_retained_document_duplicate_scene_file(source_path, runtime_dir,
                                                              duplicate_path, sizeof(duplicate_path),
                                                              diagnostics, sizeof(diagnostics))) return 22;
    reloaded = ps_stageg_read_text(duplicate_path);
    if (!reloaded) return 23;
    digest = ps_stageg_hash_text(digest, reloaded);
    ps_stageg_trace("b2_saved", "duplicate=stage_g_source_copy", digest);

    free(reloaded);
    reloaded = NULL;
    ps_stageg_trace("b2_destroyed", "memory_released=1", digest);
    reloaded = ps_stageg_read_text(duplicate_path);
    if (!reloaded || !strstr(reloaded, "\"scene_id\": \"stage_g_source_copy\"")) return 24;
    ps_stageg_trace("b2_reloaded", "scene_id=stage_g_source_copy", digest);

    if (!scene_editor_retained_document_resolve_save_path(runtime_dir, duplicate_path,
                                                          "ignored", "stage_g_source_copy",
                                                          save_path, sizeof(save_path)) ||
        strcmp(save_path, duplicate_path) != 0) return 25;
    ps_stageg_trace("b2_compared", "path_reused=1", digest);
    if (!ps_stageg_write_text("retained_scene.canonical", reloaded)) return 26;
    snprintf(canonical, sizeof(canonical), "name=%s\npath=runtime_scenes/source_scene_copy/scene_runtime.json\ndigest=%016llx\n",
             name, (unsigned long long)digest);
    if (!ps_stageg_write_text("persistence.canonical", canonical)) return 27;
    ps_stageg_trace("b2_canonical", "artifacts=2", digest);
    free(reloaded);
    ps_stageg_trace("b2_shutdown", "memory_released=1", digest);
    return 0;
}
