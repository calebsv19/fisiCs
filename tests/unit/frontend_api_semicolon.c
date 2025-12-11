#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(){ int x = 1\n return x; }\n";
    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("missing_semicolon.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed on missing semicolon case\n");
        return 1;
    }
    int saw = 0;
    for (size_t i = 0; i < res.diag_count; ++i) {
        if (res.diagnostics[i].code == CDIAG_PARSER_EXPECT_SEMICOLON) {
            saw = 1;
            break;
        }
    }
    if (!saw) {
        fprintf(stderr, "expected missing-semicolon diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.diag_count > 0) {
        if (!res.diagnostics[0].message || res.diagnostics[0].message[0] == '\0') {
            fprintf(stderr, "expected diagnostic message content\n");
            fisics_free_analysis_result(&res);
            return 1;
        }
        if (!res.diagnostics[0].hint || res.diagnostics[0].hint[0] == '\0') {
            fprintf(stderr, "expected diagnostic hint content\n");
            fisics_free_analysis_result(&res);
            return 1;
        }
    }
    if (res.token_count == 0) {
        fprintf(stderr, "expected token spans for missing-semicolon case\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);
    return 0;
}
