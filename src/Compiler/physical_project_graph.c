// SPDX-License-Identifier: Apache-2.0

#include "Compiler/physical_project_graph.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/Units/units_parser.h"
#include "Extensions/Units/units_registry.h"

#define PHYSICAL_INPUT_MAX_BYTES (64u * 1024u * 1024u)
#define PHYSICAL_TOTAL_INPUT_MAX_BYTES (512u * 1024u * 1024u)
#define PHYSICAL_MAX_ITEMS 262144u
#define PHYSICAL_MAX_QUANTITY_KINDS 65536u

typedef struct SemanticInput {
    char* project_id;
    char** source_paths;
    uint64_t* source_hashes;
    size_t* source_lengths;
    size_t source_count;
    char** external_targets;
    size_t external_count;
    FisicsPhysicalSemanticRecord* records;
    size_t record_count;
    FisicsPhysicalSemanticEdge* edges;
    size_t edge_count;
} SemanticInput;

static void diag(FisicsSemanticProjectDiagnostic* d, const char* fmt, ...) {
    if (!d || d->message[0]) return;
    va_list args; va_start(args, fmt);
    vsnprintf(d->message, sizeof(d->message), fmt, args);
    va_end(args);
}

static char* dup_text(const char* text) {
    if (!text) return NULL;
    size_t n = strlen(text) + 1u;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, text, n);
    return out;
}

static bool exact_fields(const FisicsJsonValue* object,
                         const char* const* fields, size_t count) {
    if (!object || object->type != FISICS_JSON_OBJECT || object->as.object.count != count) return false;
    for (size_t i = 0; i < count; ++i) if (!fisics_json_object_get(object, fields[i])) return false;
    return true;
}

static bool parse_hex64(const FisicsJsonValue* value, uint64_t* out) {
    if (!value || value->type != FISICS_JSON_STRING || strlen(value->as.text) != 16u) return false;
    uint64_t result = 0;
    for (size_t i = 0; i < 16u; ++i) {
        char c = value->as.text[i]; unsigned digit;
        if (c >= '0' && c <= '9') digit = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (unsigned)(c - 'a' + 10);
        else return false;
        result = result * 16u + digit;
    }
    *out = result; return true;
}

static bool valid_range(int sl, int sc, int el, int ec) {
    return sl > 0 && sc > 0 && el > 0 && ec > 0 && (el > sl || (el == sl && ec >= sc));
}

static bool canonical_name(const char* text) {
    const char* detail = NULL;
    return text && fisics_semantic_metadata_is_canonical_name(text, &detail);
}

static bool read_json_file(const char* path, FisicsJsonValue** out,
                           FisicsSemanticProjectDiagnostic* d) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        diag(d, "physical project input is not a regular file: %s", path ? path : "<null>"); return false;
    }
    if (st.st_size < 0 || (uint64_t)st.st_size > PHYSICAL_INPUT_MAX_BYTES) {
        diag(d, "physical project input exceeds %u bytes: %s", PHYSICAL_INPUT_MAX_BYTES, path); return false;
    }
    FILE* f = fopen(path, "rb");
    if (!f) { diag(d, "failed to read physical project input: %s", path); return false; }
    size_t n = (size_t)st.st_size;
    char* bytes = (char*)malloc(n + 1u);
    bool ok = bytes && fread(bytes, 1u, n, f) == n && fgetc(f) == EOF && !ferror(f);
    if (fclose(f) != 0) ok = false;
    if (!ok) { free(bytes); diag(d, "failed to read complete physical project input: %s", path); return false; }
    bytes[n] = '\0'; FisicsJsonDiagnostic jd = {0};
    ok = fisics_json_parse(bytes, n, out, &jd);
    free(bytes);
    if (!ok) diag(d, "invalid physical project JSON %s: %s", path, jd.message);
    return ok;
}

static bool add_input_size(const char* path, uint64_t* total,
                           FisicsSemanticProjectDiagnostic* d) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) {
        diag(d, "cannot stat physical project input: %s", path ? path : "<null>");
        return false;
    }
    uint64_t size = (uint64_t)st.st_size;
    if (size > PHYSICAL_INPUT_MAX_BYTES ||
        *total > (uint64_t)PHYSICAL_TOTAL_INPUT_MAX_BYTES - size) {
        diag(d, "physical project inputs exceed aggregate %u byte limit",
             PHYSICAL_TOTAL_INPUT_MAX_BYTES);
        return false;
    }
    *total += size;
    return true;
}

static bool verify_source(const char* path, size_t length, uint64_t expected,
                          FisicsSemanticProjectDiagnostic* d) {
    char* canonical = realpath(path, NULL);
    if (!canonical || strcmp(canonical, path) != 0) {
        free(canonical); diag(d, "physical units source is missing or not canonical: %s", path); return false;
    }
    free(canonical);
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || (uint64_t)st.st_size != length) {
        diag(d, "stale physical units source length: %s", path); return false;
    }
    FILE* f = fopen(path, "rb");
    if (!f) { diag(d, "failed to read physical units source: %s", path); return false; }
    uint64_t hash = UINT64_C(1469598103934665603); size_t total = 0; unsigned char buffer[16384];
    while (!feof(f)) {
        size_t count = fread(buffer, 1u, sizeof(buffer), f); total += count;
        for (size_t i = 0; i < count; ++i) { hash ^= buffer[i]; hash *= UINT64_C(1099511628211); }
        if (ferror(f)) { fclose(f); diag(d, "failed while hashing physical units source: %s", path); return false; }
    }
    if (fclose(f) != 0 || total != length || hash != expected) {
        diag(d, "stale physical units source hash: %s", path); return false;
    }
    return true;
}

static bool parse_source_range(const FisicsJsonValue* source, char** path,
                               int* sl, int* sc, int* el, int* ec) {
    static const char* const fields[] = {"file","start_line","start_column","end_line","end_column"};
    const FisicsJsonValue* file = fisics_json_object_get(source, "file");
    if (!exact_fields(source, fields, 5u) || !file || file->type != FISICS_JSON_STRING ||
        !fisics_json_integer_int(fisics_json_object_get(source, "start_line"), sl) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "start_column"), sc) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "end_line"), el) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "end_column"), ec) ||
        !valid_range(*sl, *sc, *el, *ec)) return false;
    *path = dup_text(file->as.text); return *path != NULL;
}

static void free_semantic_input(SemanticInput* in) {
    if (!in) return;
    free(in->project_id);
    for (size_t i = 0; i < in->source_count; ++i) free(in->source_paths[i]);
    free(in->source_paths); free(in->source_hashes); free(in->source_lengths);
    for (size_t i = 0; i < in->external_count; ++i) free(in->external_targets[i]);
    free(in->external_targets);
    for (size_t i = 0; i < in->record_count; ++i) {
        FisicsPhysicalSemanticRecord* r = &in->records[i];
        free(r->source_identity); free(r->authored_id); free(r->symbol_stable_id);
        free(r->symbol_name); free(r->source_path);
    }
    free(in->records);
    for (size_t i = 0; i < in->edge_count; ++i) {
        free(in->edges[i].kind); free(in->edges[i].target_id);
    }
    free(in->edges); memset(in, 0, sizeof(*in));
}

