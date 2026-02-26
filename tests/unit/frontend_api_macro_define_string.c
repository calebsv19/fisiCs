#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src =
        "const char* shader_root(void) { return VK_RENDERER_SHADER_ROOT; }\n"
        "int main(void) { return shader_root()[0] == '\\0'; }\n";

    const char* defs[] = {
        "-DVK_RENDERER_SHADER_ROOT=\"$(VK_RENDERER_RESOLVED_DIR)\""
    };
    FisicsFrontendOptions opts = {0};
    opts.macro_defines = defs;
    opts.macro_define_count = 1;

    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("vk_macro_define.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    for (size_t i = 0; i < res.diag_count; ++i) {
        const FisicsDiagnostic* d = &res.diagnostics[i];
        if (d->message && strstr(d->message, "Undeclared identifier")) {
            fprintf(stderr, "unexpected undeclared identifier diagnostic: %s\n", d->message);
            fisics_free_analysis_result(&res);
            return 1;
        }
    }

    fisics_free_analysis_result(&res);
    return 0;
}
