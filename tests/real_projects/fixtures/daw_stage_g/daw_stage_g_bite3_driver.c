#include "daw_stage_g_common.h"

#include "app_state.h"
#include "input/timeline/timeline_geometry.h"
#include "ui/panes.h"
#include "ui/timeline_view.h"

#include <math.h>

void daw_stage_g_geometry_seed(Pane pane);

static void trace_geom(const char* checkpoint, const TimelineGeometry* g) {
    char fields[256];
    snprintf(fields, sizeof(fields),
             "left=%d|width=%d|top=%d|track_h=%d|visible=%.2f|start=%.2f|pps=%.2f",
             g->content_left, g->content_width, g->track_top, g->track_height,
             g->visible_seconds, g->window_start_seconds, g->pixels_per_second);
    daw_g_trace(checkpoint, fields);
}

int main(void) {
    AppState state;
    TimelineGeometry geom;
    Pane pane = {.rect = {40, 100, 1200, 500}, .visible = true};
    char fields[256], canonical[512];
    memset(&state, 0, sizeof(state));
    state.engine = (Engine*)(uintptr_t)1;
    state.timeline_visible_seconds = 10.0f;
    state.timeline_window_start_seconds = 0.0f;
    state.timeline_vertical_scale = 1.0f;
    daw_stage_g_geometry_seed(pane);

    daw_g_expect(timeline_compute_geometry(&state, &pane, &geom), "initial geometry");
    trace_geom("b3_begin", &geom);
    int x_at_five = timeline_seconds_to_x(&geom, 5.0f);
    float seconds_back = timeline_x_to_seconds(&geom, x_at_five);
    daw_g_expect(fabsf(seconds_back - 5.0f) < 0.02f, "coordinate round trip");
    snprintf(fields, sizeof(fields), "x_at_5=%d|seconds_back=%.2f|track0=%d",
             x_at_five, seconds_back,
             timeline_track_at_position(&state, geom.track_top + 10, geom.track_height, geom.track_spacing));
    daw_g_trace("b3_coordinate", fields);

    state.timeline_window_start_seconds = 8.0f;
    daw_g_expect(timeline_compute_geometry(&state, &pane, &geom), "forward move");
    trace_geom("b3_move_forward", &geom);
    state.timeline_window_start_seconds = -99.0f;
    daw_g_expect(timeline_compute_geometry(&state, &pane, &geom), "reverse move");
    daw_g_expect(geom.window_start_seconds == -5.0f, "reverse clamp");
    trace_geom("b3_move_reverse", &geom);

    state.timeline_visible_seconds = 0.01f;
    state.timeline_vertical_scale = 99.0f;
    daw_g_expect(timeline_compute_geometry(&state, &pane, &geom), "resize clamp");
    daw_g_expect(geom.visible_seconds == TIMELINE_MIN_VISIBLE_SECONDS, "visible clamp");
    daw_g_expect(geom.track_height == (int)(TIMELINE_BASE_TRACK_HEIGHT * TIMELINE_MAX_VERTICAL_SCALE),
                 "vertical clamp");
    trace_geom("b3_resize_clamp", &geom);

    Pane narrow = {.rect = {0, 0, 100, 100}, .visible = true};
    daw_g_expect(!timeline_compute_geometry(&state, &narrow, &geom), "reject narrow pane");
    daw_g_expect(timeline_seconds_to_x(NULL, 4.0f) == 0, "reject null geometry");
    daw_g_trace("b3_invalid", "narrow=0|null_seconds_to_x=0");

    snprintf(canonical, sizeof(canonical),
             "version=1\ninitial_x_at_5=%d\nroundtrip=%.2f\nreverse_clamp=-5.00\nvisible_clamp=%.2f\ntrack_height=%d\n",
             x_at_five, seconds_back, TIMELINE_MIN_VISIBLE_SECONDS,
             (int)(TIMELINE_BASE_TRACK_HEIGHT * TIMELINE_MAX_VERTICAL_SCALE));
    daw_g_write_text("interaction.canonical", canonical);
    daw_g_trace("b3_canonical", "artifact=interaction.canonical|version=1");
    daw_g_trace("b3_shutdown", "interaction_active=0|state_released=1");
    return 0;
}