static bool parse_semantic_graph(const char* path, SemanticInput* out,
                                 FisicsSemanticProjectDiagnostic* d) {
    memset(out, 0, sizeof(*out)); FisicsJsonValue* root = NULL;
    if (!read_json_file(path, &root, d)) return false;
    static const char* const root_fields[] = {"schema","schema_version","producer","project","complete",
        "translation_units","allowed_external_targets","records","edges"};
    uint64_t version = 0;
    const FisicsJsonValue* schema = fisics_json_object_get(root, "schema");
    const FisicsJsonValue* complete = fisics_json_object_get(root, "complete");
    if (!exact_fields(root, root_fields, 9u) || !schema || schema->type != FISICS_JSON_STRING ||
        strcmp(schema->as.text, "fisiCs.semantic_project_graph") != 0 ||
        !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"), &version) || version != 1u ||
        !complete || complete->type != FISICS_JSON_BOOL || !complete->as.boolean) {
        diag(d, "physical composition requires complete semantic project graph schema v1: %s", path); goto fail;
    }
    const FisicsJsonValue* project = fisics_json_object_get(root, "project");
    static const char* const project_fields[] = {"id","manifest_path"};
    const FisicsJsonValue* project_id = fisics_json_object_get(project, "id");
    if (!exact_fields(project, project_fields, 2u) || !project_id || project_id->type != FISICS_JSON_STRING ||
        !canonical_name(project_id->as.text)) { diag(d, "semantic project graph has invalid project identity"); goto fail; }
    out->project_id = dup_text(project_id->as.text);
    const FisicsJsonValue* tus = fisics_json_object_get(root, "translation_units");
    const FisicsJsonValue* ext = fisics_json_object_get(root, "allowed_external_targets");
    const FisicsJsonValue* records = fisics_json_object_get(root, "records");
    const FisicsJsonValue* edges = fisics_json_object_get(root, "edges");
    if (!out->project_id || !tus || tus->type != FISICS_JSON_ARRAY || tus->as.array.count == 0u ||
        tus->as.array.count > FISICS_PHYSICAL_PROJECT_MAX_UNITS_RECEIPTS ||
        !ext || ext->type != FISICS_JSON_ARRAY || !records || records->type != FISICS_JSON_ARRAY ||
        records->as.array.count > PHYSICAL_MAX_ITEMS || !edges || edges->type != FISICS_JSON_ARRAY ||
        edges->as.array.count > PHYSICAL_MAX_ITEMS) { diag(d, "semantic project graph exceeds physical v1 limits"); goto fail; }
    out->source_count = tus->as.array.count;
    out->source_paths = (char**)calloc(out->source_count, sizeof(char*));
    out->source_hashes = (uint64_t*)calloc(out->source_count, sizeof(uint64_t));
    out->source_lengths = (size_t*)calloc(out->source_count, sizeof(size_t));
    if (!out->source_paths || !out->source_hashes || !out->source_lengths) goto oom;
    for (size_t i = 0; i < out->source_count; ++i) {
        const FisicsJsonValue* tu = tus->as.array.items[i];
        const FisicsJsonValue* source = fisics_json_object_get(tu, "source");
        static const char* const source_fields[] = {"path","hash_fnv1a64","length_bytes"};
        const FisicsJsonValue* p = fisics_json_object_get(source, "path");
        if (!source || !exact_fields(source, source_fields, 3u) || !p || p->type != FISICS_JSON_STRING ||
            !parse_hex64(fisics_json_object_get(source, "hash_fnv1a64"), &out->source_hashes[i]) ||
            !fisics_json_integer_size(fisics_json_object_get(source, "length_bytes"), &out->source_lengths[i])) {
            diag(d, "semantic project translation_units[%zu] has invalid source receipt", i); goto fail;
        }
        out->source_paths[i] = dup_text(p->as.text); if (!out->source_paths[i]) goto oom;
    }
    out->external_count = ext->as.array.count;
    out->external_targets = (char**)calloc(out->external_count ? out->external_count : 1u, sizeof(char*));
    if (!out->external_targets) goto oom;
    for (size_t i = 0; i < out->external_count; ++i) {
        const FisicsJsonValue* item = ext->as.array.items[i];
        if (!item || item->type != FISICS_JSON_STRING || !canonical_name(item->as.text)) {
            diag(d, "semantic project external target is invalid"); goto fail;
        }
        out->external_targets[i] = dup_text(item->as.text); if (!out->external_targets[i]) goto oom;
    }
    out->record_count = records->as.array.count;
    out->records = (FisicsPhysicalSemanticRecord*)calloc(out->record_count ? out->record_count : 1u, sizeof(*out->records));
    if (!out->records) goto oom;
    for (size_t i = 0; i < out->record_count; ++i) {
        const FisicsJsonValue* value = records->as.array.items[i];
        FisicsPhysicalSemanticRecord* r = &out->records[i]; size_t index = 0;
        const FisicsJsonValue* sid = fisics_json_object_get(value, "source_identity");
        const FisicsJsonValue* aid = fisics_json_object_get(value, "authored_id");
        const FisicsJsonValue* stable = fisics_json_object_get(value, "symbol_stable_id");
        const FisicsJsonValue* name = fisics_json_object_get(value, "symbol_name");
        if (!fisics_json_integer_size(fisics_json_object_get(value, "record_index"), &index) || index != i ||
            !sid || sid->type != FISICS_JSON_STRING || !name || name->type != FISICS_JSON_STRING ||
            !stable || (stable->type != FISICS_JSON_STRING && stable->type != FISICS_JSON_NULL) ||
            !aid || (aid->type != FISICS_JSON_STRING && aid->type != FISICS_JSON_NULL) ||
            !parse_source_range(fisics_json_object_get(value, "source"), &r->source_path,
                                &r->start_line,&r->start_column,&r->end_line,&r->end_column)) {
            diag(d, "semantic project records[%zu] is invalid", i); goto fail;
        }
        r->source_identity = dup_text(sid->as.text); r->symbol_name = dup_text(name->as.text);
        if (aid->type == FISICS_JSON_STRING) r->authored_id = dup_text(aid->as.text);
        if (stable->type == FISICS_JSON_STRING) r->symbol_stable_id = dup_text(stable->as.text);
        if (!r->source_identity || !r->symbol_name ||
            (aid->type == FISICS_JSON_STRING && !r->authored_id) ||
            (stable->type == FISICS_JSON_STRING && !r->symbol_stable_id)) goto oom;
    }
    out->edge_count = edges->as.array.count;
    out->edges = (FisicsPhysicalSemanticEdge*)calloc(out->edge_count ? out->edge_count : 1u, sizeof(*out->edges));
    if (!out->edges) goto oom;
    for (size_t i = 0; i < out->edge_count; ++i) {
        const FisicsJsonValue* value = edges->as.array.items[i];
        const FisicsJsonValue* kind = fisics_json_object_get(value, "kind");
        const FisicsJsonValue* target = fisics_json_object_get(value, "target_id");
        if (!fisics_json_integer_size(fisics_json_object_get(value, "source_record_index"), &out->edges[i].source_record_index) ||
            out->edges[i].source_record_index >= out->record_count || !kind || kind->type != FISICS_JSON_STRING ||
            (strcmp(kind->as.text,"produces") != 0 && strcmp(kind->as.text,"consumes") != 0) ||
            !target || target->type != FISICS_JSON_STRING || !canonical_name(target->as.text)) {
            diag(d, "semantic project edges[%zu] is invalid", i); goto fail;
        }
        out->edges[i].kind = dup_text(kind->as.text); out->edges[i].target_id = dup_text(target->as.text);
        if (!out->edges[i].kind || !out->edges[i].target_id) goto oom;
    }
    fisics_json_free(root); return true;
oom:
    diag(d, "out of memory reading semantic project graph");
fail:
    fisics_json_free(root); free_semantic_input(out); return false;
}

