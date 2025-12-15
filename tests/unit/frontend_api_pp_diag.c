#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fisics_frontend.h"

static char* slurp(const char* path, size_t* outLen) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    if (outLen) *outLen = n;
    return buf;
}

int main(void) {
    const char* root = "tests/preprocessor/diag_header_path";
    const char* file = "tests/preprocessor/diag_header_path/main.c";
    size_t len = 0;
    char* src = slurp(file, &len);
    if (!src) {
        fprintf(stderr, "failed to read %s\n", file);
        return 1;
    }

    const char* include_paths[] = { root };
    FisicsFrontendOptions opts = {0};
    opts.include_paths = include_paths;
    opts.include_path_count = 1;
    // strict mode is fine; we just want diagnostics

    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer(file, src, len, &opts, &res)) {
        fprintf(stderr, "analysis failed unexpectedly\n");
        free(src);
        return 1;
    }

    int foundBad = 0;
    int foundMain = 0;
    for (size_t i = 0; i < res.diag_count; ++i) {
        const FisicsDiagnostic* d = &res.diagnostics[i];
        if (d->file_path && strstr(d->file_path, "bad.h")) {
            foundBad = 1;
        }
        if (d->file_path && strstr(d->file_path, "main.c")) {
            foundMain = 1;
        }
    }

    fisics_free_analysis_result(&res);
    free(src);

    if (!foundBad) {
        fprintf(stderr, "expected diagnostic to reference bad.h\n");
        return 1;
    }
    if (foundMain && !foundBad) {
        fprintf(stderr, "diagnostic pointed to main.c but not bad.h\n");
        return 1;
    }
    return 0;
}
