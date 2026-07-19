#include "app_state.h"
#include "engine/engine.h"
#include "ui/layout.h"
#include "ui/panes.h"

static Pane g_timeline;
static EngineRuntimeConfig g_config = {.sample_rate = 48000, .block_size = 256};
static EngineClip g_clips[2];
static EngineTrack g_track;

void daw_stage_g_geometry_seed(Pane pane) {
    g_timeline = pane;
    g_clips[0].timeline_start_frames = 0;
    g_clips[0].duration_frames = 480000;
    g_clips[1].timeline_start_frames = 720000;
    g_clips[1].duration_frames = 240000;
    g_track.clips = g_clips;
    g_track.clip_count = 2;
}

const EngineRuntimeConfig* engine_get_config(const Engine* engine) {
    return engine ? &g_config : NULL;
}

const EngineTrack* engine_get_tracks(const Engine* engine) {
    return engine ? &g_track : NULL;
}

int engine_get_track_count(const Engine* engine) {
    return engine ? 1 : 0;
}

uint64_t engine_clip_get_total_frames(const Engine* engine, int track_index, int clip_index) {
    (void)engine; (void)track_index;
    return clip_index >= 0 && clip_index < 2 ? g_clips[clip_index].duration_frames : 0;
}

const Pane* ui_layout_get_pane(const AppState* state, int index) {
    return state && index == 1 ? &g_timeline : NULL;
}

int timeline_view_controls_height_for_width(int timeline_width) {
    return timeline_width < 700 ? 96 : 64;
}

int timeline_view_ruler_height(void) { return 28; }
int timeline_view_track_header_width(void) { return 184; }