static void free_binding(FisicsPhysicalUnitsBinding* b) {
    if (!b) return;
    free(b->binding_id); free(b->source_path); free(b->symbol_stable_id);
    free(b->symbol_name); free(b->dimension_text); free(b->unit_name);
    free(b->unit_symbol); free(b->unit_family); free(b->scale_binary64);
    free(b->offset_binary64); memset(b, 0, sizeof(*b));
}

static bool parse_dimension(const FisicsJsonValue* value, char** text, int8_t dim[8]) {
    static const char* const fields[] = {"text","vector"};
    const FisicsJsonValue* t = fisics_json_object_get(value, "text");
    const FisicsJsonValue* vector = fisics_json_object_get(value, "vector");
    if (!exact_fields(value, fields, 2u) || !t || t->type != FISICS_JSON_STRING || !t->as.text[0] ||
        !vector || vector->type != FISICS_JSON_ARRAY || vector->as.array.count != 8u) return false;
    for (size_t i = 0; i < 8u; ++i) {
        int n = 0; if (!fisics_json_integer_int(vector->as.array.items[i], &n) || n < -128 || n > 127) return false;
        dim[i] = (int8_t)n;
    }
    *text = dup_text(t->as.text); return *text != NULL;
}

static bool parse_units_artifact(const char* path, FisicsPhysicalSourceReceipt* receipt,
                                 FisicsPhysicalUnitsBinding** bindings, size_t* binding_count,
                                 FisicsSemanticProjectDiagnostic* d) {
    FisicsJsonValue* root = NULL;
    if (!read_json_file(path, &root, d)) return false;
    static const char* const fields[] = {"schema","schema_version","producer","analysis_contract","source",
        "dimension_system","unit_registry","overlay_enabled","complete","attachments"};
    uint64_t version = 0;
    const FisicsJsonValue* schema = fisics_json_object_get(root,"schema");
    const FisicsJsonValue* complete = fisics_json_object_get(root,"complete");
    const FisicsJsonValue* overlay = fisics_json_object_get(root,"overlay_enabled");
    const FisicsJsonValue* system = fisics_json_object_get(root,"dimension_system");
    if (!exact_fields(root, fields, 10u) || !schema || schema->type != FISICS_JSON_STRING ||
        strcmp(schema->as.text,"fisiCs.units_metadata") != 0 ||
        !fisics_json_integer_u64(fisics_json_object_get(root,"schema_version"),&version) || version != 1u ||
        !complete || complete->type != FISICS_JSON_BOOL || !complete->as.boolean ||
        !overlay || overlay->type != FISICS_JSON_BOOL || !overlay->as.boolean ||
        !system || system->type != FISICS_JSON_STRING || strcmp(system->as.text,"fisiCs.si_dim8.v1") != 0) {
        diag(d,"physical composition requires complete enabled units metadata schema v1: %s",path); goto fail;
    }
    const FisicsJsonValue* source = fisics_json_object_get(root,"source");
    static const char* const source_fields[] = {"path","hash_fnv1a64","length_bytes"};
    const FisicsJsonValue* source_path = fisics_json_object_get(source,"path");
    if (!exact_fields(source,source_fields,3u) || !source_path || source_path->type != FISICS_JSON_STRING ||
        !parse_hex64(fisics_json_object_get(source,"hash_fnv1a64"),&receipt->source_hash) ||
        !fisics_json_integer_size(fisics_json_object_get(source,"length_bytes"),&receipt->source_length)) {
        diag(d,"units metadata source receipt is invalid: %s",path); goto fail;
    }
    const FisicsJsonValue* registry = fisics_json_object_get(root,"unit_registry");
    static const char* const registry_fields[] = {"id","fingerprint_fnv1a64"};
    const FisicsJsonValue* registry_id = fisics_json_object_get(registry,"id");
    if (!exact_fields(registry,registry_fields,2u) || !registry_id || registry_id->type != FISICS_JSON_STRING ||
        strcmp(registry_id->as.text,"fisiCs.unit_registry") != 0 ||
        !parse_hex64(fisics_json_object_get(registry,"fingerprint_fnv1a64"),&receipt->registry_fingerprint)) {
        diag(d,"units metadata registry receipt is invalid: %s",path); goto fail;
    }
    receipt->artifact_path = dup_text(path); receipt->source_path = dup_text(source_path->as.text);
    if (!receipt->artifact_path || !receipt->source_path ||
        !verify_source(receipt->source_path,receipt->source_length,receipt->source_hash,d)) goto fail;
    const FisicsJsonValue* array = fisics_json_object_get(root,"attachments");
    if (!array || array->type != FISICS_JSON_ARRAY || array->as.array.count > PHYSICAL_MAX_ITEMS) {
        diag(d,"units metadata attachments exceed v1 limits: %s",path); goto fail;
    }
    FisicsPhysicalUnitsBinding* items = (FisicsPhysicalUnitsBinding*)calloc(
        array->as.array.count ? array->as.array.count : 1u,sizeof(*items));
    if (!items) { diag(d,"out of memory reading units attachments"); goto fail; }
    for (size_t i = 0; i < array->as.array.count; ++i) {
        FisicsPhysicalUnitsBinding* b = &items[i]; const FisicsJsonValue* value = array->as.array.items[i];
        static const char* const af[] = {"binding_id","symbol_stable_id","symbol_name","source","dimension","unit"};
        const FisicsJsonValue* id = fisics_json_object_get(value,"binding_id");
        const FisicsJsonValue* stable = fisics_json_object_get(value,"symbol_stable_id");
        const FisicsJsonValue* name = fisics_json_object_get(value,"symbol_name");
        if (!exact_fields(value,af,6u) || !id || id->type != FISICS_JSON_STRING || !id->as.text[0] ||
            !stable || (stable->type != FISICS_JSON_STRING && stable->type != FISICS_JSON_NULL) ||
            !name || name->type != FISICS_JSON_STRING || !name->as.text[0] ||
            !parse_source_range(fisics_json_object_get(value,"source"),&b->source_path,
                                &b->start_line,&b->start_column,&b->end_line,&b->end_column) ||
            strcmp(b->source_path,receipt->source_path) != 0 ||
            !parse_dimension(fisics_json_object_get(value,"dimension"),&b->dimension_text,b->dimension)) {
            diag(d,"units metadata attachment %zu is invalid: %s",i,path); goto attachment_fail;
        }
        b->binding_id=dup_text(id->as.text); b->symbol_name=dup_text(name->as.text);
        if (stable->type==FISICS_JSON_STRING) b->symbol_stable_id=dup_text(stable->as.text);
        if (!b->binding_id||!b->symbol_name||(stable->type==FISICS_JSON_STRING&&!b->symbol_stable_id)) goto attachment_oom;
        const FisicsJsonValue* unit=fisics_json_object_get(value,"unit");
        if (unit->type==FISICS_JSON_OBJECT) {
            static const char* const uf[]={"source_text","name","symbol","family","conversion_model","scale_binary64","offset_binary64"};
            const FisicsJsonValue* un=fisics_json_object_get(unit,"name");
            const FisicsJsonValue* us=fisics_json_object_get(unit,"symbol");
            const FisicsJsonValue* fam=fisics_json_object_get(unit,"family");
            const FisicsJsonValue* model=fisics_json_object_get(unit,"conversion_model");
            const FisicsJsonValue* scale=fisics_json_object_get(unit,"scale_binary64");
            const FisicsJsonValue* offset=fisics_json_object_get(unit,"offset_binary64");
            if (!exact_fields(unit,uf,7u)||!un||un->type!=FISICS_JSON_STRING||!us||us->type!=FISICS_JSON_STRING||
                !fam||fam->type!=FISICS_JSON_STRING||!model||model->type!=FISICS_JSON_STRING||
                (strcmp(model->as.text,"linear")!=0&&strcmp(model->as.text,"affine")!=0)||
                !scale||scale->type!=FISICS_JSON_STRING||!offset||offset->type!=FISICS_JSON_STRING) {
                diag(d,"units metadata attachment %zu unit is invalid",i); goto attachment_fail;
            }
            char* end=NULL; errno=0; double sv=strtod(scale->as.text,&end);
            if(errno||!end||*end||!isfinite(sv)||sv<=0.0){diag(d,"units metadata scale is invalid");goto attachment_fail;}
            errno=0; double ov=strtod(offset->as.text,&end);
            if(errno||!end||*end||!isfinite(ov)){diag(d,"units metadata offset is invalid");goto attachment_fail;}
            b->unit_name=dup_text(un->as.text); b->unit_symbol=dup_text(us->as.text); b->unit_family=dup_text(fam->as.text);
            b->scale_binary64=dup_text(scale->as.text); b->offset_binary64=dup_text(offset->as.text);
            if(!b->unit_name||!b->unit_symbol||!b->unit_family||!b->scale_binary64||!b->offset_binary64) goto attachment_oom;
            const FisicsUnitDef* def=NULL;
            if(!fisics_unit_lookup(b->unit_name,&def)||!def||strcmp(def->symbol,b->unit_symbol)!=0||
               strcmp(fisics_dim_family_name(def->family),b->unit_family)!=0||
               memcmp(def->dim.e,b->dimension,8u)!=0||def->scale_to_canonical!=sv||def->offset_to_canonical!=ov){
                diag(d,"units metadata attachment %zu disagrees with current unit registry",i);goto attachment_fail;
            }
        } else if (unit->type!=FISICS_JSON_NULL) { diag(d,"units metadata attachment %zu unit must be object or null",i);goto attachment_fail; }
        continue;
attachment_oom:
        diag(d,"out of memory reading units attachment");
attachment_fail:
        for(size_t j=0;j<array->as.array.count;++j) free_binding(&items[j]); free(items); goto fail;
    }
    *bindings=items; *binding_count=array->as.array.count; fisics_json_free(root); return true;
fail:
    fisics_json_free(root); free(receipt->artifact_path); free(receipt->source_path); memset(receipt,0,sizeof(*receipt)); return false;
}

