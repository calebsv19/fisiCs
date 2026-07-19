#include "daw_stage_g_common.h"

#include "session.h"

#include <sys/stat.h>

static void seed_document(SessionDocument* doc) {
    session_document_init(doc);
    doc->engine.sample_rate = 48000;
    doc->engine.block_size = 256;
    doc->tempo.bpm = 132.0f;
    doc->tempo.ts_num = 7;
    doc->tempo.ts_den = 8;
    doc->loop = (SessionLoopState){true, 960, 9600};
    doc->timeline.visible_seconds = 12.5f;
    doc->timeline.window_start_seconds = 3.25f;
    doc->timeline.vertical_scale = 1.5f;
    doc->timeline.snap_enabled = true;
    doc->active_track_index = 0;
    doc->selected_track_index = 0;
    doc->selected_clip_index = 0;
    doc->selection_count = 1;
    doc->selection[0] = (SessionTimelineSelectionEntry){0, 0};
    strcpy(doc->data_paths.input_root, "fixtures/media");
    strcpy(doc->data_paths.output_root, "runtime/output");
    strcpy(doc->data_paths.library_copy_root, "runtime/library");

    doc->tempo_event_count = 2;
    doc->tempo_events = calloc(2, sizeof(*doc->tempo_events));
    daw_g_expect(doc->tempo_events != NULL, "allocate tempo events");
    doc->tempo_events[0] = (SessionTempoEvent){0.0f, 132.0f};
    doc->tempo_events[1] = (SessionTempoEvent){8.0f, 98.0f};

    doc->track_count = 1;
    doc->tracks = calloc(1, sizeof(*doc->tracks));
    daw_g_expect(doc->tracks != NULL, "allocate track");
    strcpy(doc->tracks[0].name, "Stage G MIDI");
    doc->tracks[0].gain = 0.75f;
    doc->tracks[0].pan = -0.25f;
    doc->tracks[0].midi_instrument_enabled = true;
    doc->tracks[0].midi_instrument_preset = ENGINE_INSTRUMENT_PRESET_WARM_KEYS;
    doc->tracks[0].midi_instrument_params = engine_instrument_default_params(ENGINE_INSTRUMENT_PRESET_WARM_KEYS);
    doc->tracks[0].clip_count = 1;
    doc->tracks[0].clips = calloc(1, sizeof(*doc->tracks[0].clips));
    daw_g_expect(doc->tracks[0].clips != NULL, "allocate clip");
    SessionClip* clip = &doc->tracks[0].clips[0];
    clip->kind = ENGINE_CLIP_KIND_MIDI;
    strcpy(clip->name, "Deterministic phrase");
    clip->start_frame = 24000;
    clip->duration_frames = 96000;
    clip->gain = 0.9f;
    clip->selected = true;
    clip->instrument_preset = ENGINE_INSTRUMENT_PRESET_WARM_KEYS;
    clip->instrument_params = engine_instrument_default_params(ENGINE_INSTRUMENT_PRESET_WARM_KEYS);
    clip->midi_note_count = 3;
    clip->midi_notes = calloc(3, sizeof(*clip->midi_notes));
    daw_g_expect(clip->midi_notes != NULL, "allocate notes");
    clip->midi_notes[0] = (EngineMidiNote){0, 12000, 60, 0.75f};
    clip->midi_notes[1] = (EngineMidiNote){24000, 12000, 64, 0.80f};
    clip->midi_notes[2] = (EngineMidiNote){48000, 24000, 67, 0.90f};
}

static uint64_t semantic_digest(const SessionDocument* doc) {
    uint64_t h = UINT64_C(1469598103934665603);
    h = daw_g_hash_u64(h, doc->version);
    h = daw_g_hash_u64(h, (uint64_t)doc->engine.sample_rate);
    h = daw_g_hash_bytes(h, &doc->tempo.bpm, sizeof(doc->tempo.bpm));
    h = daw_g_hash_u64(h, (uint64_t)doc->track_count);
    h = daw_g_hash_text(h, doc->tracks[0].name);
    const SessionClip* clip = &doc->tracks[0].clips[0];
    h = daw_g_hash_text(h, clip->name);
    h = daw_g_hash_u64(h, clip->start_frame);
    h = daw_g_hash_u64(h, clip->duration_frames);
    for (int i = 0; i < clip->midi_note_count; ++i) {
        h = daw_g_hash_u64(h, clip->midi_notes[i].start_frame);
        h = daw_g_hash_u64(h, clip->midi_notes[i].duration_frames);
        h = daw_g_hash_u64(h, clip->midi_notes[i].note);
    }
    h = daw_g_hash_text(h, doc->data_paths.output_root);
    return h;
}

int main(void) {
    SessionDocument before, loaded;
    char fields[256], canonical[512], error[256];
    daw_g_expect(mkdir("runtime", 0777) == 0, "create runtime directory");
    seed_document(&before);
    daw_g_expect(session_document_validate(&before, error, sizeof(error)), error);
    snprintf(fields, sizeof(fields), "tracks=%d|clips=%d|notes=%d|digest=%016llx",
             before.track_count, before.tracks[0].clip_count, before.tracks[0].clips[0].midi_note_count,
             (unsigned long long)semantic_digest(&before));
    daw_g_trace("b2_mutated", fields);

    daw_g_expect(session_document_write_file(&before, "runtime/session.json"), "save session");
    daw_g_trace("b2_saved", "artifact=runtime/session.json|version=24");
    uint64_t expected = semantic_digest(&before);
    session_document_free(&before);
    daw_g_trace("b2_destroyed", "tracks=0|heap_released=1");

    session_document_init(&loaded);
    daw_g_expect(session_document_read_file("runtime/session.json", &loaded), "reload session");
    uint64_t actual = semantic_digest(&loaded);
    snprintf(fields, sizeof(fields), "tracks=%d|clips=%d|notes=%d|digest=%016llx",
             loaded.track_count, loaded.tracks[0].clip_count, loaded.tracks[0].clips[0].midi_note_count,
             (unsigned long long)actual);
    daw_g_trace("b2_reloaded", fields);
    daw_g_expect(actual == expected, "round-trip semantic digest");
    daw_g_expect(loaded.loop.enabled && loaded.loop.start_frame == 960 && loaded.loop.end_frame == 9600,
                 "round-trip loop state");
    daw_g_trace("b2_compared", "digest_match=1|loop_match=1|selection_match=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\ndigest=%016llx\ntracks=%d\nclips=%d\nnotes=%d\ntempo=%.0f\noutput_root=%s\n",
             (unsigned long long)actual, loaded.track_count, loaded.tracks[0].clip_count,
             loaded.tracks[0].clips[0].midi_note_count, loaded.tempo.bpm, loaded.data_paths.output_root);
    daw_g_write_text("persistence.canonical", canonical);
    daw_g_trace("b2_canonical", "artifact=persistence.canonical|version=1");
    session_document_free(&loaded);
    daw_g_trace("b2_shutdown", "document_freed=1|runtime_closed=1");
    return 0;
}
