#include "ray_tracing_stage_g_common.h"
#include "app/scene_project_render_request.h"
#include "ray_tracing/ray_tracing_app_main.h"
#include "render/runtime_native_3d_async_render_bridge.h"

#include <stdlib.h>

typedef struct WorkflowState {
    RuntimeNative3DAsyncRenderProgressBuffer *progress;
    uint8_t host[6u * 4u * 4u];
    unsigned events;
    unsigned updates;
    unsigned routes;
    unsigned submits;
} WorkflowState;

static int parsed_argc;
static int defaults_loaded;
static int session_runs;

void AnimationParseArgs(int argc, char *argv[]) {
    (void)argv;
    parsed_argc = argc;
}

void AnimationLoadRuntimeDefaults(void) {
    defaults_loaded += 1;
}

int AnimationRunAppSession(void) {
    session_runs += 1;
    return 0;
}

static bool handle_events(void *user_data) {
    WorkflowState *state = (WorkflowState *)user_data;
    state->events += 1u;
    return true;
}

static bool update_frame(void *user_data) {
    WorkflowState *state = (WorkflowState *)user_data;
    state->updates += 1u;
    return true;
}

static bool route_frame(void *user_data) {
    WorkflowState *state = (WorkflowState *)user_data;
    state->routes += 1u;
    return true;
}

static bool submit_render(void *user_data) {
    WorkflowState *state = (WorkflowState *)user_data;
    RuntimeNative3DAsyncRenderProgressRect rect = {1, 1, 4, 2};
    state->submits += 1u;
    return RuntimeNative3DAsyncRenderProgressBuffer_PublishDirtyRectABGR(
        state->progress, 77u, state->host, 6, 4, rect);
}