static void free_quantity(FisicsPhysicalQuantityKind* q) {
    free(q->id); free(q->dimension_system); free(q->dimension_text); free(q->catalog_path);
    memset(q,0,sizeof(*q));
}

static void free_contract(FisicsPhysicalContract* c) {
    free(c->contract_id); free(c->subject_id); free(c->quantity_kind); free(c->expected_unit);
    free(c->value_form); free(c->tensor_shape); free(c->reference_kind);
    free(c->reference_subject); free(c->period_binary64); free(c->frame_id); free(c->catalog_path);
    free(c->conversion_model); free(c->conversion_scale_binary64); free(c->conversion_offset_binary64);
    memset(c,0,sizeof(*c));
}

static bool copy_nullable_string(const FisicsJsonValue* value, char** out) {
    if (!value) return false;
    if (value->type==FISICS_JSON_NULL) { *out=NULL; return true; }
    if (value->type!=FISICS_JSON_STRING||!value->as.text[0]) return false;
    *out=dup_text(value->as.text); return *out!=NULL;
}

static bool parse_catalog(const char* path,
                          FisicsPhysicalQuantityKind** quantities,size_t* quantity_count,
                          FisicsPhysicalContract** contracts,size_t* contract_count,
                          FisicsSemanticProjectDiagnostic* d) {
    FisicsJsonValue* root=NULL; if(!read_json_file(path,&root,d)) return false;
    static const char* const fields[]={"schema","schema_version","catalog_id","quantity_kinds","contracts"};
    uint64_t version=0; const FisicsJsonValue* schema=fisics_json_object_get(root,"schema");
    const FisicsJsonValue* catalog_id=fisics_json_object_get(root,"catalog_id");
    const FisicsJsonValue* qs=fisics_json_object_get(root,"quantity_kinds");
    const FisicsJsonValue* cs=fisics_json_object_get(root,"contracts");
    if(!exact_fields(root,fields,5u)||!schema||schema->type!=FISICS_JSON_STRING||
       strcmp(schema->as.text,"fisiCs.physical_contract_catalog")!=0||
       !fisics_json_integer_u64(fisics_json_object_get(root,"schema_version"),&version)||version!=1u||
       !catalog_id||catalog_id->type!=FISICS_JSON_STRING||!canonical_name(catalog_id->as.text)||
       !qs||qs->type!=FISICS_JSON_ARRAY||!cs||cs->type!=FISICS_JSON_ARRAY||
       qs->as.array.count>PHYSICAL_MAX_QUANTITY_KINDS||cs->as.array.count>PHYSICAL_MAX_ITEMS||
       *quantity_count>PHYSICAL_MAX_QUANTITY_KINDS-qs->as.array.count||*contract_count>PHYSICAL_MAX_ITEMS-cs->as.array.count){
        diag(d,"invalid or oversized physical contract catalog: %s",path);goto fail;
    }
    size_t qbase=*quantity_count,cbase=*contract_count;
    if(qs->as.array.count){
        FisicsPhysicalQuantityKind* qgrown=(FisicsPhysicalQuantityKind*)realloc(*quantities,(qbase+qs->as.array.count)*sizeof(**quantities));
        if(!qgrown){diag(d,"out of memory reading quantity kinds");goto fail;}
        *quantities=qgrown;
        memset(*quantities+qbase,0,qs->as.array.count*sizeof(**quantities));
    }
    *quantity_count=qbase+qs->as.array.count;
    for(size_t i=0;i<qs->as.array.count;++i){
        FisicsPhysicalQuantityKind* q=&(*quantities)[qbase+i];const FisicsJsonValue* v=qs->as.array.items[i];
        static const char* const qf[]={"id","dimension_system","dimension_vector","dimension_text"};
        const FisicsJsonValue* id=fisics_json_object_get(v,"id");const FisicsJsonValue* sys=fisics_json_object_get(v,"dimension_system");
        const FisicsJsonValue* vec=fisics_json_object_get(v,"dimension_vector");const FisicsJsonValue* text=fisics_json_object_get(v,"dimension_text");
        if(!exact_fields(v,qf,4u)||!id||id->type!=FISICS_JSON_STRING||!canonical_name(id->as.text)||
           !sys||sys->type!=FISICS_JSON_STRING||strcmp(sys->as.text,"fisiCs.si_dim8.v1")!=0||
           !vec||vec->type!=FISICS_JSON_ARRAY||vec->as.array.count!=8u||!text||text->type!=FISICS_JSON_STRING||!text->as.text[0]){
            diag(d,"quantity_kinds[%zu] is invalid in %s",i,path);goto fail_added;
        }
        for(size_t j=0;j<8u;++j){int n=0;if(!fisics_json_integer_int(vec->as.array.items[j],&n)||n<-128||n>127){diag(d,"quantity dimension exponent is invalid");goto fail_added;}q->dimension[j]=(int8_t)n;}
        q->id=dup_text(id->as.text);q->dimension_system=dup_text(sys->as.text);q->dimension_text=dup_text(text->as.text);q->catalog_path=dup_text(path);
        if(!q->id||!q->dimension_system||!q->dimension_text||!q->catalog_path){diag(d,"out of memory reading quantity kind");goto fail_added;}
        FisicsDim8 parsed_dim = {{0}};
        char* parse_error = NULL;
        if (!fisics_units_parse_dim_expr(q->dimension_text, &parsed_dim, &parse_error) ||
            memcmp(parsed_dim.e, q->dimension, 8u) != 0) {
            diag(d, "quantity kind %s dimension_text disagrees with dimension_vector%s%s",
                 q->id, parse_error ? ": " : "", parse_error ? parse_error : "");
            free(parse_error);
            goto fail_added;
        }
        free(parse_error);
    }
    if(cs->as.array.count){
        FisicsPhysicalContract* cgrown=(FisicsPhysicalContract*)realloc(*contracts,(cbase+cs->as.array.count)*sizeof(**contracts));
        if(!cgrown){diag(d,"out of memory reading physical contracts");goto fail_added;}
        *contracts=cgrown;
        memset(*contracts+cbase,0,cs->as.array.count*sizeof(**contracts));
    }
    *contract_count=cbase+cs->as.array.count;
    for(size_t i=0;i<cs->as.array.count;++i){
        FisicsPhysicalContract* c=&(*contracts)[cbase+i];const FisicsJsonValue* v=cs->as.array.items[i];
        static const char* const cf[]={"contract_id","subject","quantity_kind","unit_policy","value_form","reference_semantics","frame_id"};
        const FisicsJsonValue* id=fisics_json_object_get(v,"contract_id");const FisicsJsonValue* subject=fisics_json_object_get(v,"subject");
        const FisicsJsonValue* quantity=fisics_json_object_get(v,"quantity_kind");
        if(!exact_fields(v,cf,7u)||!id||id->type!=FISICS_JSON_STRING||!canonical_name(id->as.text)||
           !quantity||quantity->type!=FISICS_JSON_STRING||!canonical_name(quantity->as.text)){
            diag(d,"contracts[%zu] has invalid identity in %s",i,path);goto fail_added;
        }
        static const char* const sf[]={"kind","id"};const FisicsJsonValue* sk=fisics_json_object_get(subject,"kind");const FisicsJsonValue* si=fisics_json_object_get(subject,"id");
        if(!exact_fields(subject,sf,2u)||!sk||sk->type!=FISICS_JSON_STRING||!si||si->type!=FISICS_JSON_STRING||!si->as.text[0]){diag(d,"contract subject is invalid");goto fail_added;}
        if(strcmp(sk->as.text,"semantic_id")==0){if(!canonical_name(si->as.text)){diag(d,"semantic contract subject is not canonical");goto fail_added;}c->subject_kind=FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID;}
        else if(strcmp(sk->as.text,"source_identity")==0)c->subject_kind=FISICS_PHYSICAL_SUBJECT_SOURCE_IDENTITY;
        else{diag(d,"unknown physical subject kind");goto fail_added;}
        const FisicsJsonValue* policy=fisics_json_object_get(v,"unit_policy");static const char* const pf[]={"mode","unit"};
        const FisicsJsonValue* mode=fisics_json_object_get(policy,"mode");const FisicsJsonValue* unit=fisics_json_object_get(policy,"unit");
        if(!exact_fields(policy,pf,2u)||!mode||mode->type!=FISICS_JSON_STRING||!unit){diag(d,"physical unit_policy is invalid");goto fail_added;}
        if(strcmp(mode->as.text,"dimension_only")==0){c->unit_policy=FISICS_PHYSICAL_UNIT_DIMENSION_ONLY;if(unit->type!=FISICS_JSON_NULL){diag(d,"dimension_only unit must be null");goto fail_added;}}
        else if(strcmp(mode->as.text,"exact_unit")==0)c->unit_policy=FISICS_PHYSICAL_UNIT_EXACT;
        else if(strcmp(mode->as.text,"convertible_unit")==0)c->unit_policy=FISICS_PHYSICAL_UNIT_CONVERTIBLE;
        else{diag(d,"unknown physical unit policy");goto fail_added;}
        if(c->unit_policy!=FISICS_PHYSICAL_UNIT_DIMENSION_ONLY){
            const FisicsUnitDef* def=NULL;
            if(unit->type!=FISICS_JSON_STRING||!fisics_unit_lookup(unit->as.text,&def)||!def){diag(d,"physical contract expected unit is unknown");goto fail_added;}
            c->expected_unit=dup_text(def->name);if(!c->expected_unit)goto fail_added;
        }
        const FisicsJsonValue* form=fisics_json_object_get(v,"value_form");static const char* const vf[]={"kind","component_count","tensor_rank","tensor_shape"};
        const FisicsJsonValue* fk=fisics_json_object_get(form,"kind");const FisicsJsonValue* components=fisics_json_object_get(form,"component_count");
        const FisicsJsonValue* rank=fisics_json_object_get(form,"tensor_rank");const FisicsJsonValue* shape=fisics_json_object_get(form,"tensor_shape");
        if(!exact_fields(form,vf,4u)||!fk||fk->type!=FISICS_JSON_STRING||!components||!rank||!shape||shape->type!=FISICS_JSON_ARRAY){diag(d,"physical value_form is invalid");goto fail_added;}
        if(strcmp(fk->as.text,"scalar")&&strcmp(fk->as.text,"vector")&&strcmp(fk->as.text,"tensor")&&strcmp(fk->as.text,"opaque")){diag(d,"unknown physical value form");goto fail_added;}
        c->value_form=dup_text(fk->as.text);if(!c->value_form)goto fail_added;
        if(components->type==FISICS_JSON_NUMBER){if(!fisics_json_integer_size(components,&c->component_count)||c->component_count==0||c->component_count>65536u)goto bad_form;c->has_component_count=true;}else if(components->type!=FISICS_JSON_NULL)goto bad_form;
        if(rank->type==FISICS_JSON_NUMBER){if(!fisics_json_integer_size(rank,&c->tensor_rank)||c->tensor_rank==0||c->tensor_rank>8u)goto bad_form;c->has_tensor_rank=true;}else if(rank->type!=FISICS_JSON_NULL)goto bad_form;
        c->tensor_shape_count=shape->as.array.count;if(c->tensor_shape_count){c->tensor_shape=(size_t*)calloc(c->tensor_shape_count,sizeof(size_t));if(!c->tensor_shape)goto fail_added;for(size_t j=0;j<c->tensor_shape_count;++j)if(!fisics_json_integer_size(shape->as.array.items[j],&c->tensor_shape[j])||c->tensor_shape[j]==0||c->tensor_shape[j]>65536u)goto bad_form;}
        if((!strcmp(c->value_form,"vector")&&(!c->has_component_count||c->has_tensor_rank||c->tensor_shape_count))||
           (!strcmp(c->value_form,"tensor")&&(!c->has_tensor_rank||c->tensor_shape_count!=c->tensor_rank||c->has_component_count))||
           ((strcmp(c->value_form,"vector")&&strcmp(c->value_form,"tensor"))&&(c->has_component_count||c->has_tensor_rank||c->tensor_shape_count)))goto bad_form;
        const FisicsJsonValue* ref=fisics_json_object_get(v,"reference_semantics");static const char* const rf[]={"kind","reference_subject","period_binary64"};
        const FisicsJsonValue* rk=fisics_json_object_get(ref,"kind");const FisicsJsonValue* rs=fisics_json_object_get(ref,"reference_subject");const FisicsJsonValue* period=fisics_json_object_get(ref,"period_binary64");
        if(!exact_fields(ref,rf,3u)||!rk||rk->type!=FISICS_JSON_STRING||
           (strcmp(rk->as.text,"linear")&&strcmp(rk->as.text,"absolute")&&strcmp(rk->as.text,"difference")&&strcmp(rk->as.text,"relative")&&strcmp(rk->as.text,"cyclic"))||
           !copy_nullable_string(rs,&c->reference_subject)||!copy_nullable_string(period,&c->period_binary64))goto bad_ref;
        c->reference_kind=dup_text(rk->as.text);if(!c->reference_kind)goto fail_added;
        if(!strcmp(c->reference_kind,"relative")){if(!c->reference_subject||!canonical_name(c->reference_subject)||c->period_binary64)goto bad_ref;}
        else if(!strcmp(c->reference_kind,"cyclic")){char* end=NULL;double p=c->period_binary64?strtod(c->period_binary64,&end):0.0;if(!c->period_binary64||!end||*end||!isfinite(p)||p<=0.0||c->reference_subject)goto bad_ref;}
        else if(c->reference_subject||c->period_binary64)goto bad_ref;
        if(!copy_nullable_string(fisics_json_object_get(v,"frame_id"),&c->frame_id)||(c->frame_id&&!canonical_name(c->frame_id))){diag(d,"physical frame_id is invalid");goto fail_added;}
        c->contract_id=dup_text(id->as.text);c->subject_id=dup_text(si->as.text);c->quantity_kind=dup_text(quantity->as.text);c->catalog_path=dup_text(path);
        if(!c->contract_id||!c->subject_id||!c->quantity_kind||!c->catalog_path)goto fail_added;
        continue;
bad_form: diag(d,"physical value_form invariants are invalid");goto fail_added;
bad_ref: diag(d,"physical reference_semantics invariants are invalid");goto fail_added;
    }
    fisics_json_free(root);return true;
fail_added:
    for(size_t i=qbase;i<*quantity_count;++i)free_quantity(&(*quantities)[i]);*quantity_count=qbase;
    for(size_t i=cbase;i<*contract_count;++i)free_contract(&(*contracts)[i]);*contract_count=cbase;
fail:
    fisics_json_free(root);return false;
}

