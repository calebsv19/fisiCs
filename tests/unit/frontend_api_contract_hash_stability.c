#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

static int analyze(const char* file_path, const char* src, FisicsAnalysisResult* out) {
    *out = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer(file_path, src, strlen(src), NULL, out)) {
        fprintf(stderr, "fisics_analyze_buffer failed for %s\n", file_path);
        return 1;
    }
    return 0;
}

int main(void) {
    const char* src_a = "int main(void){ return 1; }\n";
    const char* src_b = "int main(void){ return 2; }\n";
    FisicsAnalysisResult first = {0};
    FisicsAnalysisResult second = {0};
    FisicsAnalysisResult changed = {0};

    if (analyze("hash_a_1.c", src_a, &first) != 0) return 1;
    if (analyze("hash_a_2.c", src_a, &second) != 0) {
        fisics_free_analysis_result(&first);
        return 1;
    }
    if (analyze("hash_b.c", src_b, &changed) != 0) {
        fisics_free_analysis_result(&first);
        fisics_free_analysis_result(&second);
        return 1;
    }

    if (first.contract.source_hash != second.contract.source_hash) {
        fprintf(stderr, "source_hash not deterministic for identical buffers\n");
        fisics_free_analysis_result(&first);
        fisics_free_analysis_result(&second);
        fisics_free_analysis_result(&changed);
        return 1;
    }

    if (first.contract.source_hash == changed.contract.source_hash) {
        fprintf(stderr, "source_hash did not change for different buffers\n");
        fisics_free_analysis_result(&first);
        fisics_free_analysis_result(&second);
        fisics_free_analysis_result(&changed);
        return 1;
    }

    fisics_free_analysis_result(&first);
    fisics_free_analysis_result(&second);
    fisics_free_analysis_result(&changed);
    return 0;
}
