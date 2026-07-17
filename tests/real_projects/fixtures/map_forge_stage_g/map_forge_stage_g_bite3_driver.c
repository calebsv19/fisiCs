#include "app/app_headless.h"
#include "app/app_internal.h"
#include "app/app_map_viewport_internal.h"
#include "app/headless/app_headless_run_internal.h"
#include "camera/camera_viewport_bridge.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void checkpoint(const char *name, int a, int b, const char *detail) {
    printf("TRACE|1|%s|a=%d|b=%d|detail=%s\n", name, a, b, detail ? detail : "ok");
}

static void seed_route(RouteState *route) {
    static double node_x[] = {0.0, 0.0, 10.0};
    static double node_y[] = {0.0, 10.0, 10.0};
    static uint32_t nodes[] = {0u, 1u, 2u};
    static float cumulative_length[] = {0.0f, 10.0f, 20.0f};
    static float cumulative_time[] = {0.0f, 10.0f, 20.0f};
    memset(route, 0, sizeof(*route));
    route->graph.node_count = 3u;
    route->graph.node_x = node_x;
    route->graph.node_y = node_y;
    route->path.nodes = nodes;
    route->path.count = 3u;
    route->path.cumulative_length_m = cumulative_length;
    route->path.cumulative_time_s = cumulative_time;
    route->path.total_length_m = 20.0f;
    route->path.total_time_s = 20.0f;
    route->objective = ROUTE_OBJECTIVE_SHORTEST_DISTANCE;
}

static int write_canonical(const MapForgeHeadlessRunResult *result) {
    FILE *file = fopen("headless.canonical", "wb");
    if (!file || !result) return 0;
    fprintf(file,
            "region=%s|size=%dx%d|fps=%d|frames=%u|route_nodes=%u|preview=%d|frames_written=%u\n",
            result->job.map_region,
            result->job.camera.width,
            result->job.camera.height,
            result->playback_fps,
            result->estimated_frame_count,
            result->route_state.path.count,
            result->preview_written ? 1 : 0,
            result->frames_written_count);
    return fclose(file) == 0;
}

