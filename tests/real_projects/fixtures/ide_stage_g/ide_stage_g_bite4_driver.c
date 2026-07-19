#include "ide_stage_g_common.h"

#include "core/Analysis/analysis_store.h"
#include "core/Diagnostics/diagnostics_engine.h"
#include "core/LoopEvents/event_queue.h"
#include "core/LoopKernel/mainthread_context.h"
#include "core/LoopResults/completed_results_queue.h"
#include "ide/Panes/ToolPanels/Errors/errors_filter.h"

#include <sys/stat.h>

typedef struct PublishProjection {
    unsigned int events;
    uint64_t digest;
} PublishProjection;

static void publish_event(const IDEEvent* event, void* user_data) {
    PublishProjection* projection = (PublishProjection*)user_data;
    ide_g_expect(event->type == IDE_EVENT_DIAGNOSTICS_UPDATED, "diagnostics event kind");
    analysis_store_mark_published(event->payload.analysis.data_stamp);
    projection->events++;
    projection->digest = ide_g_hash_u64(projection->digest, event->sequence);
    projection->digest = ide_g_hash_u64(projection->digest,
                                        event->payload.analysis.analysis_run_id);
    projection->digest = ide_g_hash_u64(projection->digest,
                                        event->payload.analysis.data_stamp);
}

static uint64_t diagnostics_digest(void) {
    uint64_t hash = UINT64_C(1469598103934665603);
    const AnalysisFileDiagnostics* ordered[16];
    size_t count = analysis_store_file_count();
    ide_g_expect(count <= 16u, "bounded diagnostics projection");
    for (size_t i = 0; i < count; ++i) {
        ordered[i] = analysis_store_file_at(i);
    }
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1u; j < count; ++j) {
            if (strcmp(ordered[j]->path, ordered[i]->path) < 0) {
                const AnalysisFileDiagnostics* swap = ordered[i];
                ordered[i] = ordered[j];
                ordered[j] = swap;
            }
        }
    }
    for (size_t i = 0; i < count; ++i) {
        const AnalysisFileDiagnostics* file = ordered[i];
        hash = ide_g_hash_text(hash, file->path);
        hash = ide_g_hash_u64(hash, (uint64_t)file->count);
        for (int j = 0; j < file->count; ++j) {
            const Diagnostic* diagnostic = &file->diags[j];
            hash = ide_g_hash_u64(hash, (uint64_t)diagnostic->line);
            hash = ide_g_hash_u64(hash, (uint64_t)diagnostic->column);
            hash = ide_g_hash_u64(hash, (uint64_t)diagnostic->severity);
            hash = ide_g_hash_u64(hash, (uint64_t)diagnostic->category);
            hash = ide_g_hash_u64(hash, (uint64_t)diagnostic->codeId);
            hash = ide_g_hash_text(hash, diagnostic->message);
        }
    }
    return hash;
}

static void seed_diagnostic(const char* path,
                            int line,
                            int column,
                            DiagKind kind,
                            int category,
                            int code,
                            const char* message) {
    FisicsDiagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.file_path = (char*)path;
    diagnostic.line = line;
    diagnostic.column = column;
    diagnostic.length = 2;
    diagnostic.kind = kind;
    diagnostic.category_id = category;
    diagnostic.code_id = code;
    diagnostic.message = (char*)message;
    analysis_store_upsert(path, &diagnostic, 1u);
}

