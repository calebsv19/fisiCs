#include "app/app_headless.h"
#include "app/app_internal.h"
#include "app/app_pins.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_schedule_count = 0;

void app_route_schedule_recompute(AppState *app, double debounce_sec) {
    (void)app;
    (void)debounce_sec;
    g_schedule_count += 1;
}

void app_playback_reset(AppState *app) {
    if (!app) return;
    app->route_state_bridge.playback_time_s = 0.0f;
    app->route_state_bridge.playback_playing = false;
    app_route_preview_reset(app);
}

void camera_set_heading_target(Camera *camera, float heading_rad) {
    if (camera) camera->heading_target_rad = heading_rad;
}

void route_state_clear(RouteState *state) {
    if (!state) return;
    route_path_free(&state->path);
    route_path_free(&state->drive_path);
    route_path_free(&state->walk_path);
    memset(state, 0, sizeof(*state));
}

static void checkpoint(const char *name,
                       const RoutePath *path,
                       const MapForgeHeadlessPlaybackSample *sample,
                       const char *detail) {
    int progress_milli = sample ? (int)lroundf(sample->progress * 1000.0f) : 0;
    int heading_milli = sample ? (int)lroundf(sample->heading_rad * 1000.0f) : 0;
    printf("TRACE|1|%s|nodes=%u|segment=%u,progress_milli=%d,heading_milli=%d|detail=%s\n",
           name,
           path ? path->count : 0u,
           sample ? sample->segment_index : 0u,
           progress_milli,
           heading_milli,
           detail ? detail : "ok");
}

static RouteEndpointAnchor anchor(uint32_t node, float x, float y) {
    RouteEndpointAnchor result;
    memset(&result, 0, sizeof(result));
    result.valid = true;
    result.node = node;
    result.world_x = x;
    result.world_y = y;
    return result;
}

