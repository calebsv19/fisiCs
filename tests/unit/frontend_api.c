#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(){ return missing; }\n";
    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("inline_input.c", src, strlen(src), &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }
    if (res.token_count == 0) {
        fprintf(stderr, "expected tokens\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.diag_count == 0) {
        fprintf(stderr, "expected diagnostics\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);

    const char* missing_semicolon_src = "int main(){ int x = 1\n return x; }\n";
    FisicsAnalysisResult semi = {0};
    if (!fisics_analyze_buffer("missing_semicolon.c", missing_semicolon_src, strlen(missing_semicolon_src), &semi)) {
        fprintf(stderr, "fisics_analyze_buffer failed on missing semicolon case\n");
        return 1;
    }
    bool sawMissingSemicolon = false;
    for (size_t i = 0; i < semi.diag_count; ++i) {
        if (semi.diagnostics[i].code == CDIAG_PARSER_EXPECT_SEMICOLON) {
            sawMissingSemicolon = true;
            break;
        }
    }
    if (!sawMissingSemicolon) {
        fprintf(stderr, "expected missing-semicolon diagnostic\n");
        fisics_free_analysis_result(&semi);
        return 1;
    }
    fisics_free_analysis_result(&semi);
    return 0;
}