int main(void) {
    AppState app;
    MapForgeHeadlessRunResult result;
    MapForgeHeadlessPlaybackHeadingState heading;
    MapForgeHeadlessPlaybackSample preview;
    MapForgeHeadlessPlaybackSample frames[2];
    MapForgeHeadlessRenderPin from_pin;
    MapForgeHeadlessRenderPin to_pin;
    MapForgePin from_model;
    MapForgePin to_model;
    float screen_x = 0.0f;
    float screen_y = 0.0f;
    float world_x = 0.0f;
    float world_y = 0.0f;

    memset(&app, 0, sizeof(app));
    app.width = 320;
    app.height = 200;
    app.ui_state_bridge.map_viewport_rect = (SDL_FRect){10.0f, 20.0f, 300.0f, 160.0f};
    camera_init(&app.view_state_bridge.camera);
    app.view_state_bridge.camera.x = app.view_state_bridge.camera.x_target = 5.0f;
    app.view_state_bridge.camera.y = app.view_state_bridge.camera.y_target = 5.0f;
    app.view_state_bridge.camera.zoom = app.view_state_bridge.camera.zoom_target = 18.0f;
    checkpoint("b3_viewport", 300, 160, "fixed_rect");
    if (!app_map_world_to_screen(&app, 5.0f, 5.0f, &screen_x, &screen_y) ||
        !app_map_screen_to_world(&app, screen_x, screen_y, &world_x, &world_y) ||
        fabsf(world_x - 5.0f) > 0.01f || fabsf(world_y - 5.0f) > 0.01f) return 11;
    checkpoint("b3_roundtrip", (int)lroundf(screen_x * 100.0f), (int)lroundf(screen_y * 100.0f), "world_screen_world");
    if (camera_viewport_bridge_zoom_target_at_anchor(&app.view_state_bridge.camera,
                                                     160.0f, 80.0f, 300, 160, 18.5f).code != CORE_OK) return 12;
    checkpoint("b3_zoom", (int)lroundf(app.view_state_bridge.camera.zoom_target * 1000.0f), 0, "anchor_zoom");
    if (camera_viewport_bridge_pan_target_by_screen_delta(&app.view_state_bridge.camera,
                                                          12.0f, -8.0f, 300, 160).code != CORE_OK) return 12;
    checkpoint("b3_pan", (int)lroundf(app.view_state_bridge.camera.x_target), (int)lroundf(app.view_state_bridge.camera.y_target), "screen_delta");
    if (!renderer_resize(&app.renderer, 640, 360)) return 12;
    checkpoint("b3_resize", app.renderer.width, app.renderer.height, "bounded_resize");
    camera_set_heading_target(&app.view_state_bridge.camera, 0.75f);
    checkpoint("b3_heading_up", (int)lroundf(app.view_state_bridge.camera.heading_target_rad * 1000.0f), 1, "heading_target");
    camera_set_heading_target(&app.view_state_bridge.camera, 0.0f);
    checkpoint("b3_north_up", 0, 0, "heading_zero");

    memset(&result, 0, sizeof(result));
    memset(&from_pin, 0, sizeof(from_pin));
    memset(&to_pin, 0, sizeof(to_pin));
    memset(&from_model, 0, sizeof(from_model));
    memset(&to_model, 0, sizeof(to_model));
    seed_route(&result.route_state);
    result.ok = true;
    result.job_loaded = true;
    result.route_computed = true;
    result.playback_trace_written = true;
    snprintf(result.status, sizeof(result.status), "complete");
    snprintf(result.out_dir, sizeof(result.out_dir), ".");
    snprintf(result.canonical_job_request_path, sizeof(result.canonical_job_request_path), "job.request.canonical.json");
    snprintf(result.command, sizeof(result.command), "mapforge-stage-g --bite 3\n");
    snprintf(result.timestamp_utc, sizeof(result.timestamp_utc), "2000-01-01T00:00:00Z");
    snprintf(result.git_commit, sizeof(result.git_commit), "stage-g-fixture");
    result.job.version = 1u;
    snprintf(result.job.type, sizeof(result.job.type), "mapforge_route_playback");
    snprintf(result.job.map_region, sizeof(result.job.map_region), "seattle");
    snprintf(result.job.from_pin, sizeof(result.job.from_pin), "demo_start");
    snprintf(result.job.to_pin, sizeof(result.job.to_pin), "demo_goal");
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
    result.job.output.quality_profile = MAPFORGE_HEADLESS_QUALITY_PROFILE_RUNTIME;
    snprintf(result.job.output.frame_format, sizeof(result.job.output.frame_format), "bmp");
    result.playback_duration_s = 1.0f;
    result.playback_fps = 2;
    result.estimated_frame_count = 2u;
    map_forge_headless_playback_reset_heading_state(&heading);
    if (!map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 10.0f, &heading, &preview) ||
        !map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 0.0f, NULL, &frames[0]) ||
        !map_forge_headless_playback_sample(&result.route_state.graph, &result.route_state.path,
                                            &result.job.playback, 20.0f, NULL, &frames[1])) return 13;
    checkpoint("b3_job", result.job.camera.width, result.job.camera.height, "small_headless_job");
    checkpoint("b3_trace", result.estimated_frame_count, (int)lroundf(preview.progress * 1000.0f), "sampled");

    from_pin.valid = true; from_pin.nearest_node = 0u; from_pin.world_x = 0.0; from_pin.world_y = 0.0;
    to_pin.valid = true; to_pin.nearest_node = 2u; to_pin.world_x = 10.0; to_pin.world_y = 10.0;
    if (!map_forge_headless_render_route_images(".", &result.job, &result.region,
                                                &from_pin, &to_pin, &result.route_state,
                                                &preview, frames, 2u, &result.image_exports)) return 14;
    result.preview_written = result.image_exports.preview_written;
    result.frames_written = result.image_exports.frames_written;
    result.frames_written_count = result.image_exports.frames_written_count;
    checkpoint("b3_preview", result.preview_written ? 1 : 0, 160 * 90, "bmp_written");
    checkpoint("b3_frames", (int)result.frames_written_count, 2, "declared_frame_set");

    snprintf(from_model.id, sizeof(from_model.id), "demo_start");
    snprintf(to_model.id, sizeof(to_model.id), "demo_goal");
    result.from_pin.pin = &from_model;
    result.from_pin.nearest_node = 0u;
    result.from_pin.world_x = 0.0;
    result.from_pin.world_y = 0.0;
    result.to_pin.pin = &to_model;
    result.to_pin.nearest_node = 2u;
    result.to_pin.world_x = 10.0;
    result.to_pin.world_y = 10.0;
    map_forge_headless_record_job_warnings(&result);
    if (!map_forge_headless_write_outputs(&result) || !write_canonical(&result)) return 15;

    snprintf(result.job.output.frame_format, sizeof(result.job.output.frame_format), "png");
    if (map_forge_headless_render_route_images(".", &result.job, &result.region,
                                               &from_pin, &to_pin, &result.route_state,
                                               &preview, frames, 2u, &result.image_exports) ||
        map_forge_headless_render_route_images(".", &result.job, NULL,
                                               &from_pin, &to_pin, &result.route_state,
                                               &preview, frames, 2u, &result.image_exports) ||
        app_map_screen_to_world(&app, -1.0f, -1.0f, &world_x, &world_y)) return 16;
    checkpoint("b3_invalid", 3, 3, "format_region_bounds_rejected");
    renderer_shutdown(&app.renderer);
    checkpoint("b3_shutdown", 0, 0, "render_state_closed");
    return 0;
}
