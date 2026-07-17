#include "mem_console_stage_g_common.h"

#include "mem_console_db.h"
#include "mem_console_prefs.h"

#include <unistd.h>

static int64_t query_i64(CoreMemDb *db, const char *sql) {
    CoreMemStmt stmt = {0};
    CoreResult result;
    int has_row = 0;
    int64_t value = -1;
    result = core_memdb_prepare(db, sql, &stmt);
    if (result.code != CORE_OK) return -1;
    result = core_memdb_stmt_step(&stmt, &has_row);
    if (result.code != CORE_OK || !has_row) { (void)core_memdb_stmt_finalize(&stmt); return -1; }
    result = core_memdb_stmt_column_i64(&stmt, 0, &value);
    (void)core_memdb_stmt_finalize(&stmt);
    return result.code == CORE_OK ? value : -1;
}

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    CoreMemDb db = {0};
    CoreResult result;
    MemConsoleState state;
    int64_t link_id = 0;
    uint64_t digest = basis;
    char detail[768];
    int64_t item_count;
    int64_t pinned_count;
    int64_t supports_count;

    (void)unlink("workflow.sqlite");
    result = core_memdb_open("workflow.sqlite", &db);
    if (result.code != CORE_OK) return 40;
    digest = mc_stageg_hash_text(digest, "db_open=1");
    mc_stageg_trace("b4_bootstrap", "db_open=1", digest);

    result = core_memdb_exec(
        &db,
        "INSERT INTO mem_item (id,stable_id,title,body,fingerprint,workspace_key,project_key,kind,created_ns,updated_ns,pinned,canonical,ttl_until_ns,archived_ns) VALUES "
        "(101,'alpha','Alpha','body alpha','fa','codework','mem_console','note',1,1,0,0,NULL,NULL),"
        "(202,'beta','Beta','body beta','fb','codework','fisiCs','decision',2,2,0,0,NULL,NULL);"
        "INSERT INTO mem_item_fts(rowid,title,body) VALUES (101,'Alpha','body alpha'),(202,'Beta','body beta');");
    if (result.code != CORE_OK) return 41;
    item_count = query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE archived_ns IS NULL;");
    snprintf(detail, sizeof(detail), "items=%lld", (long long)item_count);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b4_seeded", detail, digest);

    memset(&state, 0, sizeof(state));
    state.selected_item_id = 101;
    snprintf(state.selected_body, sizeof(state.selected_body), "%s", "body alpha");
    snprintf(state.title_edit_text, sizeof(state.title_edit_text), "%s", "Alpha Renamed");
    result = rename_selected_from_title_buffer(&db, &state);
    if (result.code != CORE_OK) return 42;
    result = set_selected_item_flag(&db, &state, MEM_CONSOLE_ITEM_FLAG_PINNED, 1);
    if (result.code != CORE_OK) return 43;
    pinned_count = query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE pinned=1 AND title='Alpha Renamed';");
    snprintf(detail, sizeof(detail), "renamed=1,pinned=%lld", (long long)pinned_count);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b4_mutated", detail, digest);

    snprintf(state.relationship_target_text, sizeof(state.relationship_target_text), "%s", "202");
    result = create_selected_relationship_to_target(&db, &state, &link_id);
    if (result.code != CORE_OK || link_id <= 0) return 44;
    state.relationship_action_link_id = link_id;
    result = cycle_selected_relationship_kind(&db, &state, &link_id);
    if (result.code != CORE_OK) return 45;
    supports_count = query_i64(&db, "SELECT COUNT(*) FROM mem_link WHERE from_item_id=101 AND to_item_id=202 AND kind='supports';");
    snprintf(detail, sizeof(detail), "link=%lld,supports=%lld", (long long)link_id,
             (long long)supports_count);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b4_related", detail, digest);

    result = mem_console_app_prefs_save("workflow.app.pack", "workflow.sqlite",
                                        "input", "output", "workflow.sqlite");
    if (result.code != CORE_OK) return 46;
    result = core_memdb_close(&db);
    if (result.code != CORE_OK) return 47;
    mc_stageg_trace("b4_saved_destroyed", "prefs_saved=1,db_closed=1", digest);

    memset(&db, 0, sizeof(db));
    result = core_memdb_open("workflow.sqlite", &db);
    if (result.code != CORE_OK) return 48;
    item_count = query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE archived_ns IS NULL;");
    pinned_count = query_i64(&db, "SELECT COUNT(*) FROM mem_item WHERE pinned=1;");
    supports_count = query_i64(&db, "SELECT COUNT(*) FROM mem_link WHERE kind='supports';");
    snprintf(detail, sizeof(detail), "items=%lld,pinned=%lld,supports=%lld",
             (long long)item_count, (long long)pinned_count, (long long)supports_count);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b4_reloaded", detail, digest);

    state.selected_item_id = 101;
    snprintf(state.relationship_target_text, sizeof(state.relationship_target_text), "%s", "101");
    result = create_selected_relationship_to_target(&db, &state, &link_id);
    if (result.code != CORE_ERR_INVALID_ARG || item_count != 2 || pinned_count != 1 || supports_count != 1) return 49;
    digest = mc_stageg_hash_text(digest, "self_link_rejected=1");
    mc_stageg_trace("b4_invalid", "self_link_rejected=1", digest);

    snprintf(detail, sizeof(detail),
             "schema=mem_console_stage_g_bite4_v1\ndigest=%016llx\nitems=%lld\npinned=%lld\nsupports=%lld\n",
             (unsigned long long)digest, (long long)item_count,
             (long long)pinned_count, (long long)supports_count);
    if (!mc_stageg_write_text("workflow.canonical", detail)) return 50;
    mc_stageg_trace("b4_canonical", "artifact=workflow.canonical", digest);
    result = core_memdb_close(&db);
    if (result.code != CORE_OK) return 51;
    mc_stageg_trace("b4_shutdown", "db_closed=1", digest);
    return 0;
}
