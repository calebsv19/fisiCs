#include "ide_stage_g_common.h"

#include "core/Analysis/analysis_token_store.h"

#include <sys/stat.h>

static uint64_t token_digest(void) {
    uint64_t hash = UINT64_C(1469598103934665603);
    const AnalysisFileTokens* ordered[16];
    size_t count = analysis_token_store_file_count();
    ide_g_expect(count <= 16u, "bounded token projection");
    for (size_t i = 0; i < count; ++i) {
        ordered[i] = analysis_token_store_file_at(i);
    }
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1u; j < count; ++j) {
            if (strcmp(ordered[j]->path, ordered[i]->path) < 0) {
                const AnalysisFileTokens* swap = ordered[i];
                ordered[i] = ordered[j];
                ordered[j] = swap;
            }
        }
    }
    for (size_t i = 0; i < count; ++i) {
        const AnalysisFileTokens* file = ordered[i];
        hash = ide_g_hash_text(hash, file->path);
        hash = ide_g_hash_u64(hash, file->count);
        for (size_t j = 0; j < file->count; ++j) {
            hash = ide_g_hash_u64(hash, (uint64_t)file->spans[j].line);
            hash = ide_g_hash_u64(hash, (uint64_t)file->spans[j].column);
            hash = ide_g_hash_u64(hash, (uint64_t)file->spans[j].length);
            hash = ide_g_hash_u64(hash, (uint64_t)file->spans[j].kind);
        }
    }
    return hash;
}

int main(void) {
    char fields[256];
    char canonical[512];
    ide_g_expect(mkdir("runtime", 0777) == 0, "create runtime directory");
    ide_g_expect(mkdir("runtime/workspace", 0777) == 0, "create workspace directory");

    FisicsTokenSpan main_spans[3];
    memset(main_spans, 0, sizeof(main_spans));
    main_spans[0] = (FisicsTokenSpan){1, 1, 3, FISICS_TOK_KEYWORD};
    main_spans[1] = (FisicsTokenSpan){1, 5, 4, FISICS_TOK_IDENTIFIER};
    main_spans[2] = (FisicsTokenSpan){2, 3, 6, FISICS_TOK_STRING};
    FisicsTokenSpan util_spans[2];
    memset(util_spans, 0, sizeof(util_spans));
    util_spans[0] = (FisicsTokenSpan){4, 2, 5, FISICS_TOK_IDENTIFIER};
    util_spans[1] = (FisicsTokenSpan){4, 8, 2, FISICS_TOK_NUMBER};

    analysis_token_store_clear();
    analysis_token_store_upsert("src/main.c", main_spans, 3u);
    analysis_token_store_upsert("src/util.c", util_spans, 2u);
    uint64_t before = token_digest();
    snprintf(fields, sizeof(fields), "files=%zu|tokens=5|newest=src/util.c|digest=%016llx",
             analysis_token_store_file_count(), (unsigned long long)before);
    ide_g_trace("b2_mutated", fields);

    analysis_token_store_save("runtime/workspace");
    struct stat artifact_stat;
    ide_g_expect(stat("runtime/workspace/ide_files/analysis_tokens.json", &artifact_stat) == 0,
                 "token artifact exists");
    snprintf(fields, sizeof(fields), "artifact=analysis_tokens.json|bytes=%lld|files=2",
             (long long)artifact_stat.st_size);
    ide_g_trace("b2_saved", fields);

    analysis_token_store_clear();
    ide_g_expect(analysis_token_store_file_count() == 0u, "token store destroyed");
    ide_g_trace("b2_destroyed", "files=0|tokens=0");

    analysis_token_store_load("runtime/workspace");
    uint64_t after = token_digest();
    ide_g_expect(analysis_token_store_file_count() == 2u, "reload file count");
    ide_g_expect(before == after, "reload semantic digest");
    const AnalysisFileTokens* newest = analysis_token_store_file_at(0u);
    ide_g_expect(newest && strcmp(newest->path, "src/main.c") == 0,
                 "observe production recency reversal");
    snprintf(fields, sizeof(fields),
             "files=2|tokens=5|newest=src/main.c|recency_preserved=0|digest=%016llx",
             (unsigned long long)after);
    ide_g_trace("b2_reloaded", fields);

    analysis_token_store_remove("src/util.c");
    analysis_token_store_save("runtime/workspace");
    analysis_token_store_clear();
    analysis_token_store_load("runtime/workspace");
    ide_g_expect(analysis_token_store_file_count() == 1u, "persist removal");
    uint64_t removed_digest = token_digest();
    snprintf(fields, sizeof(fields), "files=1|tokens=3|remaining=src/main.c|digest=%016llx",
             (unsigned long long)removed_digest);
    ide_g_trace("b2_removed_reloaded", fields);

    analysis_token_store_save("");
    ide_g_expect(analysis_token_store_file_count() == 1u, "invalid save preserves state");
    ide_g_trace("b2_invalid", "empty_workspace=0|state_preserved=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\ninitial_files=2\ninitial_tokens=5\ninitial_digest=%016llx\nfinal_files=1\nfinal_tokens=3\nfinal_digest=%016llx\n",
             (unsigned long long)before, (unsigned long long)removed_digest);
    ide_g_write_text("persistence.canonical", canonical);
    ide_g_trace("b2_canonical", "artifact=persistence.canonical|version=1");
    analysis_token_store_clear();
    ide_g_trace("b2_shutdown", "store_cleared=1|artifact_closed=1");
    return 0;
}
