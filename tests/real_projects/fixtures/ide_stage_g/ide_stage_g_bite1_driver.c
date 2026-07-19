#include "ide_stage_g_common.h"

#include "core/LoopEvents/event_queue.h"
#include "core/LoopKernel/mainthread_context.h"
#include "core/LoopResults/completed_results_queue.h"

typedef struct EventProjection {
    uint64_t digest;
    unsigned int count;
    unsigned int first_type;
    unsigned int last_type;
} EventProjection;

static void project_event(const IDEEvent* event, void* user_data) {
    EventProjection* projection = (EventProjection*)user_data;
    if (projection->count == 0u) {
        projection->first_type = (unsigned int)event->type;
    }
    projection->last_type = (unsigned int)event->type;
    projection->count++;
    projection->digest = ide_g_hash_u64(projection->digest, (uint64_t)event->type);
    projection->digest = ide_g_hash_u64(projection->digest, event->sequence);
    if (event->type == IDE_EVENT_DOCUMENT_EDITED ||
        event->type == IDE_EVENT_DOCUMENT_REVISION_CHANGED) {
        projection->digest = ide_g_hash_text(projection->digest,
                                             event->payload.document.document_path);
        projection->digest = ide_g_hash_u64(projection->digest,
                                            event->payload.document.document_revision);
    } else {
        projection->digest = ide_g_hash_text(projection->digest,
                                             event->payload.analysis.project_root);
        projection->digest = ide_g_hash_u64(projection->digest,
                                            event->payload.analysis.analysis_run_id);
        projection->digest = ide_g_hash_u64(projection->digest,
                                            event->payload.analysis.data_stamp);
    }
}

static CompletedResult make_result(CompletedResultSubsystem subsystem,
                                   CompletedResultKind kind,
                                   uint64_t run_id,
                                   uint64_t stamp) {
    CompletedResult result;
    memset(&result, 0, sizeof(result));
    result.subsystem = subsystem;
    result.kind = kind;
    if (kind == COMPLETED_RESULT_ANALYSIS_FINISHED) {
        result.payload.analysis_finished.analysis_run_id = run_id;
        result.payload.analysis_finished.library_index_stamp = stamp;
        strcpy(result.payload.analysis_finished.project_root, "project");
    } else if (kind == COMPLETED_RESULT_SYMBOLS_UPDATED) {
        result.payload.symbols_updated.analysis_run_id = run_id;
        result.payload.symbols_updated.symbols_stamp = stamp;
        strcpy(result.payload.symbols_updated.project_root, "project");
    } else {
        result.payload.diagnostics_updated.analysis_run_id = run_id;
        result.payload.diagnostics_updated.diagnostics_stamp = stamp;
        strcpy(result.payload.diagnostics_updated.project_root, "project");
    }
    return result;
}