int main(void) {
    const char *root = getenv("RAY_TRACING_STAGEG_PROJECT_ROOT");
    char runtime[PATH_MAX];
    char error[512];
    char *argv[] = {(char *)"ray_tracing", (char *)"--headless-stage-g"};
    RayTracingSceneProjectRenderRequest project;
    RayTracingSceneProjectRenderRequest reloaded;
    RuntimeNative3DRenderRequestSnapshot snapshot;
    RuntimeNative3DAsyncRenderAssessment assessment;
    RuntimeNative3DAsyncRenderProgressSnapshot progress_snapshot;
    RayTracingFrameEventsRequest events_request = {handle_events, NULL};
    RayTracingFrameEventsOutcome events_outcome;
    RayTracingFrameUpdateRequest update_request = {update_frame, NULL};
    RayTracingFrameUpdateOutcome update_outcome;
    RayTracingFrameRouteRequest route_request = {route_frame, NULL};
    RayTracingFrameRouteOutcome route_outcome;
    RayTracingRenderSubmitRequest submit_request = {submit_render, NULL};
    RayTracingRenderSubmitOutcome submit_outcome;
    WorkflowState state;
    uint8_t pixels[4u * 2u * 4u];
    size_t required = 0u;
    uint64_t digest;
    FILE *file;
    size_t i;

    if (!root) return 11;
    memset(&state, 0, sizeof(state));
    state.progress = RuntimeNative3DAsyncRenderProgressBuffer_Create();
    if (!state.progress) return 12;
    for (i = 0u; i < sizeof(state.host); ++i)
        state.host[i] = (uint8_t)((i * 29u + 11u) & 0xffu);
    events_request.user_data = &state;
    update_request.user_data = &state;
    route_request.user_data = &state;
    submit_request.user_data = &state;

    if (!ray_tracing_app_bootstrap() || !ray_tracing_app_config_load() ||
        !ray_tracing_app_state_seed() || !ray_tracing_app_subsystems_init() ||
        !ray_tracing_runtime_start() || parsed_argc != 0 || defaults_loaded != 1)
        return 13;
    rt_stageg_trace("b4_bootstrap", "runtime_started", 0u);

    snprintf(runtime, sizeof(runtime), "%s/scene_runtime.json", root);
    if (!ray_tracing_scene_project_render_request_resolve(runtime, NULL, &project,
                                                          error, sizeof(error)) ||
        !project.project_owned)
        return 14;
    rt_stageg_trace("b4_project", "request_resolved", 0u);
    if (!ray_tracing_scene_project_render_request_write(&project, 12, 6, 2,
                                                        error, sizeof(error)))
        return 15;
    rt_stageg_trace("b4_saved", "window_12_6_2", 0u);
    memset(&project, 0, sizeof(project));
    if (!ray_tracing_scene_project_render_request_resolve(runtime, NULL, &reloaded,
                                                          error, sizeof(error)) ||
        reloaded.simulation_start_frame != 12 || reloaded.simulation_frame_count != 6 ||
        reloaded.simulation_frame_stride != 2)
        return 16;
    rt_stageg_trace("b4_reloaded", "request_restored", 0u);

    if (!rt_stageg_build_snapshot(&snapshot)) return 17;
    assessment = RuntimeNative3DAsyncRender_AssessSnapshot(&snapshot);
    if (!assessment.ready) return 18;
    digest = rt_stageg_snapshot_digest(&snapshot);
    rt_stageg_trace("b4_snapshot", "async_ready", digest);

    if (!ray_tracing_app_frame_events(&events_request, &events_outcome) ||
        !events_outcome.accepted_by_wrapper || !events_outcome.handled)
        return 19;
    rt_stageg_trace("b4_events", "handled", digest);
    if (!ray_tracing_app_frame_update(&update_request, &update_outcome) ||
        !update_outcome.accepted_by_wrapper || !update_outcome.updated)
        return 20;
    rt_stageg_trace("b4_update", "frame_updated", digest);
    if (!ray_tracing_app_frame_route(&route_request, &route_outcome) ||
        !route_outcome.accepted_by_wrapper || !route_outcome.routed)
        return 21;
    rt_stageg_trace("b4_route", "frame_routed", digest);
    if (!ray_tracing_app_render_submit(&submit_request, &submit_outcome) ||
        !submit_outcome.accepted_by_wrapper || !submit_outcome.submitted)
        return 22;
    rt_stageg_trace("b4_submit", "render_published", digest);

    if (ray_tracing_app_run_loop() != 0 || session_runs != 1) return 23;
    rt_stageg_trace("b4_run_loop", "session_completed", digest);
    memset(pixels, 0, sizeof(pixels));
    if (!RuntimeNative3DAsyncRenderProgressBuffer_CopyLatest(
            state.progress, 77u, &progress_snapshot, pixels, sizeof(pixels), &required) ||
        required != sizeof(pixels) || progress_snapshot.sequence != 1u)
        return 24;
    digest = rt_stageg_hash_bytes(digest, pixels, sizeof(pixels));
    file = fopen("render.abgr", "wb");
    if (!file || fwrite(pixels, 1u, sizeof(pixels), file) != sizeof(pixels) ||
        fclose(file) != 0)
        return 25;
    file = fopen("workflow.canonical", "wb");
    if (!file) return 26;
    fprintf(file,
            "schema=ray_tracing_stage_g_workflow_v1\nwindow=12:6:2\n"
            "snapshot_generation=77\nreadiness=ready_exclusive_single_job\n"
            "events=%u\nupdates=%u\nroutes=%u\nsubmits=%u\nsession_runs=%d\n"
            "progress_sequence=%llu\nprogress_bytes=%zu\ndigest=%016llx\n",
            state.events, state.updates, state.routes, state.submits, session_runs,
            (unsigned long long)progress_snapshot.sequence, required,
            (unsigned long long)digest);
    if (fclose(file) != 0) return 27;
    rt_stageg_trace("b4_export", "state_and_pixels", digest);

    ray_tracing_app_shutdown();
    if (ray_tracing_app_render_submit(&submit_request, &submit_outcome) ||
        submit_outcome.accepted_by_wrapper)
        return 28;
    RuntimeNative3DAsyncRenderProgressBuffer_Destroy(state.progress);
    rt_stageg_trace("b4_shutdown", "post_shutdown_rejected", 0u);
    (void)argv;
    return 0;
}
