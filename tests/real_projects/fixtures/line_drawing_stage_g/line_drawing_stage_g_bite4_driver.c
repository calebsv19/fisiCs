#include "line_drawing_stage_g_common.h"

#include "Layout/layout.h"
#include "Layout/scene/layout_scene_path_edit.h"
#include "Layout/scene/layout_scene_path_traversal.h"
#include "Tools/canonical_scene_export_authoring.h"
#include "Tools/scene_authoring_import.h"
#include "Tools/scene_project_export.h"
#include "cjson/cJSON.h"

#include <stdlib.h>

static cJSON* workflow_export(const Layout* layout, const char* schema) {
    cJSON* root = cJSON_CreateObject();
    cJSON* materials = cJSON_CreateArray();
    cJSON* lights = cJSON_CreateArray();
    cJSON* cameras = cJSON_CreateArray();
    cJSON* paths = cJSON_CreateArray();
    if (!root || !materials || !lights || !cameras || !paths) return NULL;
    cJSON_AddStringToObject(root, "schema", schema);
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

static int workflow_write_json(const char* path, const cJSON* root) {
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

static cJSON* workflow_read_json(const char* path) {
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
    LineDrawingScenePath before_edit;
    LineDrawingScenePath after_edit;
    LineDrawingScenePathElementRef inserted;
    LineDrawingScenePathTraversalTable table;
    LineDrawingScenePathTraversalSample sample;
    LineDrawingSceneProjectExportOptions project_options = {
        .project_name = "stage_g_line_drawing",
        .created_by = "fisics_stage_g",
        .timestamp_utc = "2026-07-16T00:00:00Z",
        .authoring_scene = "scene_authoring.json",
        .runtime_scene = "scene_runtime.json",
        .objects = NULL,
        .object_count = 0u,
    };
    cJSON* authoring;
    cJSON* runtime;
    cJSON* parsed;
    bool has_records = false;
    char diagnostics[256];
    size_t index;
    uint64_t digest;
    uint64_t reloaded_digest;

    memset(&layout, 0, sizeof(layout));
    memset(&reloaded, 0, sizeof(reloaded));
    Layout_SceneAuthoringState_Init(&layout.sceneAuthoring);
    ld_stageg_trace("b4_bootstrap", "headless_state", 0u);

    if (!LineDrawingSceneProjectExport_WriteProjectFiles(
            ".", &project_options, diagnostics, sizeof(diagnostics))) return 11;
    ld_stageg_trace("b4_project", "scene_project_scaffold", 0u);

    if (!Layout_SceneAuthoringState_AddDefaultCameraPath(&layout.sceneAuthoring, &index) ||
        !Layout_SceneAuthoringState_AddDefaultLightPath(&layout.sceneAuthoring, &index) ||
        !Layout_SceneAuthoringState_AddDefaultLight(&layout.sceneAuthoring, &index) ||
        !Layout_SceneAuthoringState_AddDefaultMaterial(&layout.sceneAuthoring, &index)) return 12;
    if (!Layout_SceneAuthoringState_Select(&layout.sceneAuthoring,
            LINE_DRAWING_SCENE_AUTHORING_SELECTION_PATH, 0u)) return 13;
    digest = ld_stageg_authoring_digest(&layout.sceneAuthoring);
    ld_stageg_trace("b4_author", "scene_records", digest);

    before_edit = layout.sceneAuthoring.paths[0];
    if (!Layout_ScenePathEdit_SplitSegment(
            &layout.sceneAuthoring.paths[0], 0u, 0.5f, &inserted) ||
        !Layout_ScenePathEdit_SetElementWorldPoint(
            &layout.sceneAuthoring.paths[0], inserted, (Vec3){1.0f, 3.0f, -2.0f})) return 14;
    after_edit = layout.sceneAuthoring.paths[0];
    ld_stageg_trace("b4_edit", "split_and_move", ld_stageg_authoring_digest(&layout.sceneAuthoring));

    if (!Layout_ScenePathTraversal_Build(&layout.sceneAuthoring.paths[0], &table) ||
        !Layout_ScenePathTraversal_EvaluateNormalized(
            &table, 0.625f, LINE_DRAWING_SCENE_PATH_PLAYBACK_ONCE, &sample)) return 15;
    ld_stageg_trace("b4_traverse", "normalized_0625",
                    (uint64_t)(sample.distance * 1000.0f + 0.5f));

    layout.sceneAuthoring.paths[0] = before_edit;
    ld_stageg_trace("b4_undo", "snapshot_before_edit", ld_stageg_authoring_digest(&layout.sceneAuthoring));
    layout.sceneAuthoring.paths[0] = after_edit;
    digest = ld_stageg_authoring_digest(&layout.sceneAuthoring);
    ld_stageg_trace("b4_redo", "snapshot_after_edit", digest);

    authoring = workflow_export(&layout, "scene_authoring_v1");
    runtime = workflow_export(&layout, "scene_runtime_v1");
    if (!authoring || !runtime ||
        !workflow_write_json("scene_authoring.json", authoring) ||
        !workflow_write_json("scene_runtime.json", runtime)) return 16;
    ld_stageg_trace("b4_save", "authoring_and_runtime", digest);

    cJSON_Delete(authoring);
    cJSON_Delete(runtime);
    memset(&layout, 0, sizeof(layout));
    parsed = workflow_read_json("scene_authoring.json");
    if (!parsed || !LineDrawingSceneAuthoringImport_ParseCanonical(
            parsed, &reloaded.sceneAuthoring, &has_records) || !has_records) return 17;
    reloaded_digest = ld_stageg_authoring_digest(&reloaded.sceneAuthoring);
    ld_stageg_trace("b4_reload", "destroy_and_import", reloaded_digest);

    if (!ld_stageg_write_state(&reloaded.sceneAuthoring,
                               "workflow.canonical", "line_drawing_bite4")) return 18;
    ld_stageg_trace("b4_export", "project_artifacts_complete", reloaded_digest);

    cJSON_Delete(parsed);
    ld_stageg_trace("b4_shutdown", "workflow_complete", 0u);
    return 0;
}