static int compare_receipts(const void* l,const void* r){
    const FisicsPhysicalSourceReceipt* a=l;const FisicsPhysicalSourceReceipt* b=r;
    int c=strcmp(a->source_path,b->source_path);return c?c:strcmp(a->artifact_path,b->artifact_path);
}
static int compare_bindings(const void* l,const void* r){
    const FisicsPhysicalUnitsBinding* a=l;const FisicsPhysicalUnitsBinding* b=r;int c=strcmp(a->source_path,b->source_path);if(c)return c;
    if(a->start_line!=b->start_line)return a->start_line<b->start_line?-1:1;if(a->start_column!=b->start_column)return a->start_column<b->start_column?-1:1;
    c=strcmp(a->symbol_name,b->symbol_name);return c?c:strcmp(a->binding_id,b->binding_id);
}
static int compare_quantities(const void* l,const void* r){
    const FisicsPhysicalQuantityKind* a=l;const FisicsPhysicalQuantityKind* b=r;
    int c=strcmp(a->id,b->id);return c?c:strcmp(a->catalog_path,b->catalog_path);
}
static int compare_contract_ids(const void* l,const void* r){return strcmp(((const FisicsPhysicalContract*)l)->contract_id,((const FisicsPhysicalContract*)r)->contract_id);}
static int compare_contract_subjects(const void* l,const void* r){
    const FisicsPhysicalContract* a=l;const FisicsPhysicalContract* b=r;if(a->subject_kind!=b->subject_kind)return a->subject_kind<b->subject_kind?-1:1;
    int c=strcmp(a->subject_id,b->subject_id);return c?c:strcmp(a->contract_id,b->contract_id);
}

