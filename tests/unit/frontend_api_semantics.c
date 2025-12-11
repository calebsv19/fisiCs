#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    const char* src = "int foo(){ return 1; }\nint main(){ return bar(); }\n";
    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("undeclared_call.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed on undeclared call case\n");
        return 1;
    }
    int sawUndeclared = 0;
    for (size_t i = 0; i < res.diag_count; ++i) {
        if (res.diagnostics[i].message &&
            strstr(res.diagnostics[i].message, "Undeclared identifier")) {
            sawUndeclared = 1;
            break;
        }
    }
    if (!sawUndeclared) {
        fprintf(stderr, "expected undeclared identifier diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.symbol_count < 2) {
        fprintf(stderr, "expected symbols for foo and main\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);
    return 0;
}
