#include "daw_stage_g_common.h"

#include "engine/midi.h"
#include "time/tempo.h"

static uint64_t note_digest(const EngineMidiNoteList* list) {
    uint64_t h = UINT64_C(1469598103934665603);
    h = daw_g_hash_u64(h, (uint64_t)list->note_count);
    for (int i = 0; i < list->note_count; ++i) {
        const EngineMidiNote* n = &list->notes[i];
        h = daw_g_hash_u64(h, n->start_frame);
        h = daw_g_hash_u64(h, n->duration_frames);
        h = daw_g_hash_u64(h, n->note);
        h = daw_g_hash_bytes(h, &n->velocity, sizeof(n->velocity));
    }
    return h;
}

int main(void) {
    EngineMidiNoteList notes;
    TempoMap tempo;
    int index = -1;
    char fields[256];
    char canonical[512];

    engine_midi_note_list_init(&notes);
    tempo_map_init(&tempo, 48000.0);
    daw_g_trace("b1_bootstrap", "sample_rate=48000|note_count=0|tempo_events=1");

    const EngineMidiNote seeded[] = {
        {960, 240, 67, 0.75f}, {0, 120, 60, 1.0f}, {480, 360, 64, 0.5f}
    };
    daw_g_expect(engine_midi_note_list_set(&notes, seeded, 3), "seed note list");
    snprintf(fields, sizeof(fields), "count=%d|first=%u|last=%u|digest=%016llx",
             notes.note_count, notes.notes[0].note, notes.notes[2].note,
             (unsigned long long)note_digest(&notes));
    daw_g_trace("b1_seeded", fields);

    daw_g_expect(engine_midi_note_list_insert(&notes, (EngineMidiNote){240, 120, 62, 0.8f}, &index),
                 "insert note");
    daw_g_expect(index == 1, "insert sort index");
    daw_g_expect(engine_midi_note_list_update(&notes, 3, (EngineMidiNote){120, 60, 72, 0.25f}, &index),
                 "update note");
    daw_g_expect(index == 1, "update sort index");
    daw_g_expect(engine_midi_note_list_remove(&notes, 2), "remove note");
    snprintf(fields, sizeof(fields), "count=%d|first=%u|middle=%u|digest=%016llx",
             notes.note_count, notes.notes[0].note, notes.notes[1].note,
             (unsigned long long)note_digest(&notes));
    daw_g_trace("b1_edited", fields);

    daw_g_expect(!engine_midi_note_list_insert(&notes, (EngineMidiNote){0, 0, 60, 1.0f}, NULL),
                 "reject zero duration");
    daw_g_expect(!engine_midi_note_list_insert(&notes, (EngineMidiNote){0, 1, 60, 1.5f}, NULL),
                 "reject velocity");
    daw_g_expect(engine_midi_note_list_validate(&notes), "validate notes");
    daw_g_trace("b1_invalid", "zero_duration=0|velocity_overflow=0|valid=1");

    const TempoEvent events[] = {{8.0, 96.0}, {0.0, 120.0}, {4.0, 140.0}};
    daw_g_expect(tempo_map_set_events(&tempo, events, 3), "set tempo events");
    snprintf(fields, sizeof(fields), "events=%d|beat0=%.0f|beat4=%.0f|beat8=%.0f|frame_at_10=%.0f",
             tempo.event_count, tempo.events[0].bpm, tempo.events[1].bpm, tempo.events[2].bpm,
             tempo_map_beats_to_samples(&tempo, 10.0));
    daw_g_trace("b1_tempo", fields);

    snprintf(canonical, sizeof(canonical),
             "version=1\nnotes=%d\nnote_digest=%016llx\ntempo_events=%d\nframe_at_10=%.0f\n",
             notes.note_count, (unsigned long long)note_digest(&notes), tempo.event_count,
             tempo_map_beats_to_samples(&tempo, 10.0));
    daw_g_write_text("state_transfer.canonical", canonical);
    daw_g_trace("b1_canonical", "artifact=state_transfer.canonical|version=1");

    tempo_map_free(&tempo);
    engine_midi_note_list_free(&notes);
    daw_g_trace("b1_shutdown", "notes_freed=1|tempo_freed=1");
    return 0;
}