static bool same_quantity(const FisicsPhysicalQuantityKind* a,const FisicsPhysicalQuantityKind* b){
    return strcmp(a->dimension_system,b->dimension_system)==0&&memcmp(a->dimension,b->dimension,8u)==0&&strcmp(a->dimension_text,b->dimension_text)==0;
}

static bool canonicalize_quantities(FisicsPhysicalProjectGraph* g,FisicsSemanticProjectDiagnostic* d){
    qsort(g->quantity_kinds,g->quantity_kind_count,sizeof(*g->quantity_kinds),compare_quantities);
    size_t write=0;
    for(size_t i=0;i<g->quantity_kind_count;++i){
        if(write&&strcmp(g->quantity_kinds[write-1u].id,g->quantity_kinds[i].id)==0){
            if(!same_quantity(&g->quantity_kinds[write-1u],&g->quantity_kinds[i])){diag(d,"conflicting quantity kind %s in %s and %s",g->quantity_kinds[i].id,g->quantity_kinds[write-1u].catalog_path,g->quantity_kinds[i].catalog_path);return false;}
            free_quantity(&g->quantity_kinds[i]);continue;
        }
        if(write!=i){g->quantity_kinds[write]=g->quantity_kinds[i];memset(&g->quantity_kinds[i],0,sizeof(g->quantity_kinds[i]));}
        ++write;
    }
    g->quantity_kind_count=write;return true;
}

static FisicsPhysicalQuantityKind* find_quantity(FisicsPhysicalProjectGraph* g,const char* id){
    size_t lo=0,hi=g->quantity_kind_count;while(lo<hi){size_t mid=lo+(hi-lo)/2u;int c=strcmp(g->quantity_kinds[mid].id,id);if(c<0)lo=mid+1u;else hi=mid;}
    return lo<g->quantity_kind_count&&strcmp(g->quantity_kinds[lo].id,id)==0?&g->quantity_kinds[lo]:NULL;
}

static bool external_exists(const SemanticInput* in,const char* id){
    for(size_t i=0;i<in->external_count;++i)if(strcmp(in->external_targets[i],id)==0)return true;return false;
}
static size_t record_for_subject(const SemanticInput* in,FisicsPhysicalSubjectKind kind,const char* id){
    for(size_t i=0;i<in->record_count;++i){const char* candidate=kind==FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID?in->records[i].authored_id:in->records[i].source_identity;if(candidate&&strcmp(candidate,id)==0)return i;}return SIZE_MAX;
}
static bool identity_exists(const SemanticInput* in,const char* id){return record_for_subject(in,FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID,id)!=SIZE_MAX||external_exists(in,id);}