int main(void) {
    char fields[256];
    char canonical[768];
    ide_g_expect(mkdir("runtime", 0777) == 0, "create runtime directory");
    ide_g_expect(mkdir("runtime/workspace", 0777) == 0, "create workspace directory");
    mainthread_context_set_owner_current();
    initDiagnosticsEngine();
    completed_results_queue_init();
    loop_events_init();
    analysis_store_clear();
    ide_g_trace("b4_bootstrap", "owner=1|files=0|results=0|events=0");

    seed_diagnostic("src/main.c", 12, 4, DIAG_WARNING,
                    FISICS_DIAG_CATEGORY_SEMANTIC, 1001, "implicit conversion");
    seed_diagnostic("src/util.c", 7, 2, DIAG_ERROR,
                    FISICS_DIAG_CATEGORY_EXTENSION, 4103, "dimension mismatch");
    uint64_t seeded_digest = diagnostics_digest();
    uint64_t live_stamp = analysis_store_combined_stamp();
    snprintf(fields, sizeof(fields), "files=2|diagnostics=2|stamp=%llu|digest=%016llx",
             (unsigned long long)live_stamp, (unsigned long long)seeded_digest);
    ide_g_trace("b4_project", fields);

    analysis_store_save("runtime/workspace");
    struct stat artifact_stat;
    ide_g_expect(stat("runtime/workspace/ide_files/analysis_diagnostics.json", &artifact_stat) == 0,
                 "diagnostics artifact exists");
    analysis_store_clear();
    ide_g_trace("b4_saved_destroyed", "artifact=analysis_diagnostics.json|files=0");

    analysis_store_load("runtime/workspace");
    uint64_t loaded_digest = diagnostics_digest();
    live_stamp = analysis_store_combined_stamp();
    ide_g_expect(analysis_store_file_count() == 2u, "reload diagnostics files");
    ide_g_expect(loaded_digest == seeded_digest, "reload diagnostics digest");
    analysis_store_flatten_to_engine();
    ide_g_expect(getDiagnosticCount() == 2, "flatten diagnostics");
    const AnalysisFileDiagnostics* newest_file = analysis_store_file_at(0u);
    ide_g_expect(newest_file && strcmp(newest_file->path, "src/main.c") == 0,
                 "observe production diagnostics recency reversal");
    const Diagnostic* selected = NULL;
    for (int i = 0; i < getDiagnosticCount(); ++i) {
        const Diagnostic* candidate = getDiagnosticAt(i);
        if (errors_filter_diagnostic_matches_query(candidate, "dimension")) {
            selected = candidate;
            break;
        }
    }
    ide_g_expect(selected != NULL, "filter diagnostic message");
    ide_g_expect(!errors_filter_diagnostic_matches_query(selected, "lexer"),
                 "filter negative query");
    snprintf(fields, sizeof(fields),
             "files=2|diagnostics=%d|stamp=%llu|query_match=1|recency_preserved=0|digest=%016llx",
             getDiagnosticCount(), (unsigned long long)live_stamp,
             (unsigned long long)loaded_digest);
    ide_g_trace("b4_reloaded_filtered", fields);

    CompletedResult result;
    memset(&result, 0, sizeof(result));
    result.subsystem = COMPLETED_SUBSYSTEM_DIAGNOSTICS;
    result.kind = COMPLETED_RESULT_DIAGNOSTICS_UPDATED;
    result.payload.diagnostics_updated.analysis_run_id = 77u;
    result.payload.diagnostics_updated.diagnostics_stamp = live_stamp;
    strcpy(result.payload.diagnostics_updated.project_root, "project");
    ide_g_expect(completed_results_queue_push(&result), "queue diagnostics result");
    CompletedResult popped;
    ide_g_expect(completed_results_queue_pop_any(&popped), "pop diagnostics result");
    ide_g_expect(popped.payload.diagnostics_updated.diagnostics_stamp ==
                 analysis_store_combined_stamp(), "live result stamp");
    completed_results_queue_note_applied();
    ide_g_expect(loop_events_emit_diagnostics_updated(
                     popped.payload.diagnostics_updated.project_root,
                     popped.payload.diagnostics_updated.analysis_run_id,
                     popped.payload.diagnostics_updated.diagnostics_stamp),
                 "emit published event");
    completed_results_queue_release(&popped);
    PublishProjection projection = {0u, UINT64_C(1469598103934665603)};
    ide_g_expect(loop_events_drain_bounded(4u, publish_event, &projection) == 1u,
                 "publish event drain");
    ide_g_expect(analysis_store_published_stamp() == live_stamp, "published stamp");
    snprintf(fields, sizeof(fields), "applied=1|events=%u|published=%llu|digest=%016llx",
             projection.events, (unsigned long long)analysis_store_published_stamp(),
             (unsigned long long)projection.digest);
    ide_g_trace("b4_published", fields);

    result.payload.diagnostics_updated.diagnostics_stamp = live_stamp - 1u;
    ide_g_expect(completed_results_queue_push(&result), "queue stale result");
    ide_g_expect(completed_results_queue_pop_any(&popped), "pop stale result");
    ide_g_expect(popped.payload.diagnostics_updated.diagnostics_stamp !=
                 analysis_store_combined_stamp(), "stale result stamp");
    completed_results_queue_note_stale_dropped();
    completed_results_queue_release(&popped);
    ide_g_expect(loop_events_size() == 0u, "stale result emits no event");
    CompletedResultsQueueStats stats;
    completed_results_queue_snapshot(&stats);
    ide_g_trace("b4_stale", "stale_dropped=1|events=0|state_preserved=1");

    ide_g_expect(!errors_filter_diagnostic_matches_query(NULL, "dimension"),
                 "reject null diagnostic");
    ide_g_expect(errors_filter_diagnostic_matches_query(selected, NULL),
                 "null query selects all");
    ide_g_trace("b4_invalid", "null_diagnostic=0|null_query=1|state_preserved=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\nfiles=2\ndiagnostics=2\ndigest=%016llx\nlive_stamp=%llu\npublished_stamp=%llu\nevents=%u\nevent_digest=%016llx\napplied=%llu\nstale=%llu\n",
             (unsigned long long)loaded_digest, (unsigned long long)live_stamp,
             (unsigned long long)analysis_store_published_stamp(), projection.events,
             (unsigned long long)projection.digest,
             (unsigned long long)stats.results_applied,
             (unsigned long long)stats.results_stale_dropped);
    ide_g_write_text("workflow.canonical", canonical);
    ide_g_trace("b4_canonical", "artifact=workflow.canonical|version=1");

    analysis_store_clear();
    clearDiagnostics();
    loop_events_shutdown();
    completed_results_queue_shutdown();
    mainthread_context_clear_owner();
    ide_g_trace("b4_shutdown", "owner=0|stores_cleared=1|queues_shutdown=1");
    return 0;
}