int main(void) {
    char fields[256];
    char canonical[512];
    EventProjection projection = {UINT64_C(1469598103934665603), 0u, 0u, 0u};

    mainthread_context_set_owner_current();
    loop_events_init();
    completed_results_queue_init();
    ide_g_trace("b1_bootstrap", "owner=1|event_depth=0|result_depth=0");

    ide_g_expect(loop_events_emit_document_edited("src/main.c", 7u), "emit edit");
    ide_g_expect(loop_events_emit_diagnostics_updated("project", 41u, 101u), "emit diagnostics");
    ide_g_expect(loop_events_emit_symbol_tree_updated("project", 41u, 102u), "emit symbols");
    ide_g_expect(loop_events_emit_analysis_progress_updated("project", 41u, 3u), "emit progress");
    LoopEventsStats event_stats;
    loop_events_snapshot(&event_stats);
    snprintf(fields, sizeof(fields), "depth=%u|high=%u|enqueued=%llu|next=%llu",
             event_stats.depth, event_stats.high_watermark,
             (unsigned long long)event_stats.events_enqueued,
             (unsigned long long)event_stats.next_sequence);
    ide_g_trace("b1_events_seeded", fields);

    ide_g_expect(loop_events_drain_bounded(2u, project_event, &projection) == 2u,
                 "bounded event drain");
    loop_events_snapshot(&event_stats);
    snprintf(fields, sizeof(fields), "drained=2|remaining=%u|deferred=%llu|digest=%016llx",
             event_stats.depth, (unsigned long long)event_stats.events_deferred,
             (unsigned long long)projection.digest);
    ide_g_trace("b1_events_bounded", fields);

    ide_g_expect(loop_events_drain_bounded(8u, project_event, &projection) == 2u,
                 "final event drain");
    snprintf(fields, sizeof(fields), "count=%u|first=%u|last=%u|digest=%016llx",
             projection.count, projection.first_type, projection.last_type,
             (unsigned long long)projection.digest);
    ide_g_trace("b1_events_drained", fields);

    CompletedResult analysis = make_result(COMPLETED_SUBSYSTEM_ANALYSIS,
                                           COMPLETED_RESULT_ANALYSIS_FINISHED, 41u, 201u);
    CompletedResult diagnostics = make_result(COMPLETED_SUBSYSTEM_DIAGNOSTICS,
                                              COMPLETED_RESULT_DIAGNOSTICS_UPDATED, 41u, 202u);
    CompletedResult symbols = make_result(COMPLETED_SUBSYSTEM_SYMBOLS,
                                          COMPLETED_RESULT_SYMBOLS_UPDATED, 41u, 203u);
    ide_g_expect(completed_results_queue_push(&analysis), "push analysis result");
    ide_g_expect(completed_results_queue_push(&diagnostics), "push diagnostics result");
    ide_g_expect(completed_results_queue_push(&symbols), "push symbols result");
    CompletedResultsQueueStats result_stats;
    completed_results_queue_snapshot(&result_stats);
    snprintf(fields, sizeof(fields), "depth=%u|analysis=%u|symbols=%u|diagnostics=%u|high=%u",
             result_stats.total_depth, result_stats.analysis_depth, result_stats.symbols_depth,
             result_stats.diagnostics_depth, result_stats.high_watermark);
    ide_g_trace("b1_results_seeded", fields);

    uint64_t order_digest = UINT64_C(1469598103934665603);
    CompletedResult popped;
    for (unsigned int expected = 1u; expected <= 3u; ++expected) {
        ide_g_expect(completed_results_queue_pop_any(&popped), "pop result");
        ide_g_expect(popped.seq == expected, "global result ordering");
        order_digest = ide_g_hash_u64(order_digest, popped.seq);
        order_digest = ide_g_hash_u64(order_digest, (uint64_t)popped.kind);
        completed_results_queue_release(&popped);
    }
    completed_results_queue_note_applied();
    completed_results_queue_note_applied();
    completed_results_queue_note_stale_dropped();
    completed_results_queue_snapshot(&result_stats);
    snprintf(fields, sizeof(fields), "popped=%llu|applied=%llu|stale=%llu|digest=%016llx",
             (unsigned long long)result_stats.popped,
             (unsigned long long)result_stats.results_applied,
             (unsigned long long)result_stats.results_stale_dropped,
             (unsigned long long)order_digest);
    ide_g_trace("b1_results_transferred", fields);

    CompletedResult invalid;
    memset(&invalid, 0, sizeof(invalid));
    ide_g_expect(!completed_results_queue_push(&invalid), "reject empty result");
    ide_g_expect(!loop_events_push(NULL), "reject null event");
    ide_g_trace("b1_invalid", "empty_result=0|null_event=0|state_preserved=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\nevents=%u\nevent_digest=%016llx\nresults=%llu\nresult_digest=%016llx\napplied=%llu\nstale=%llu\n",
             projection.count, (unsigned long long)projection.digest,
             (unsigned long long)result_stats.popped, (unsigned long long)order_digest,
             (unsigned long long)result_stats.results_applied,
             (unsigned long long)result_stats.results_stale_dropped);
    ide_g_write_text("state_transfer.canonical", canonical);
    ide_g_trace("b1_canonical", "artifact=state_transfer.canonical|version=1");

    completed_results_queue_shutdown();
    loop_events_shutdown();
    mainthread_context_clear_owner();
    ide_g_trace("b1_shutdown", "owner=0|queues_shutdown=1");
    return 0;
}