static void join_bindings(FisicsPhysicalProjectGraph* g){
    for(size_t i=0;i<g->binding_count;++i){FisicsPhysicalUnitsBinding* b=&g->bindings[i];
        for(size_t j=0;j<g->semantic_record_count;++j){FisicsPhysicalSemanticRecord* r=&g->semantic_records[j];
            bool stable_matches=!b->symbol_stable_id||(r->symbol_stable_id&&strcmp(r->symbol_stable_id,b->symbol_stable_id)==0);
            if(stable_matches&&strcmp(r->source_path,b->source_path)==0&&strcmp(r->symbol_name,b->symbol_name)==0&&r->start_line==b->start_line&&r->start_column==b->start_column&&r->end_line==b->end_line&&r->end_column==b->end_column){b->record_index=j;b->has_record_index=true;break;}
        }
    }
}

static size_t binding_for_record(const FisicsPhysicalProjectGraph* g,size_t record){
    size_t found=SIZE_MAX;for(size_t i=0;i<g->binding_count;++i)if(g->bindings[i].has_record_index&&g->bindings[i].record_index==record){if(found!=SIZE_MAX)return SIZE_MAX-1u;found=i;}return found;
}

static bool make_conversion(FisicsPhysicalContract* c,const FisicsPhysicalUnitsBinding* b,const FisicsUnitDef* target,FisicsSemanticProjectDiagnostic* d){
    char* end=NULL;double source_scale=strtod(b->scale_binary64,&end);if(!end||*end)return false;
    double source_offset=strtod(b->offset_binary64,&end);if(!end||*end)return false;
    double scale=source_scale/target->scale_to_canonical;
    double offset=strcmp(c->reference_kind,"difference")==0?0.0:(source_offset-target->offset_to_canonical)/target->scale_to_canonical;
    char scale_text[64],offset_text[64];snprintf(scale_text,sizeof(scale_text),"%a",scale);snprintf(offset_text,sizeof(offset_text),"%a",offset);
    c->conversion_model=dup_text(offset==0.0?"linear":"affine");c->conversion_scale_binary64=dup_text(scale_text);c->conversion_offset_binary64=dup_text(offset_text);
    if(!c->conversion_model||!c->conversion_scale_binary64||!c->conversion_offset_binary64){diag(d,"out of memory creating physical conversion recipe");return false;}return true;
}

static bool validate_contracts(FisicsPhysicalProjectGraph* g,const SemanticInput* in,
                               const FisicsPhysicalProjectManifest* manifest,
                               FisicsSemanticProjectDiagnostic* d){
    qsort(g->contracts,g->contract_count,sizeof(*g->contracts),compare_contract_ids);
    for(size_t i=1;i<g->contract_count;++i)if(strcmp(g->contracts[i-1u].contract_id,g->contracts[i].contract_id)==0){diag(d,"duplicate physical contract_id %s in %s and %s",g->contracts[i].contract_id,g->contracts[i-1u].catalog_path,g->contracts[i].catalog_path);return false;}
    qsort(g->contracts,g->contract_count,sizeof(*g->contracts),compare_contract_subjects);
    for(size_t i=1;i<g->contract_count;++i)if(g->contracts[i-1u].subject_kind==g->contracts[i].subject_kind&&strcmp(g->contracts[i-1u].subject_id,g->contracts[i].subject_id)==0){diag(d,"duplicate physical contract subject %s in %s and %s",g->contracts[i].subject_id,g->contracts[i-1u].catalog_path,g->contracts[i].catalog_path);return false;}
    for(size_t i=0;i<g->contract_count;++i){FisicsPhysicalContract* c=&g->contracts[i];FisicsPhysicalQuantityKind* q=find_quantity(g,c->quantity_kind);
        if(!q){diag(d,"physical contract %s references unknown quantity kind %s",c->contract_id,c->quantity_kind);return false;}
        if(c->frame_id&&!identity_exists(in,c->frame_id)){diag(d,"physical contract %s references missing frame %s",c->contract_id,c->frame_id);return false;}
        if(c->reference_subject&&!identity_exists(in,c->reference_subject)){diag(d,"physical contract %s references missing subject %s",c->contract_id,c->reference_subject);return false;}
        size_t record=record_for_subject(in,c->subject_kind,c->subject_id);
        if(record==SIZE_MAX){if(c->subject_kind==FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID&&external_exists(in,c->subject_id)){c->state=FISICS_PHYSICAL_STATE_EXTERNAL_DECLARED;g->external_contract_count++;continue;}diag(d,"physical contract %s subject does not resolve: %s",c->contract_id,c->subject_id);return false;}
        c->record_index=record;c->has_record_index=true;size_t binding=binding_for_record(g,record);
        if(binding==SIZE_MAX-1u){diag(d,"physical contract %s has ambiguous units evidence",c->contract_id);return false;}
        if(binding==SIZE_MAX){c->state=FISICS_PHYSICAL_STATE_SOURCE_UNVERIFIED;g->unverified_contract_count++;if(manifest->evidence_policy==FISICS_PHYSICAL_EVIDENCE_REQUIRE_SOURCE){diag(d,"physical contract %s lacks required source units evidence",c->contract_id);return false;}continue;}
        c->binding_index=binding;c->has_binding_index=true;FisicsPhysicalUnitsBinding* b=&g->bindings[binding];
        if(memcmp(b->dimension,q->dimension,8u)!=0){diag(d,"physical dimension mismatch for contract %s",c->contract_id);return false;}
        if(c->unit_policy==FISICS_PHYSICAL_UNIT_DIMENSION_ONLY){c->state=FISICS_PHYSICAL_STATE_SOURCE_COMPATIBLE;continue;}
        if(!b->unit_name){diag(d,"physical contract %s requires concrete unit evidence",c->contract_id);return false;}
        const FisicsUnitDef* target=NULL;if(!fisics_unit_lookup(c->expected_unit,&target)||!target||memcmp(target->dim.e,q->dimension,8u)!=0){diag(d,"physical contract %s expected unit disagrees with quantity dimension",c->contract_id);return false;}
        if(c->unit_policy==FISICS_PHYSICAL_UNIT_EXACT){if(strcmp(b->unit_name,target->name)!=0){diag(d,"physical exact-unit mismatch for contract %s",c->contract_id);return false;}}
        else {if(strcmp(b->unit_family,fisics_dim_family_name(target->family))!=0){diag(d,"physical unit family mismatch for contract %s",c->contract_id);return false;}if(!make_conversion(c,b,target,d))return false;}
        c->state=FISICS_PHYSICAL_STATE_SOURCE_COMPATIBLE;
    }
    return true;
}

static size_t contract_for_target(const FisicsPhysicalProjectGraph* g,const char* target){
    for(size_t i=0;i<g->contract_count;++i)if(g->contracts[i].subject_kind==FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID&&strcmp(g->contracts[i].subject_id,target)==0)return i;return SIZE_MAX;
}

