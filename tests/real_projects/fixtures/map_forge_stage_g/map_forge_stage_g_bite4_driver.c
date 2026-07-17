#include "app/app_headless.h"
#include "app/app_internal.h"
#include "app/app_pins.h"
#include "app/headless/app_headless_run_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void app_route_schedule_recompute(AppState *app, double debounce_sec) {
    (void)app; (void)debounce_sec;
}

void app_playback_reset(AppState *app) {
    if (!app) return;
    app->route_state_bridge.playback_time_s = 0.0f;
    app->route_state_bridge.playback_playing = false;
}

void route_state_clear(RouteState *state) {
    if (!state) return;
    route_path_free(&state->path);
    route_path_free(&state->drive_path);
    route_path_free(&state->walk_path);
    memset(state, 0, sizeof(*state));
}

static void checkpoint(const char *name, unsigned a, unsigned b, const char *detail) {
    printf("TRACE|1|%s|a=%u|b=%u|detail=%s\n", name, a, b, detail ? detail : "ok");
}

static RouteEndpointAnchor make_anchor(uint32_t node, float x, float y) {
    RouteEndpointAnchor anchor;
    memset(&anchor, 0, sizeof(anchor));
    anchor.valid = true;
    anchor.node = node;
    anchor.world_x = x;
    anchor.world_y = y;
    return anchor;
}

