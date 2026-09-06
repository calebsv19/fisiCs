// SPDX-License-Identifier: Apache-2.0

#include "Compiler/units_metadata_json.h"

#include "Compiler/analysis_contract_version.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Extensions/Units/units_registry.h"

static bool json_string(FILE* out, const char* value) {
    if (fputc('"', out) == EOF) return false;
    const unsigned char* cursor = (const unsigned char*)(value ? value : "");
    for (; *cursor; ++cursor) {
        switch (*cursor) {
            case '"': if (fputs("\\\"", out) == EOF) return false; break;
            case '\\': if (fputs("\\\\", out) == EOF) return false; break;
            case '\b': if (fputs("\\b", out) == EOF) return false; break;
            case '\f': if (fputs("\\f", out) == EOF) return false; break;
            case '\n': if (fputs("\\n", out) == EOF) return false; break;
            case '\r': if (fputs("\\r", out) == EOF) return false; break;
            case '\t': if (fputs("\\t", out) == EOF) return false; break;
            default:
                if (*cursor < 0x20u) {
                    if (fprintf(out, "\\u%04x", (unsigned)*cursor) < 0) return false;
                } else if (fputc(*cursor, out) == EOF) return false;
        }
    }
    return fputc('"', out) != EOF;
}

static int compare_attachment_ptrs(const void* left, const void* right) {
    const FisicsUnitsAttachment* a = *(const FisicsUnitsAttachment* const*)left;
    const FisicsUnitsAttachment* b = *(const FisicsUnitsAttachment* const*)right;
    int cmp = strcmp(a->source_file_path ? a->source_file_path : "",
                     b->source_file_path ? b->source_file_path : "");
    if (cmp) return cmp;
    if (a->start_line != b->start_line) return a->start_line < b->start_line ? -1 : 1;
    if (a->start_col != b->start_col) return a->start_col < b->start_col ? -1 : 1;
    cmp = strcmp(a->symbol_name ? a->symbol_name : "", b->symbol_name ? b->symbol_name : "");
    if (cmp) return cmp;
    if (a->has_symbol_stable_id != b->has_symbol_stable_id) return a->has_symbol_stable_id ? 1 : -1;
    if (a->symbol_stable_id != b->symbol_stable_id) return a->symbol_stable_id < b->symbol_stable_id ? -1 : 1;
    return 0;
}

static bool write_binding_id(FILE* out, const char* source_path,
                             const FisicsUnitsAttachment* item) {
    char text[1024];
    int written = snprintf(text, sizeof(text), "binding:%s:%d:%d:%s:%" PRIu64,
                           source_path,
                           item->start_line, item->start_col,
                           item->symbol_name ? item->symbol_name : "",
                           item->has_symbol_stable_id ? item->symbol_stable_id : UINT64_C(0));
    return written > 0 && (size_t)written < sizeof(text) && json_string(out, text);
}