static bool apply_coverage(FisicsPhysicalProjectGraph* g,const FisicsPhysicalProjectManifest* manifest,FisicsSemanticProjectDiagnostic* d){
    for(size_t i=0;i<g->semantic_edge_count;++i){size_t index=contract_for_target(g,g->semantic_edges[i].target_id);if(index==SIZE_MAX){g->uncovered_target_count++;if(manifest->coverage_policy!=FISICS_PHYSICAL_COVERAGE_ALLOW_PARTIAL){diag(d,"missing required physical contract for relationship target %s",g->semantic_edges[i].target_id);return false;}}else{g->semantic_edges[i].target_contract_index=index;g->semantic_edges[i].has_target_contract_index=true;}}
    if(manifest->coverage_policy==FISICS_PHYSICAL_COVERAGE_ALL_AUTHORED){for(size_t i=0;i<g->semantic_record_count;++i)if(g->semantic_records[i].authored_id&&contract_for_target(g,g->semantic_records[i].authored_id)==SIZE_MAX){g->uncovered_record_count++;diag(d,"missing required physical contract for authored record %s",g->semantic_records[i].authored_id);return false;}}
    else for(size_t i=0;i<g->semantic_record_count;++i)if(g->semantic_records[i].authored_id&&contract_for_target(g,g->semantic_records[i].authored_id)==SIZE_MAX)g->uncovered_record_count++;
    return true;
}

void fisics_physical_project_graph_free(FisicsPhysicalProjectGraph* g){
    if(!g)return;free(g->project_id);free(g->manifest_path);free(g->semantic_graph_path);free(g->semantic_project_id);
    for(size_t i=0;i<g->receipt_count;++i){free(g->receipts[i].artifact_path);free(g->receipts[i].source_path);}free(g->receipts);
    for(size_t i=0;i<g->binding_count;++i)free_binding(&g->bindings[i]);free(g->bindings);
    for(size_t i=0;i<g->quantity_kind_count;++i)free_quantity(&g->quantity_kinds[i]);free(g->quantity_kinds);
    for(size_t i=0;i<g->contract_count;++i)free_contract(&g->contracts[i]);free(g->contracts);
    for(size_t i=0;i<g->semantic_record_count;++i){FisicsPhysicalSemanticRecord* r=&g->semantic_records[i];free(r->source_identity);free(r->authored_id);free(r->symbol_stable_id);free(r->symbol_name);free(r->source_path);}free(g->semantic_records);
    for(size_t i=0;i<g->semantic_edge_count;++i){free(g->semantic_edges[i].kind);free(g->semantic_edges[i].target_id);}free(g->semantic_edges);memset(g,0,sizeof(*g));
}

bool fisics_physical_project_graph_build(const FisicsPhysicalProjectManifest* manifest,FisicsPhysicalProjectGraph* out,FisicsSemanticProjectDiagnostic* d){
    if(d)d->message[0]='\0';if(!manifest||!out){diag(d,"invalid physical project graph arguments");return false;}memset(out,0,sizeof(*out));SemanticInput in={0};
    if (getenv("FISICS_TEST_FAIL_PHYSICAL_PROJECT_GRAPH_ALLOC")) {
        diag(d, "injected physical project graph allocation failure");
        return false;
    }
    uint64_t total_input_bytes = 0;
    if (!add_input_size(manifest->semantic_graph_path, &total_input_bytes, d)) return false;
    for (size_t i = 0; i < manifest->units_metadata_path_count; ++i)
        if (!add_input_size(manifest->units_metadata_paths[i], &total_input_bytes, d)) return false;
    for (size_t i = 0; i < manifest->catalog_path_count; ++i)
        if (!add_input_size(manifest->catalog_paths[i], &total_input_bytes, d)) return false;
    if(!parse_semantic_graph(manifest->semantic_graph_path,&in,d))return false;
    if(strcmp(in.project_id,manifest->project_id)!=0){diag(d,"physical project_id %s does not match semantic project %s",manifest->project_id,in.project_id);goto fail;}
    if(manifest->units_metadata_path_count!=in.source_count){diag(d,"physical project requires exactly one units receipt for every semantic translation unit");goto fail;}
    out->project_id=dup_text(manifest->project_id);out->manifest_path=dup_text(manifest->manifest_path);out->semantic_graph_path=dup_text(manifest->semantic_graph_path);out->semantic_project_id=dup_text(in.project_id);
    out->receipt_count=manifest->units_metadata_path_count;out->receipts=(FisicsPhysicalSourceReceipt*)calloc(out->receipt_count,sizeof(*out->receipts));
    if(!out->project_id||!out->manifest_path||!out->semantic_graph_path||!out->semantic_project_id||!out->receipts){diag(d,"out of memory building physical project graph");goto fail;}
    for(size_t i=0;i<out->receipt_count;++i){FisicsPhysicalUnitsBinding* local=NULL;size_t count=0;if(!parse_units_artifact(manifest->units_metadata_paths[i],&out->receipts[i],&local,&count,d))goto fail;
        if(out->binding_count>PHYSICAL_MAX_ITEMS-count){diag(d,"physical units bindings exceed v1 limit");for(size_t j=0;j<count;++j)free_binding(&local[j]);free(local);goto fail;}
        if(count){FisicsPhysicalUnitsBinding* grown=(FisicsPhysicalUnitsBinding*)realloc(out->bindings,(out->binding_count+count)*sizeof(*out->bindings));if(!grown){for(size_t j=0;j<count;++j)free_binding(&local[j]);free(local);diag(d,"out of memory merging units bindings");goto fail;}out->bindings=grown;memcpy(out->bindings+out->binding_count,local,count*sizeof(*local));out->binding_count+=count;}free(local);
    }
    qsort(out->receipts,out->receipt_count,sizeof(*out->receipts),compare_receipts);qsort(out->bindings,out->binding_count,sizeof(*out->bindings),compare_bindings);
    out->registry_fingerprint=fisics_unit_registry_fingerprint();
    for(size_t i=0;i<out->receipt_count;++i){if(out->receipts[i].registry_fingerprint!=out->registry_fingerprint){diag(d,"units registry fingerprint mismatch: %s",out->receipts[i].artifact_path);goto fail;}bool found=false;for(size_t j=0;j<in.source_count;++j)if(strcmp(out->receipts[i].source_path,in.source_paths[j])==0&&out->receipts[i].source_hash==in.source_hashes[j]&&out->receipts[i].source_length==in.source_lengths[j]){found=true;break;}if(!found){diag(d,"units receipt does not match a semantic translation unit: %s",out->receipts[i].source_path);goto fail;}if(i&&strcmp(out->receipts[i-1u].source_path,out->receipts[i].source_path)==0){diag(d,"duplicate units source receipt: %s",out->receipts[i].source_path);goto fail;}}
    for(size_t i=1;i<out->binding_count;++i)if(strcmp(out->bindings[i-1u].binding_id,out->bindings[i].binding_id)==0){diag(d,"duplicate units binding_id: %s",out->bindings[i].binding_id);goto fail;}
    for(size_t i=0;i<manifest->catalog_path_count;++i)if(!parse_catalog(manifest->catalog_paths[i],&out->quantity_kinds,&out->quantity_kind_count,&out->contracts,&out->contract_count,d))goto fail;
    if(!canonicalize_quantities(out,d))goto fail;
    SemanticInput lookup=in;
    out->semantic_records=in.records;out->semantic_record_count=in.record_count;in.records=NULL;in.record_count=0;
    out->semantic_edges=in.edges;out->semantic_edge_count=in.edge_count;in.edges=NULL;in.edge_count=0;
    lookup.records=out->semantic_records;lookup.record_count=out->semantic_record_count;
    lookup.edges=out->semantic_edges;lookup.edge_count=out->semantic_edge_count;
    join_bindings(out);
    if(!validate_contracts(out,&lookup,manifest,d)||!apply_coverage(out,manifest,d))goto fail;
    free_semantic_input(&in);return true;
fail:
    free_semantic_input(&in);fisics_physical_project_graph_free(out);return false;
}