static void seed_graph(RouteGraph *graph) {
    static double node_x[] = {0.0, 0.0, 10.0};
    static double node_y[] = {0.0, 10.0, 10.0};
    static uint32_t edge_start[] = {0u, 1u, 3u, 4u};
    static uint32_t edge_to[] = {1u, 0u, 2u, 1u};
    static float edge_length[] = {10.0f, 10.0f, 10.0f, 10.0f};
    static float edge_speed[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float edge_limit[] = {10.0f, 10.0f, 10.0f, 10.0f};
    static float edge_zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static uint8_t edge_class[] = {5u, 5u, 5u, 5u};
    memset(graph, 0, sizeof(*graph));
    graph->node_count = 3u;
    graph->edge_count = 4u;
    graph->node_x = node_x;
    graph->node_y = node_y;
    graph->edge_start = edge_start;
    graph->edge_to = edge_to;
    graph->edge_length = edge_length;
    graph->edge_speed = edge_speed;
    graph->edge_speed_limit = edge_limit;
    graph->edge_grade = edge_zero;
    graph->edge_penalty = edge_zero;
    graph->edge_class = edge_class;
}

static int write_workflow_canonical(const MapForgeHeadlessRunResult *result,
                                    const MapForgePinsFile *pins) {
    FILE *file = fopen("workflow.canonical", "wb");
    uint32_t i = 0;
    if (!file || !result || !pins) return 0;
    fprintf(file, "region=%s|pins=%zu|route=", result->job.map_region, pins->pin_count);
    for (i = 0; i < result->route_state.path.count; ++i) {
        fprintf(file, "%s%u", i ? "," : "", result->route_state.path.nodes[i]);
    }
    fprintf(file, "|frames=%u|preview=%d\n",
            result->frames_written_count,
            result->preview_written ? 1 : 0);
    return fclose(file) == 0;
}

int main(void) {
    const char *pins_path = getenv("MAPFORGE_STAGEG_PINS_INPUT");
    MapForgePinsFile pins;
    AppState app;
    MapForgeHeadlessRunResult result;
    MapForgeHeadlessPlaybackSample preview;
    MapForgeHeadlessPlaybackSample frames[2];
    MapForgeHeadlessRenderPin from_render;
    MapForgeHeadlessRenderPin to_render;
    RouteEndpointAnchor start;
    RouteEndpointAnchor goal;
    const MapForgePin *from_pin = NULL;
    const MapForgePin *to_pin = NULL;
    char error[256];

    memset(&app, 0, sizeof(app));
    memset(&result, 0, sizeof(result));
    memset(&from_render, 0, sizeof(from_render));
    memset(&to_render, 0, sizeof(to_render));
    map_forge_pins_file_init(&pins);
    camera_init(&app.view_state_bridge.camera);
    checkpoint("b4_bootstrap", 1u, 1u, "state_initialized");
    result.region.name = "seattle";
    checkpoint("b4_region", 1u, 0u, "seattle");
    if (!pins_path || !map_forge_pins_load(pins_path, &pins, error, sizeof(error))) return 11;
    checkpoint("b4_pins", (unsigned)pins.pin_count, 0u, "loaded");
    from_pin = map_forge_pins_find_by_id_const(&pins, "demo_start");
    to_pin = map_forge_pins_find_by_id_const(&pins, "demo_goal");
    if (!from_pin || !to_pin) return 12;
    seed_graph(&app.route_state_bridge.route.graph);
    start = make_anchor(0u, 0.0f, 0.0f);
    goal = make_anchor(2u, 10.0f, 10.0f);
    if (!app_route_service_set_endpoint_anchor(&app, true, &start, 0.0) ||
        !app_route_service_set_endpoint_anchor(&app, false, &goal, 0.0)) return 13;
    checkpoint("b4_endpoints", app.route_state_bridge.route.start_node,
               app.route_state_bridge.route.goal_node, "bound_from_saved_pins");
    if (!route_astar(&app.route_state_bridge.route.graph, 0u, 2u,
                     ROUTE_OBJECTIVE_SHORTEST_DISTANCE, ROUTE_MODE_CAR,
                     &app.route_state_bridge.route.path)) return 14;
    checkpoint("b4_route", app.route_state_bridge.route.path.count,
               (unsigned)app.route_state_bridge.route.path.total_time_s, "computed");

    app.width = 160;
    app.height = 90;
    app.ui_state_bridge.map_viewport_rect = (SDL_FRect){0.0f, 0.0f, 160.0f, 90.0f};
    app.view_state_bridge.camera.x = app.view_state_bridge.camera.x_target = 5.0f;
    app.view_state_bridge.camera.y = app.view_state_bridge.camera.y_target = 5.0f;
    app.view_state_bridge.camera.zoom = app.view_state_bridge.camera.zoom_target = 18.0f;
    checkpoint("b4_viewport", 160u, 90u, "camera_ready");

    result.route_state = app.route_state_bridge.route;
    result.ok = true;
    result.job_loaded = true;
    result.route_computed = true;
    result.playback_trace_written = true;
    snprintf(result.status, sizeof(result.status), "complete");
    snprintf(result.out_dir, sizeof(result.out_dir), ".");
    snprintf(result.canonical_job_request_path, sizeof(result.canonical_job_request_path), "workflow.job.json");
    snprintf(result.command, sizeof(result.command), "mapforge-stage-g --bite 4\n");
    snprintf(result.timestamp_utc, sizeof(result.timestamp_utc), "2000-01-01T00:00:00Z");
    snprintf(result.git_commit, sizeof(result.git_commit), "stage-g-fixture");
    result.job.version = 1u;
    snprintf(result.job.type, sizeof(result.job.type), "mapforge_route_playback");
    snprintf(result.job.map_region, sizeof(result.job.map_region), "seattle");
    snprintf(result.job.from_pin, sizeof(result.job.from_pin), "%s", from_pin->id);
    snprintf(result.job.to_pin, sizeof(result.job.to_pin), "%s", to_pin->id);
    result.job.route_mode = ROUTE_MODE_CAR;
    result.job.camera.width = 160;
    result.job.camera.height = 90;
    result.job.camera.has_width = true;
    result.job.camera.has_height = true;
    result.job.camera.follow_route = true;
    result.job.camera.rotate_with_heading = true;
    result.job.playback.duration_seconds = 1.0f;
    result.job.playback.fps = 2;
    result.job.playback.has_duration_seconds = true;
    result.job.playback.has_fps = true;
    result.job.output.preview_png = true;
    result.job.output.frames = true;
    result.job.output.render_mode = MAPFORGE_HEADLESS_RENDER_MODE_MAP_ROUTE_MARKER;
    result.job.output.pixel_scale = 1;
    snprintf(result.job.output.frame_format, sizeof(result.job.output.frame_format), "bmp");
    result.playback_duration_s = 1.0f;
    result.playback_fps = 2;
    result.estimated_frame_count = 2u;
    if (!map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 10.0f, NULL, &preview) ||
        !map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 0.0f, NULL, &frames[0]) ||
        !map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 20.0f, NULL, &frames[1])) return 15;
    checkpoint("b4_playback", 2u, (unsigned)(preview.progress * 1000.0f), "sampled");
    camera_set_heading_target(&app.view_state_bridge.camera, preview.heading_rad);
    camera_set_heading_target(&app.view_state_bridge.camera, 0.0f);
    checkpoint("b4_orientation", 1u, 0u, "heading_up_then_north_up");

    from_render.valid = true; from_render.nearest_node = 0u;
    to_render.valid = true; to_render.nearest_node = 2u;
    to_render.world_x = 10.0; to_render.world_y = 10.0;
    if (!map_forge_headless_render_route_images(".", &result.job, &result.region,
                                                &from_render, &to_render, &result.route_state,
                                                &preview, frames, 2u, &result.image_exports)) return 16;
    result.preview_written = result.image_exports.preview_written;
    result.frames_written = result.image_exports.frames_written;
    result.frames_written_count = result.image_exports.frames_written_count;
    checkpoint("b4_exported", result.preview_written ? 1u : 0u,
               result.frames_written_count, "preview_and_frames");

    result.from_pin.pin = from_pin;
    result.from_pin.nearest_node = 0u;
    result.to_pin.pin = to_pin;
    result.to_pin.nearest_node = 2u;
    result.to_pin.world_x = 10.0;
    result.to_pin.world_y = 10.0;
    map_forge_headless_record_job_warnings(&result);
    if (!map_forge_headless_write_outputs(&result) ||
        !map_forge_pins_save("workflow.pins.json", &pins, error, sizeof(error)) ||
        !write_workflow_canonical(&result, &pins)) return 17;
    checkpoint("b4_canonicalized", result.route_state.path.count,
               (unsigned)pins.pin_count, "outputs_saved");

    route_state_clear(&app.route_state_bridge.route);
    result.route_state.path.nodes = NULL;
    result.route_state.path.cumulative_length_m = NULL;
    result.route_state.path.cumulative_time_s = NULL;
    map_forge_pins_file_free(&pins);
    checkpoint("b4_destroyed", 0u, 0u, "route_and_pins_destroyed");
    renderer_shutdown(&app.renderer);
    checkpoint("b4_shutdown", 0u, 0u, "complete");
    return 0;
}
