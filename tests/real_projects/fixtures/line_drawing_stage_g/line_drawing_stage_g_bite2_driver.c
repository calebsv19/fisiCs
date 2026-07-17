#include "line_drawing_stage_g_common.h"

#include "Layout/layout.h"
#include "Tools/canonical_scene_export_authoring.h"
#include "Tools/scene_authoring_import.h"
#include "cjson/cJSON.h"

#include <stdlib.h>

static cJSON* export_authoring(const Layout* layout) {
    cJSON* root = cJSON_CreateObject();
    cJSON* materials = cJSON_CreateArray();
    cJSON* lights = cJSON_CreateArray();
    cJSON* cameras = cJSON_CreateArray();
    cJSON* paths = cJSON_CreateArray();
    if (!root || !materials || !lights || !cameras || !paths) return NULL;
    cJSON_AddStringToObject(root, "schema", "scene_authoring_v1");
    cJSON_AddItemToObject(root, "materials", materials);
    cJSON_AddItemToObject(root, "lights", lights);
    cJSON_AddItemToObject(root, "cameras", cameras);
    cJSON_AddItemToObject(root, "paths", paths);
    if (!LineDrawingCanonicalScene_AppendLiveSceneAuthoringRecords(
            materials, lights, cameras, paths, layout, "flat_color", "perspective")) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static int write_json(const char* path, const cJSON* root) {
    char* text = cJSON_PrintUnformatted(root);
    FILE* file;
    size_t length;
    int ok;
    if (!text) return 0;
    file = fopen(path, "wb");
    if (!file) { cJSON_free(text); return 0; }
    length = strlen(text);
    ok = fwrite(text, 1u, length, file) == length && fputc('\n', file) != EOF;
    if (fclose(file) != 0) ok = 0;
    cJSON_free(text);
    return ok;
}

static cJSON* read_json(const char* path) {
    FILE* file = fopen(path, "rb");
    long size;
    char* text;
    cJSON* root;
    if (!file) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) { fclose(file); return NULL; }
    text = (char*)malloc((size_t)size + 1u);
    if (!text) { fclose(file); return NULL; }
    if (fread(text, 1u, (size_t)size, file) != (size_t)size) {
        free(text); fclose(file); return NULL;
    }
    text[size] = '\0';
    fclose(file);
    root = cJSON_Parse(text);
    free(text);
    return root;
}

int main(void) {
    Layout layout;
    Layout reloaded;
    LineDrawingSceneAuthoringState before;
    LineDrawingSceneAuthoringState after;
    cJSON* saved;
    cJSON* parsed;
    cJSON* reexported;
    char* saved_text;
    char* reexported_text;
    bool has_records = false;
    size_t index;
    uint64_t digest;
    uint64_t reloaded_digest;

    memset(&layout, 0, sizeof(layout));
    memset(&reloaded, 0, sizeof(reloaded));
    Layout_SceneAuthoringState_Init(&layout.sceneAuthoring);
    if (!Layout_SceneAuthoringState_AddDefaultCameraPath(&layout.sceneAuthoring, &index) ||
        !Layout_SceneAuthoringState_AddDefaultLight(&layout.sceneAuthoring, &index) ||
        !Layout_SceneAuthoringState_AddDefaultMaterial(&layout.sceneAuthoring, &index)) return 11;
    before = layout.sceneAuthoring;
    layout.sceneAuthoring.paths[0].closed = true;
    layout.sceneAuthoring.paths[0].playback_mode = LINE_DRAWING_SCENE_PATH_PLAYBACK_LOOP;
    layout.sceneAuthoring.paths[0].duration_seconds = 12.5f;
    layout.sceneAuthoring.paths[0].normalized_distance = 0.375f;
    layout.sceneAuthoring.paths[0].playing = true;
    layout.sceneAuthoring.materials[0].rgba[0] = 0.25f;
    layout.sceneAuthoring.materials[0].rgba[2] = 0.75f;
    after = layout.sceneAuthoring;
    digest = ld_stageg_authoring_digest(&layout.sceneAuthoring);
    ld_stageg_trace("b2_mutated", "authoring_records", digest);

    saved = export_authoring(&layout);
    if (!saved || !write_json("scene_authoring.json", saved)) return 12;
    saved_text = cJSON_PrintUnformatted(saved);
    if (!saved_text) return 13;
    ld_stageg_trace("b2_saved", "canonical_scene_authoring", digest);

    memset(&layout, 0, sizeof(layout));
    cJSON_Delete(saved);
    ld_stageg_trace("b2_destroyed", "source_state_cleared", 0u);

    parsed = read_json("scene_authoring.json");
    if (!parsed || !LineDrawingSceneAuthoringImport_ParseCanonical(
            parsed, &reloaded.sceneAuthoring, &has_records) || !has_records) return 14;
    reloaded_digest = ld_stageg_authoring_digest(&reloaded.sceneAuthoring);
    ld_stageg_trace("b2_reloaded", "production_import", reloaded_digest);

    reexported = export_authoring(&reloaded);
    reexported_text = reexported ? cJSON_PrintUnformatted(reexported) : NULL;
    if (!reexported_text || strcmp(saved_text, reexported_text) != 0) return 16;
    ld_stageg_trace("b2_compared", "exact_semantic_reexport", reloaded_digest);

    reloaded.sceneAuthoring = before;
    if (ld_stageg_authoring_digest(&reloaded.sceneAuthoring) == digest) return 17;
    ld_stageg_trace("b2_undo", "snapshot_before_mutation",
                    ld_stageg_authoring_digest(&reloaded.sceneAuthoring));
    reloaded.sceneAuthoring = after;
    if (ld_stageg_authoring_digest(&reloaded.sceneAuthoring) != digest) return 18;
    ld_stageg_trace("b2_redo", "snapshot_after_mutation", digest);

    if (!ld_stageg_write_state(&reloaded.sceneAuthoring,
                               "roundtrip.canonical", "line_drawing_bite2")) return 19;
    ld_stageg_trace("b2_canonical", "roundtrip_artifact", digest);

    free(saved_text);
    cJSON_free(reexported_text);
    cJSON_Delete(reexported);
    cJSON_Delete(parsed);
    ld_stageg_trace("b2_shutdown", "json_released", 0u);
    return 0;
}