static void seed_graph(RouteGraph *graph) {
    static double node_x[] = {0.0, 0.0, 10.0};
    static double node_y[] = {0.0, 10.0, 10.0};
    static uint32_t edge_start[] = {0u, 1u, 3u, 4u};
    static uint32_t edge_to[] = {1u, 0u, 2u, 1u};
    static float edge_length[] = {10.0f, 10.0f, 10.0f, 10.0f};
    static float edge_speed[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float edge_speed_limit[] = {10.0f, 10.0f, 10.0f, 10.0f};
    static float edge_grade[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static float edge_penalty[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static uint8_t edge_class[] = {
        ROAD_CLASS_RESIDENTIAL,
        ROAD_CLASS_RESIDENTIAL,
        ROAD_CLASS_RESIDENTIAL,
        ROAD_CLASS_RESIDENTIAL
    };
    memset(graph, 0, sizeof(*graph));
    graph->node_count = 3u;
    graph->edge_count = 4u;
    graph->node_x = node_x;
    graph->node_y = node_y;
    graph->edge_start = edge_start;
    graph->edge_to = edge_to;
    graph->edge_length = edge_length;
    graph->edge_speed = edge_speed;
    graph->edge_speed_limit = edge_speed_limit;
    graph->edge_grade = edge_grade;
    graph->edge_penalty = edge_penalty;
    graph->edge_class = edge_class;
}

static int sample_at(const RouteGraph *graph,
                     const RoutePath *path,
                     const MapForgeHeadlessPlaybackConfig *config,
                     float time_s,
                     MapForgeHeadlessPlaybackHeadingState *heading,
                     MapForgeHeadlessPlaybackSample *sample) {
    memset(sample, 0, sizeof(*sample));
    return map_forge_headless_playback_sample(graph, path, config, time_s, heading, sample) ? 1 : 0;
}

int main(void) {
    const char *pins_path = getenv("MAPFORGE_STAGEG_PINS_INPUT");
    MapForgePinsFile pins;
    AppState app;
    RouteEndpointAnchor start;
    RouteEndpointAnchor goal;
    MapForgeHeadlessPlaybackConfig config;
    MapForgeHeadlessPlaybackHeadingState heading;
    MapForgeHeadlessPlaybackSample sample;
    MapForgeHeadlessPlaybackSample preview_sample;
    RoutePath empty_path;
    FILE *canonical = NULL;
    char error[256];
    float duration = 0.0f;
    int fps = 0;
    uint32_t frames = 0u;

    memset(&app, 0, sizeof(app));
    memset(&config, 0, sizeof(config));
    memset(&empty_path, 0, sizeof(empty_path));
    map_forge_pins_file_init(&pins);
    if (!pins_path || !map_forge_pins_load(pins_path, &pins, error, sizeof(error)) ||
        !map_forge_pins_find_by_id_const(&pins, "demo_start") ||
        !map_forge_pins_find_by_id_const(&pins, "demo_goal")) return 11;
    seed_graph(&app.route_state_bridge.route.graph);
    start = anchor(0u, 0.0f, 0.0f);
    goal = anchor(2u, 10.0f, 10.0f);
    if (!app_route_service_set_endpoint_anchor(&app, true, &start, 0.0) ||
        !app_route_service_set_endpoint_anchor(&app, false, &goal, 0.0)) return 12;
    checkpoint("b2_bound", &app.route_state_bridge.route.path, NULL, "demo_start_to_demo_goal");

    if (!route_astar(&app.route_state_bridge.route.graph,
                     app.route_state_bridge.route.start_node,
                     app.route_state_bridge.route.goal_node,
                     ROUTE_OBJECTIVE_SHORTEST_DISTANCE,
                     ROUTE_MODE_CAR,
                     &app.route_state_bridge.route.path)) return 13;
    checkpoint("b2_routed", &app.route_state_bridge.route.path, NULL, "astar_forward");
    canonical = fopen("route.canonical", "wb");
    if (!canonical) return 14;
    fprintf(canonical, "forward=%u,%u,%u|time_milli=%d|length_milli=%d\n",
            app.route_state_bridge.route.path.nodes[0],
            app.route_state_bridge.route.path.nodes[1],
            app.route_state_bridge.route.path.nodes[2],
            (int)lroundf(app.route_state_bridge.route.path.total_time_s * 1000.0f),
            (int)lroundf(app.route_state_bridge.route.path.total_length_m * 1000.0f));

    config.has_duration_seconds = true;
    config.duration_seconds = 2.0f;
    config.has_fps = true;
    config.fps = 4;
    config.heading.mode = MAPFORGE_HEADLESS_HEADING_MODE_BLENDED;
    if (!map_forge_headless_playback_plan(&config, &app.route_state_bridge.route.path, &duration, &fps, &frames) ||
        fps != 4 || frames != 8u) return 15;
    map_forge_headless_playback_reset_heading_state(&heading);
    if (!sample_at(&app.route_state_bridge.route.graph, &app.route_state_bridge.route.path, &config, 0.0f, &heading, &sample)) return 16;
    checkpoint("b2_start", &app.route_state_bridge.route.path, &sample, "time_0");
    if (!sample_at(&app.route_state_bridge.route.graph, &app.route_state_bridge.route.path, &config, 10.0f, &heading, &sample)) return 17;
    checkpoint("b2_midpoint", &app.route_state_bridge.route.path, &sample, "segment_boundary");
    if (!sample_at(&app.route_state_bridge.route.graph, &app.route_state_bridge.route.path, &config, 10.1f, &heading, &sample) ||
        sample.heading_rad <= 0.0f || sample.heading_rad >= 1.3f) return 18;
    checkpoint("b2_turn", &app.route_state_bridge.route.path, &sample, "bounded_transition");

    app.route_state_bridge.playback_time_s = 5.0f;
    app.route_state_bridge.playback_playing = false;
    app.route_state_bridge.preview_follow_enabled = true;
    app_route_preview_update(&app);
    memset(&preview_sample, 0, sizeof(preview_sample));
    preview_sample.valid = app.route_state_bridge.preview.valid;
    preview_sample.segment_index = app.route_state_bridge.preview.segment_index;
    preview_sample.progress = app.route_state_bridge.preview.sample_time_s / app.route_state_bridge.route.path.total_time_s;
    preview_sample.heading_rad = app.route_state_bridge.preview.heading_rad;
    checkpoint("b2_paused", &app.route_state_bridge.route.path, &preview_sample, "follow_while_paused");

    app_route_preview_reset(&app);
    app.route_state_bridge.preview_follow_enabled = true;
    app.route_state_bridge.preview_heading_up = true;
    app.route_state_bridge.playback_time_s = 15.0f;
    app_route_preview_update(&app);
    if (!sample_at(&app.route_state_bridge.route.graph, &app.route_state_bridge.route.path, NULL, 15.0f, NULL, &sample) ||
        app.route_state_bridge.preview.segment_index != sample.segment_index ||
        fabsf(app.route_state_bridge.preview.world_x - (float)sample.world_x) > 0.001f ||
        fabsf(app.route_state_bridge.preview.world_y - (float)sample.world_y) > 0.001f) return 19;
    checkpoint("b2_heading_up", &app.route_state_bridge.route.path, &sample, "interactive_headless_match");
    app_route_preview_toggle_heading_mode(&app);
    if (fabsf(app.view_state_bridge.camera.heading_target_rad) > 0.001f) return 20;
    checkpoint("b2_north_up", &app.route_state_bridge.route.path, &sample, "heading_zero");

    route_path_free(&app.route_state_bridge.route.path);
    if (!app_route_service_set_endpoint_anchor(&app, true, &goal, 0.0) ||
        !app_route_service_set_endpoint_anchor(&app, false, &start, 0.0) ||
        !route_astar(&app.route_state_bridge.route.graph, 2u, 0u,
                     ROUTE_OBJECTIVE_SHORTEST_DISTANCE, ROUTE_MODE_CAR,
                     &app.route_state_bridge.route.path)) return 21;
    fprintf(canonical, "reverse=%u,%u,%u|schedules=%d\n",
            app.route_state_bridge.route.path.nodes[0],
            app.route_state_bridge.route.path.nodes[1],
            app.route_state_bridge.route.path.nodes[2],
            g_schedule_count);
    checkpoint("b2_reversed", &app.route_state_bridge.route.path, NULL, "astar_reverse");

    start.valid = false;
    if (app_route_service_set_endpoint_anchor(&app, true, &start, 0.0) ||
        route_astar(&app.route_state_bridge.route.graph, 9u, 0u,
                    ROUTE_OBJECTIVE_SHORTEST_DISTANCE, ROUTE_MODE_CAR, &empty_path) ||
        map_forge_headless_playback_sample(&app.route_state_bridge.route.graph,
                                           &empty_path, NULL, 0.0f, NULL, &sample)) return 22;
    checkpoint("b2_invalid", &app.route_state_bridge.route.path, NULL, "missing_endpoint_and_no_route");
    app_route_preview_reset(&app);
    map_forge_headless_playback_reset_heading_state(&heading);
    if (app.route_state_bridge.preview.valid || heading.valid) return 23;
    checkpoint("b2_reset", &app.route_state_bridge.route.path, NULL, "preview_and_heading_reset");
    if (fclose(canonical) != 0) return 24;

    route_path_free(&app.route_state_bridge.route.path);
    map_forge_pins_file_free(&pins);
    checkpoint("b2_shutdown", NULL, NULL, "path_and_pins_freed");
    return 0;
}
