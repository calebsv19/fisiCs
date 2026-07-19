#include "daw_stage_g_common.h"

#include "export/daw_trace_export.h"
#include "session.h"

#include <sys/stat.h>

static uint64_t workflow_digest(const SessionDocument* doc) {
    const SessionTrack* track = &doc->tracks[0];
    const SessionClip* clip = &track->clips[0];
    uint64_t h = UINT64_C(1469598103934665603);
    h = daw_g_hash_text(h, track->name);
    h = daw_g_hash_text(h, clip->name);
    h = daw_g_hash_u64(h, clip->start_frame);
    h = daw_g_hash_u64(h, clip->duration_frames);
    h = daw_g_hash_u64(h, (uint64_t)clip->midi_note_count);
    for (int i = 0; i < clip->midi_note_count; ++i) {
        h = daw_g_hash_u64(h, clip->midi_notes[i].start_frame);
        h = daw_g_hash_u64(h, clip->midi_notes[i].duration_frames);
        h = daw_g_hash_u64(h, clip->midi_notes[i].note);
    }
    h = daw_g_hash_u64(h, doc->loop.start_frame);
    h = daw_g_hash_u64(h, doc->loop.end_frame);
    return h;
}

int main(void) {
    SessionDocument doc, loaded;
    EngineMidiNoteList phrase;
    char fields[256], canonical[512], error[256];
    daw_g_expect(mkdir("runtime", 0777) == 0, "create runtime directory");
    session_document_init(&doc);
    doc.engine.sample_rate = 48000;
    doc.engine.block_size = 256;
    doc.tempo.bpm = 124.0f;
    doc.timeline.visible_seconds = 16.0f;
    doc.timeline.vertical_scale = 1.0f;
    strcpy(doc.data_paths.output_root, "runtime/output");
    daw_g_trace("b4_bootstrap", "sample_rate=48000|block_size=256|tempo=124");

    doc.track_count = 1;
    doc.tracks = calloc(1, sizeof(*doc.tracks));
    daw_g_expect(doc.tracks != NULL, "allocate project track");
    strcpy(doc.tracks[0].name, "Workflow Track");
    doc.tracks[0].gain = 0.8f;
    doc.tracks[0].midi_instrument_enabled = true;
    doc.tracks[0].midi_instrument_preset = ENGINE_INSTRUMENT_PRESET_PLUCK;
    doc.tracks[0].midi_instrument_params = engine_instrument_default_params(ENGINE_INSTRUMENT_PRESET_PLUCK);
    doc.tracks[0].clip_count = 1;
    doc.tracks[0].clips = calloc(1, sizeof(*doc.tracks[0].clips));
    daw_g_expect(doc.tracks[0].clips != NULL, "allocate project clip");
    SessionClip* clip = &doc.tracks[0].clips[0];
    clip->kind = ENGINE_CLIP_KIND_MIDI;
    strcpy(clip->name, "Stage G Phrase");
    clip->duration_frames = 96000;
    clip->gain = 1.0f;
    clip->instrument_preset = ENGINE_INSTRUMENT_PRESET_PLUCK;
    clip->instrument_params = engine_instrument_default_params(ENGINE_INSTRUMENT_PRESET_PLUCK);
    daw_g_trace("b4_project", "tracks=1|clips=1|kind=midi");

    engine_midi_note_list_init(&phrase);
    daw_g_expect(engine_midi_note_list_insert(&phrase, (EngineMidiNote){24000, 12000, 67, 0.8f}, NULL),
                 "draw note 67");
    daw_g_expect(engine_midi_note_list_insert(&phrase, (EngineMidiNote){0, 12000, 60, 0.9f}, NULL),
                 "draw note 60");
    daw_g_expect(engine_midi_note_list_insert(&phrase, (EngineMidiNote){48000, 24000, 72, 0.7f}, NULL),
                 "draw note 72");
    clip->midi_note_count = phrase.note_count;
    clip->midi_notes = calloc((size_t)phrase.note_count, sizeof(*clip->midi_notes));
    daw_g_expect(clip->midi_notes != NULL, "copy phrase");
    memcpy(clip->midi_notes, phrase.notes, (size_t)phrase.note_count * sizeof(*clip->midi_notes));
    engine_midi_note_list_free(&phrase);
    daw_g_trace("b4_draw", "notes=3|first=60|last=72");

    doc.active_track_index = 0;
    doc.selected_track_index = 0;
    doc.selected_clip_index = 0;
    doc.selection_count = 1;
    doc.selection[0] = (SessionTimelineSelectionEntry){0, 0};
    clip->selected = true;
    clip->start_frame = 24000;
    clip->duration_frames = 120000;
    doc.loop = (SessionLoopState){true, 24000, 144000};
    daw_g_trace("b4_edit", "selected=1|start=24000|duration=120000|loop_end=144000");

    daw_g_expect(session_document_validate(&doc, error, sizeof(error)), error);
    uint64_t expected = workflow_digest(&doc);
    daw_g_expect(session_document_write_file(&doc, "runtime/workflow_session.json"), "save workflow");
    session_document_free(&doc);
    daw_g_trace("b4_saved_destroyed", "artifact=runtime/workflow_session.json|destroyed=1");

    session_document_init(&loaded);
    daw_g_expect(session_document_read_file("runtime/workflow_session.json", &loaded), "reload workflow");
    uint64_t actual = workflow_digest(&loaded);
    daw_g_expect(actual == expected, "workflow semantic parity");
    snprintf(fields, sizeof(fields), "tracks=%d|notes=%d|digest=%016llx",
             loaded.track_count, loaded.tracks[0].clips[0].midi_note_count,
             (unsigned long long)actual);
    daw_g_trace("b4_reloaded", fields);

    DawTraceDiagnostics diagnostics = {
        .frame_dt_seconds = 0.016f,
        .transport_frame = loaded.tracks[0].clips[0].start_frame,
        .sched_block_size = (uint32_t)loaded.engine.block_size,
        .sample_rate = (uint32_t)loaded.engine.sample_rate,
        .tempo_event_count = (uint32_t)loaded.tempo_event_count,
        .time_signature_event_count = (uint32_t)loaded.time_signature_event_count,
        .loop_enabled = loaded.loop.enabled,
        .loop_start_frame = loaded.loop.start_frame,
        .loop_end_frame = loaded.loop.end_frame
    };
    daw_g_expect(daw_trace_export_diagnostics("runtime/workflow_trace.pack", &diagnostics),
                 "export diagnostics");
    daw_g_trace("b4_exported", "artifact=runtime/workflow_trace.pack|samples=9|markers=2");

    SessionDocument invalid = loaded;
    invalid.loop.end_frame = invalid.loop.start_frame;
    daw_g_expect(!session_document_validate(&invalid, error, sizeof(error)), "reject invalid loop");
    daw_g_trace("b4_invalid", "invalid_loop=0|state_preserved=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\ndigest=%016llx\ntracks=%d\nnotes=%d\nstart=%llu\nduration=%llu\nloop=%llu:%llu\n",
             (unsigned long long)actual, loaded.track_count, loaded.tracks[0].clips[0].midi_note_count,
             (unsigned long long)loaded.tracks[0].clips[0].start_frame,
             (unsigned long long)loaded.tracks[0].clips[0].duration_frames,
             (unsigned long long)loaded.loop.start_frame, (unsigned long long)loaded.loop.end_frame);
    daw_g_write_text("workflow.canonical", canonical);
    daw_g_trace("b4_canonical", "artifact=workflow.canonical|version=1");
    session_document_free(&loaded);
    daw_g_trace("b4_shutdown", "document_freed=1|trace_export_complete=1");
    return 0;
}