static bool write_attachment(FILE* out, const char* source_path,
                             const FisicsUnitsAttachment* item, bool final) {
    if (fputs("    {\n      \"binding_id\": ", out) == EOF ||
        !write_binding_id(out, source_path, item) ||
        fputs(",\n      \"symbol_stable_id\": ", out) == EOF) return false;
    if (item->has_symbol_stable_id) {
        if (fprintf(out, "\"%" PRIu64 "\"", item->symbol_stable_id) < 0) return false;
    } else if (fputs("null", out) == EOF) return false;
    if (fputs(",\n      \"symbol_name\": ", out) == EOF ||
        !json_string(out, item->symbol_name) ||
        fputs(",\n      \"source\": {\n        \"file\": ", out) == EOF ||
        !json_string(out, source_path) ||
        fprintf(out,
                ",\n        \"start_line\": %d,\n"
                "        \"start_column\": %d,\n"
                "        \"end_line\": %d,\n"
                "        \"end_column\": %d\n      },\n"
                "      \"dimension\": {\n        \"text\": ",
                item->start_line, item->start_col, item->end_line, item->end_col) < 0 ||
        !json_string(out, item->dim_text) ||
        fputs(",\n        \"vector\": [", out) == EOF) return false;
    for (size_t i = 0; i < FISICS_UNITS_DIM_SLOTS; ++i) {
        if (fprintf(out, "%s%d", i ? ", " : "", (int)item->dim[i]) < 0) return false;
    }
    if (fputs("]\n      },\n      \"unit\": ", out) == EOF) return false;
    if (!item->unit_resolved || !item->unit_name) {
        if (fputs("null", out) == EOF) return false;
    } else {
        const FisicsUnitDef* unit = NULL;
        if (!fisics_unit_lookup(item->unit_name, &unit) || !unit) return false;
        if (fputs("{\n        \"source_text\": ", out) == EOF ||
            !json_string(out, item->unit_source_text) ||
            fputs(",\n        \"name\": ", out) == EOF || !json_string(out, unit->name) ||
            fputs(",\n        \"symbol\": ", out) == EOF || !json_string(out, unit->symbol) ||
            fputs(",\n        \"family\": ", out) == EOF ||
            !json_string(out, fisics_dim_family_name(unit->family)) ||
            fprintf(out,
                    ",\n        \"conversion_model\": \"%s\",\n"
                    "        \"scale_binary64\": \"%a\",\n"
                    "        \"offset_binary64\": \"%a\"\n      }",
                    unit->offset_to_canonical == 0.0 ? "linear" : "affine",
                    unit->scale_to_canonical, unit->offset_to_canonical) < 0) return false;
    }
    return fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_document(FILE* out, const char* source_path, uint64_t source_hash,
                           size_t source_length, bool overlay_enabled,
                           const FisicsUnitsAttachment* attachments, size_t count) {
    const FisicsUnitsAttachment** order = NULL;
    if (count) {
        order = (const FisicsUnitsAttachment**)malloc(count * sizeof(*order));
        if (!order) return false;
        for (size_t i = 0; i < count; ++i) order[i] = &attachments[i];
        qsort(order, count, sizeof(*order), compare_attachment_ptrs);
    }
    bool ok = fputs("{\n  \"schema\": \"fisiCs.units_metadata\",\n"
                    "  \"schema_version\": 1,\n"
                    "  \"producer\": {\n    \"name\": \"fisiCs\",\n"
                    "    \"version\": \"" FISICS_VERSION "\"\n  },\n"
                    "  \"analysis_contract\": {\n"
                    "    \"id\": \"" FISICS_ANALYSIS_CONTRACT_ID "\",\n"
                    "    \"version\": \"" FISICS_ANALYSIS_CONTRACT_VERSION_STRING "\"\n  },\n"
                    "  \"source\": {\n    \"path\": ", out) != EOF &&
              json_string(out, source_path) &&
              fprintf(out, ",\n    \"hash_fnv1a64\": \"%016" PRIx64 "\",\n"
                           "    \"length_bytes\": %zu\n  },\n"
                           "  \"dimension_system\": \"fisiCs.si_dim8.v1\",\n"
                           "  \"unit_registry\": {\n"
                           "    \"id\": \"fisiCs.unit_registry\",\n"
                           "    \"fingerprint_fnv1a64\": \"%016" PRIx64 "\"\n  },\n"
                           "  \"overlay_enabled\": %s,\n"
                           "  \"complete\": true,\n"
                           "  \"attachments\": [\n",
                           source_hash, source_length, fisics_unit_registry_fingerprint(),
                           overlay_enabled ? "true" : "false") >= 0;
    for (size_t i = 0; ok && i < count; ++i) {
        ok = write_attachment(out, source_path, order[i], i + 1u == count);
    }
    if (ok) ok = fputs("  ]\n}\n", out) != EOF;
    free(order);
    return ok;
}

static char* canonical_destination(const char* path) {
    char* existing = realpath(path, NULL);
    if (existing) return existing;
    const char* slash = strrchr(path, '/');
    const char* leaf = path;
    char* directory = NULL;
    if (slash) {
        size_t count = (size_t)(slash - path);
        directory = count ? (char*)malloc(count + 1u) : strdup("/");
        if (directory && count) { memcpy(directory, path, count); directory[count] = '\0'; }
        leaf = slash + 1;
    } else directory = strdup(".");
    if (!directory || !leaf[0]) { free(directory); return NULL; }
    char* base = realpath(directory, NULL);
    free(directory);
    if (!base) return NULL;
    size_t count = strlen(base) + strlen(leaf) + 2u;
    char* result = (char*)malloc(count);
    if (result) snprintf(result, count, "%s%s%s", base,
                         strcmp(base, "/") == 0 ? "" : "/", leaf);
    free(base);
    return result;
}

bool fisics_units_metadata_write_json_atomic(
    const char* output_path, const char* source_path, uint64_t source_hash,
    size_t source_length, bool overlay_enabled,
    const FisicsUnitsAttachment* attachments, size_t attachment_count) {
    if (!output_path || !output_path[0] || !source_path || !source_path[0] ||
        (attachment_count && !attachments)) return false;
    char* canonical_output = canonical_destination(output_path);
    if (!canonical_output || strcmp(canonical_output, source_path) == 0) {
        free(canonical_output);
        return false;
    }
    free(canonical_output);
    size_t length = strlen(output_path);
    char* temporary = (char*)malloc(length + sizeof(".tmp.XXXXXX"));
    if (!temporary) return false;
    snprintf(temporary, length + sizeof(".tmp.XXXXXX"), "%s.tmp.XXXXXX", output_path);
    int descriptor = mkstemp(temporary);
    if (descriptor < 0) { free(temporary); return false; }
    FILE* out = fdopen(descriptor, "w");
    bool ok = out && write_document(out, source_path, source_hash, source_length,
                                    overlay_enabled, attachments, attachment_count);
    if (getenv("FISICS_TEST_FAIL_UNITS_METADATA_WRITE")) ok = false;
    if (out) {
        if (ok && fflush(out) != 0) ok = false;
        if (ok && fsync(descriptor) != 0) ok = false;
        if (fclose(out) != 0) ok = false;
    } else close(descriptor);
    if (ok && rename(temporary, output_path) != 0) ok = false;
    if (!ok) unlink(temporary);
    free(temporary);
    return ok;
}
