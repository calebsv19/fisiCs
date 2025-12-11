#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    const char* src = "int foo(){return 1;} int main(){int x=0; return x+foo();}\n";
    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("tokens.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed on tokens case\n");
        return 1;
    }
    if (res.token_count == 0) {
        fprintf(stderr, "expected token spans\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.symbol_count < 2) {
        fprintf(stderr, "expected symbol capture for foo and main\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);
    return 0;
}
