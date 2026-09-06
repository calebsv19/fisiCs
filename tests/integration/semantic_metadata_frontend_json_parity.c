#include <stdio.h>
#include <stdlib.h>

#include "Compiler/semantic_metadata_json.h"
#include "fisics_frontend.h"

static char* read_file(const char* path, size_t* length) {
    FILE* file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END) != 0) {
        if (file) fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    char* data = (char*)malloc((size_t)size + 1u);
    if (!data) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    data[size] = '\0';
    *length = (size_t)size;
    return data;
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    size_t length = 0;
    char* source = read_file(argv[1], &length);
    char* path = realpath(argv[1], NULL);
    if (!source || !path) {
        free(source);
        free(path);
        return 1;
    }

    FisicsFrontendOptions options = {0};
    options.overlay_features = FISICS_OVERLAY_SEMANTIC_METADATA;
    FisicsAnalysisResultEx result = {
        .abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION,
        .struct_size = sizeof(FisicsAnalysisResultEx)
    };
    bool ok = fisics_analyze_buffer_ex(path, source, length, &options, &result);
    if (ok && !result.analysis.contract.partial && !result.analysis.contract.fatal) {
        ok = fisics_semantic_metadata_write_json_atomic(
            argv[2], path, result.analysis.contract.source_hash,
            (size_t)result.analysis.contract.source_length,
            result.semantic_metadata_records,
            result.semantic_metadata_record_count,
            result.semantic_metadata_edges,
            result.semantic_metadata_edge_count);
    }
    fisics_free_analysis_result_ex(&result);
    free(source);
    free(path);
    return ok ? 0 : 1;
}
