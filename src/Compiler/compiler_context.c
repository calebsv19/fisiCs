#include "Compiler/compiler_context.h"
#include "Syntax/target_layout.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Syntax/target_layout.h"
#include "Utils/logging.h"

#define CC_GROW_CAP(cap) ((cap) < 8 ? 8 : (cap) * 2)

// -------------------- tiny StringSet --------------------
static void cc_stringset_free(CCStringSet* s) {
    if (!s) return;
    for (size_t i = 0; i < s->count; ++i) free(s->items[i]);
    free(s->items);
    s->items = NULL; s->count = s->capacity = 0;
}

static bool cc_stringset_contains(const CCStringSet* s, const char* key) {
    if (!s || !key) return false;
    for (size_t i = 0; i < s->count; ++i) {
        if (strcmp(s->items[i], key) == 0) return true;
    }
    return false;
}

static bool cc_stringset_add(CCStringSet* s, const char* key) {
    if (!s || !key) return false;
    if (cc_stringset_contains(s, key)) return true; // idempotent

    if (s->count == s->capacity) {
        size_t new_cap = CC_GROW_CAP(s->capacity);
        char** n = (char**)realloc(s->items, new_cap * sizeof(char*));
        if (!n) return false;
        s->items = n;
        s->capacity = new_cap;
    }
    s->items[s->count] = strdup(key);
    if (!s->items[s->count]) return false;
    s->count++;
    return true;
}

// -------------------- Tag tables --------------------
static void cc_tag_table_free(CCTagTable* table) {
    if (!table) return;
    for (size_t i = 0; i < table->count; ++i) {
        if (table->records[i].fieldLayouts) {
            for (size_t j = 0; j < table->records[i].fieldLayoutCount; ++j) {
                free(table->records[i].fieldLayouts[j].name);
            }
            free(table->records[i].fieldLayouts);
            table->records[i].fieldLayouts = NULL;
        }
        free(table->records[i].name);
    }
    free(table->records);
    table->records = NULL;
    table->count = table->capacity = 0;
}

static CCTagRecord* cc_tag_lookup_mut(CCTagTable* table, const char* name) {
    if (!table || !name) return NULL;
    for (size_t i = 0; i < table->count; ++i) {
        if (strcmp(table->records[i].name, name) == 0) {
            return &table->records[i];
        }
    }
    return NULL;
}

static const CCTagRecord* cc_tag_lookup_const(const CCTagTable* table, const char* name) {
    if (!table || !name) return NULL;
    for (size_t i = 0; i < table->count; ++i) {
        if (strcmp(table->records[i].name, name) == 0) {
            return &table->records[i];
        }
    }
    return NULL;
}

static bool cc_tag_table_add(CCTagTable* table, const char* name) {
    if (!table || !name) return false;
    if (cc_tag_lookup_mut(table, name)) return true;
    if (table->count == table->capacity) {
        size_t new_cap = CC_GROW_CAP(table->capacity);
        CCTagRecord* recs = realloc(table->records, new_cap * sizeof(CCTagRecord));
        if (!recs) return false;
        table->records = recs;
        table->capacity = new_cap;
    }
    CCTagRecord* rec = &table->records[table->count++];
    rec->name = strdup(name);
    if (!rec->name) {
        --table->count;
        return false;
    }
    rec->isDefined = false;
    rec->fingerprint = 0;
    rec->definition = NULL;
    rec->hasLayout = false;
    rec->layoutSize = 0;
    rec->layoutAlign = 0;
    rec->computingLayout = false;
    rec->fieldLayouts = NULL;
    rec->fieldLayoutCount = 0;
    rec->fieldLayoutCapacity = 0;
    return true;
}

// -------------------- builtins --------------------
static const char* CC_BUILTIN_LIST[] = {
    // C core
    "void","_Bool","bool",
    "char","signed char","unsigned char",
    "short","short int","signed short","signed short int",
    "unsigned short","unsigned short int",
    "int","signed","signed int",
    "unsigned","unsigned int",
    "long","long int","signed long","signed long int",
    "unsigned long","unsigned long int",
    "long long","long long int",
    "unsigned long long","unsigned long long int",
    "float","double","long double",

    // common typedef’d integer widths (treat as builtin names for now)
    "int8_t","int16_t","int32_t","int64_t",
    "uint8_t","uint16_t","uint32_t","uint64_t",
    "size_t","ssize_t","intptr_t","uintptr_t","ptrdiff_t",
};

static void cc_builtins_init(CCBuiltins* b) {
    b->names = CC_BUILTIN_LIST;
    b->count = sizeof(CC_BUILTIN_LIST)/sizeof(CC_BUILTIN_LIST[0]);
}

static bool cc_builtins_has(const CCBuiltins* b, const char* name) {
    for (size_t i = 0; i < b->count; ++i) {
        if (strcmp(b->names[i], name) == 0) return true;
    }
    return false;
}

// -------------------- API --------------------
CompilerContext* cc_create(void) {
    CompilerContext* c = (CompilerContext*)calloc(1, sizeof(*c));
    if (!c) return NULL;
    cc_builtins_init(&c->builtins);
    include_graph_init(&c->includeGraph);
    c->dl_canary_front = 0xdeadbeefcafebabeULL;
    c->dl_canary_back  = 0xabad1dea12345678ULL;
    c->targetLayout = NULL;
    c->warnIgnoredInterop = true;
    c->errorIgnoredInterop = false;
    return c;
}

void cc_destroy(CompilerContext* ctx) {
    if (!ctx) return;
    cc_stringset_free(&ctx->typedef_names);
    cc_tag_table_free(&ctx->tag_struct);
    cc_tag_table_free(&ctx->tag_union);
    cc_tag_table_free(&ctx->tag_enum);
    include_graph_destroy(&ctx->includeGraph);
    compiler_diagnostics_clear(ctx);
    cc_clear_token_spans(ctx);
    cc_clear_symbols(ctx);
    cc_clear_includes(ctx);
    free(ctx->targetTriple);
    free(ctx->targetLayout);
    free(ctx->dataLayout);
    // builtins are static literals; nothing to free.
    free(ctx);
}

void cc_seed_builtins(CompilerContext* ctx) {
    if (!ctx) return;
    // Seed common typedefs so the parser recognizes them as type names even
    // without headers.
    const char* typedefs[] = {
        "size_t",
        "ssize_t",
        "intptr_t",
        "uintptr_t",
        "ptrdiff_t",
        "bool",
        "wchar_t",
        "wint_t",
        "sig_atomic_t",
        "locale_t",
    };
    for (size_t i = 0; i < sizeof(typedefs) / sizeof(typedefs[0]); ++i) {
        cc_add_typedef(ctx, typedefs[i]);
    }
}

bool cc_is_typedef(const CompilerContext* ctx, const char* name) {
    return cc_stringset_contains(&ctx->typedef_names, name);
}
bool cc_add_typedef(CompilerContext* ctx, const char* name) {
    return cc_stringset_add(&ctx->typedef_names, name);
}

static CCTagTable* pick_tag_table(CompilerContext* ctx, CCTagKind k) {
    switch (k) {
        case CC_TAG_STRUCT: return &ctx->tag_struct;
        case CC_TAG_UNION:  return &ctx->tag_union;
        case CC_TAG_ENUM:   return &ctx->tag_enum;
        default: return NULL;
    }
}
static const CCTagTable* pick_tag_table_const(const CompilerContext* ctx, CCTagKind k) {
    switch (k) {
        case CC_TAG_STRUCT: return &ctx->tag_struct;
        case CC_TAG_UNION:  return &ctx->tag_union;
        case CC_TAG_ENUM:   return &ctx->tag_enum;
        default: return NULL;
    }
}

static bool cc_tag_exists_other_kind(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    if (!ctx || !name) return false;
    CCTagKind kinds[] = { CC_TAG_STRUCT, CC_TAG_UNION, CC_TAG_ENUM };
    for (size_t i = 0; i < sizeof(kinds)/sizeof(kinds[0]); ++i) {
        if (kinds[i] == kind) continue;
        const CCTagTable* table = pick_tag_table_const(ctx, kinds[i]);
        if (table && cc_tag_lookup_const(table, name)) {
            return true;
        }
    }
    return false;
}

bool cc_has_tag(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    return table ? (cc_tag_lookup_const(table, name) != NULL) : false;
}

static CCTagRecord* cc_tag_lookup_mut_named(CompilerContext* ctx, CCTagKind kind, const char* name) {
    CCTagTable* table = pick_tag_table(ctx, kind);
    return table ? cc_tag_lookup_mut(table, name) : NULL;
}

bool cc_add_tag(CompilerContext* ctx, CCTagKind kind, const char* name) {
    if (cc_tag_exists_other_kind(ctx, kind, name)) {
        return false;
    }
    CCTagTable* table = pick_tag_table(ctx, kind);
    return table ? cc_tag_table_add(table, name) : false;
}

CCTagDefineResult cc_define_tag(CompilerContext* ctx,
                                CCTagKind kind,
                                const char* name,
                                uint64_t fingerprint,
                                struct ASTNode* definition) {
    if (!ctx || !name) return CC_TAGDEF_CONFLICT;
    if (cc_tag_exists_other_kind(ctx, kind, name)) {
        return CC_TAGDEF_CONFLICT;
    }
    if (!cc_add_tag(ctx, kind, name)) {
        return CC_TAGDEF_CONFLICT;
    }
    CCTagTable* table = pick_tag_table(ctx, kind);
    if (!table) return CC_TAGDEF_CONFLICT;
    CCTagRecord* rec = cc_tag_lookup_mut(table, name);
    if (!rec) return CC_TAGDEF_CONFLICT;
    if (rec->isDefined) {
        if (rec->fingerprint == fingerprint) {
            if (!rec->definition && definition) {
                rec->definition = definition;
            }
            return CC_TAGDEF_MATCHING;
        }
        return CC_TAGDEF_CONFLICT;
    }
    rec->isDefined = true;
    rec->fingerprint = fingerprint;
    rec->definition = definition;
    rec->hasLayout = false;
    rec->layoutSize = 0;
    rec->layoutAlign = 0;
    rec->computingLayout = false;
    if (rec->fieldLayouts) {
        for (size_t i = 0; i < rec->fieldLayoutCount; ++i) {
            free(rec->fieldLayouts[i].name);
        }
        free(rec->fieldLayouts);
        rec->fieldLayouts = NULL;
        rec->fieldLayoutCount = 0;
        rec->fieldLayoutCapacity = 0;
    }
    return CC_TAGDEF_ADDED;
}

bool cc_tag_is_defined(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    const CCTagRecord* rec = cc_tag_lookup_const(table, name);
    return rec ? rec->isDefined : false;
}

struct ASTNode* cc_tag_definition(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    const CCTagRecord* rec = cc_tag_lookup_const(table, name);
    return rec ? rec->definition : NULL;
}

bool cc_set_tag_layout(CompilerContext* ctx, CCTagKind kind, const char* name, size_t size, size_t align) {
    CCTagRecord* rec = cc_tag_lookup_mut_named(ctx, kind, name);
    if (!rec) return false;
    rec->hasLayout = true;
    rec->layoutSize = size;
    rec->layoutAlign = align;
    rec->computingLayout = false;
    return true;
}

bool cc_get_tag_layout(const CompilerContext* ctx, CCTagKind kind, const char* name, size_t* sizeOut, size_t* alignOut) {
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    const CCTagRecord* rec = cc_tag_lookup_const(table, name);
    if (!rec || !rec->hasLayout) return false;
    if (sizeOut) *sizeOut = rec->layoutSize;
    if (alignOut) *alignOut = rec->layoutAlign;
    return true;
}

bool cc_tag_mark_computing(CompilerContext* ctx, CCTagKind kind, const char* name, bool computing) {
    CCTagRecord* rec = cc_tag_lookup_mut_named(ctx, kind, name);
    if (!rec) return false;
    rec->computingLayout = computing;
    return true;
}

bool cc_tag_is_computing(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    const CCTagRecord* rec = cc_tag_lookup_const(table, name);
    return rec ? rec->computingLayout : false;
}

bool cc_set_tag_field_layouts(CompilerContext* ctx,
                              CCTagKind kind,
                              const char* name,
                              const CCTagFieldLayout* layouts,
                              size_t count) {
    CCTagRecord* rec = cc_tag_lookup_mut_named(ctx, kind, name);
    if (!rec) return false;
    // Clear existing
    if (rec->fieldLayouts) {
        for (size_t i = 0; i < rec->fieldLayoutCount; ++i) {
            free(rec->fieldLayouts[i].name);
        }
        free(rec->fieldLayouts);
        rec->fieldLayouts = NULL;
        rec->fieldLayoutCount = rec->fieldLayoutCapacity = 0;
    }
    if (count == 0 || !layouts) return true;
    rec->fieldLayouts = (CCTagFieldLayout*)calloc(count, sizeof(CCTagFieldLayout));
    if (!rec->fieldLayouts) return false;
    rec->fieldLayoutCapacity = rec->fieldLayoutCount = count;
    for (size_t i = 0; i < count; ++i) {
        rec->fieldLayouts[i] = layouts[i];
        if (layouts[i].name) {
            rec->fieldLayouts[i].name = strdup(layouts[i].name);
            if (!rec->fieldLayouts[i].name) {
                rec->fieldLayoutCount = i;
                return false;
            }
        }
    }
    return true;
}

bool cc_get_tag_field_layouts(const CompilerContext* ctx,
                              CCTagKind kind,
                              const char* name,
                              const CCTagFieldLayout** layoutsOut,
                              size_t* countOut) {
    if (layoutsOut) *layoutsOut = NULL;
    if (countOut) *countOut = 0;
    const CCTagTable* table = pick_tag_table_const(ctx, kind);
    const CCTagRecord* rec = cc_tag_lookup_const(table, name);
    if (!rec || !rec->fieldLayouts) return false;
    if (layoutsOut) *layoutsOut = rec->fieldLayouts;
    if (countOut) *countOut = rec->fieldLayoutCount;
    return true;
}

bool cc_is_builtin_type(const CompilerContext* ctx, const char* name) {
    (void)ctx;
    return cc_builtins_has(&ctx->builtins, name);
}

bool cc_set_include_graph(CompilerContext* ctx, const IncludeGraph* graph) {
    if (!ctx) return false;
    include_graph_destroy(&ctx->includeGraph);
    return include_graph_clone(&ctx->includeGraph, graph);
}

const IncludeGraph* cc_get_include_graph(const CompilerContext* ctx) {
    return ctx ? &ctx->includeGraph : NULL;
}

bool cc_set_target_triple(CompilerContext* ctx, const char* triple) {
    if (!ctx) return false;
    if (ctx->dl_canary_front != 0xdeadbeefcafebabeULL || ctx->dl_canary_back != 0xabad1dea12345678ULL) {
        LOG_WARN("compiler", "dataLayout canary tripped in cc_set_target_triple");
    }
    free(ctx->targetTriple);
    ctx->targetTriple = triple ? strdup(triple) : NULL;
    return true;
}

const char* cc_get_target_triple(const CompilerContext* ctx) {
    return ctx ? ctx->targetTriple : NULL;
}

bool cc_set_target_layout(CompilerContext* ctx, const TargetLayout* layout) {
    if (!ctx) return false;
    if (ctx->dl_canary_front != 0xdeadbeefcafebabeULL || ctx->dl_canary_back != 0xabad1dea12345678ULL) {
        LOG_WARN("compiler", "dataLayout canary tripped in cc_set_target_layout");
    }
    free(ctx->targetLayout);
    if (layout) {
        ctx->targetLayout = (TargetLayout*)malloc(sizeof(TargetLayout));
        if (!ctx->targetLayout) return false;
        memcpy(ctx->targetLayout, layout, sizeof(TargetLayout));
    } else {
        ctx->targetLayout = NULL;
    }
    return true;
}

const TargetLayout* cc_get_target_layout(const CompilerContext* ctx) {
    return ctx ? ctx->targetLayout : NULL;
}

bool cc_set_data_layout(CompilerContext* ctx, const char* layout) {
    if (!ctx) return false;
    if (ctx->dl_canary_front != 0xdeadbeefcafebabeULL || ctx->dl_canary_back != 0xabad1dea12345678ULL) {
        LOG_WARN("compiler", "dataLayout canary tripped entering cc_set_data_layout");
    }
    if (layout && layout[0]) {
        size_t len = strlen(layout);
        LOG_WARN("compiler", "cc_set_data_layout called (len=%zu, starts='%c')", len, layout[0]);
    } else {
        LOG_WARN("compiler", "cc_set_data_layout called with null/empty layout");
    }
    free(ctx->dataLayout);
    ctx->dataLayout = layout ? strdup(layout) : NULL;
    return true;
}

const char* cc_get_data_layout(const CompilerContext* ctx) {
    return ctx ? ctx->dataLayout : NULL;
}

void cc_set_interop_diag(CompilerContext* ctx, bool warnIgnored, bool errorIgnored) {
    if (!ctx) return;
    ctx->warnIgnoredInterop = warnIgnored;
    ctx->errorIgnoredInterop = errorIgnored;
}

bool cc_warn_ignored_interop(const CompilerContext* ctx) {
    if (!ctx) return true;
    return ctx->warnIgnoredInterop;
}

bool cc_error_ignored_interop(const CompilerContext* ctx) {
    if (!ctx) return false;
    return ctx->errorIgnoredInterop;
}

// ---- Token spans ----
static void cc_token_spans_free(CompilerTokenSpans* spans) {
    if (!spans) return;
    free(spans->items);
    spans->items = NULL;
    spans->count = 0;
    spans->capacity = 0;
}

bool cc_set_token_spans(CompilerContext* ctx, const FisicsTokenSpan* spans, size_t count) {
    if (!ctx) return false;
    cc_token_spans_free(&ctx->tokenSpans);
    if (!spans || count == 0) {
        return true;
    }
    FisicsTokenSpan* copy = (FisicsTokenSpan*)malloc(count * sizeof(FisicsTokenSpan));
    if (!copy) return false;
    memcpy(copy, spans, count * sizeof(FisicsTokenSpan));
    ctx->tokenSpans.items = copy;
    ctx->tokenSpans.count = count;
    ctx->tokenSpans.capacity = count;
    return true;
}

bool cc_append_token_span(CompilerContext* ctx, const FisicsTokenSpan* span) {
    if (!ctx || !span) return false;
    CompilerTokenSpans* s = &ctx->tokenSpans;
    if (s->count == s->capacity) {
        size_t newCap = s->capacity ? s->capacity * 2 : 64;
        FisicsTokenSpan* grown = (FisicsTokenSpan*)realloc(s->items, newCap * sizeof(FisicsTokenSpan));
        if (!grown) return false;
        s->items = grown;
        s->capacity = newCap;
    }
    s->items[s->count++] = *span;
    return true;
}

const FisicsTokenSpan* cc_get_token_spans(const CompilerContext* ctx, size_t* countOut) {
    if (countOut) {
        *countOut = ctx ? ctx->tokenSpans.count : 0;
    }
    return ctx ? ctx->tokenSpans.items : NULL;
}

void cc_clear_token_spans(CompilerContext* ctx) {
    cc_token_spans_free(ctx ? &ctx->tokenSpans : NULL);
}

// ---- Symbols ----
static char* cc_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static void cc_symbols_free(CompilerSymbols* symbols) {
    if (!symbols) return;
    for (size_t i = 0; i < symbols->count; ++i) {
        free((char*)symbols->items[i].name);
        free((char*)symbols->items[i].file_path);
    }
    free(symbols->items);
    symbols->items = NULL;
    symbols->count = 0;
    symbols->capacity = 0;
}

bool cc_set_symbols(CompilerContext* ctx, const FisicsSymbol* symbols, size_t count) {
    if (!ctx) return false;
    cc_symbols_free(&ctx->symbols);
    if (!symbols || count == 0) {
        return true;
    }
    FisicsSymbol* copy = (FisicsSymbol*)calloc(count, sizeof(FisicsSymbol));
    if (!copy) return false;
    for (size_t i = 0; i < count; ++i) {
        copy[i] = symbols[i];
        if (symbols[i].name) {
            copy[i].name = cc_strdup(symbols[i].name);
            if (!copy[i].name) {
                cc_symbols_free(&(CompilerSymbols){ .items = copy, .count = i, .capacity = count });
                return false;
            }
        }
        if (symbols[i].file_path) {
            copy[i].file_path = cc_strdup(symbols[i].file_path);
            if (!copy[i].file_path) {
                cc_symbols_free(&(CompilerSymbols){ .items = copy, .count = i + 1, .capacity = count });
                return false;
            }
        }
    }
    ctx->symbols.items = copy;
    ctx->symbols.count = count;
    ctx->symbols.capacity = count;
    return true;
}

const FisicsSymbol* cc_get_symbols(const CompilerContext* ctx, size_t* countOut) {
    if (countOut) {
        *countOut = ctx ? ctx->symbols.count : 0;
    }
    return ctx ? ctx->symbols.items : NULL;
}

void cc_clear_symbols(CompilerContext* ctx) {
    cc_symbols_free(ctx ? &ctx->symbols : NULL);
}

// ---- Includes ----
static void cc_includes_free(CompilerIncludes* includes) {
    if (!includes) return;
    for (size_t i = 0; i < includes->count; ++i) {
        free((char*)includes->items[i].name);
        free((char*)includes->items[i].resolved_path);
    }
    free(includes->items);
    includes->items = NULL;
    includes->count = 0;
    includes->capacity = 0;
}

bool cc_set_includes(CompilerContext* ctx, const FisicsInclude* includes, size_t count) {
    if (!ctx) return false;
    cc_includes_free(&ctx->includes);
    if (!includes || count == 0) {
        return true;
    }
    FisicsInclude* copy = (FisicsInclude*)calloc(count, sizeof(FisicsInclude));
    if (!copy) return false;
    for (size_t i = 0; i < count; ++i) {
        copy[i] = includes[i];
        if (includes[i].name) {
            copy[i].name = cc_strdup(includes[i].name);
            if (!copy[i].name) {
                cc_includes_free(&(CompilerIncludes){ .items = copy, .count = i, .capacity = count });
                return false;
            }
        }
        if (includes[i].resolved_path) {
            copy[i].resolved_path = cc_strdup(includes[i].resolved_path);
            if (!copy[i].resolved_path) {
                cc_includes_free(&(CompilerIncludes){ .items = copy, .count = i + 1, .capacity = count });
                return false;
            }
        }
    }
    ctx->includes.items = copy;
    ctx->includes.count = count;
    ctx->includes.capacity = count;
    return true;
}

bool cc_append_include(CompilerContext* ctx, const FisicsInclude* include) {
    if (!ctx || !include) return false;
    CompilerIncludes* inc = &ctx->includes;
    if (inc->count == inc->capacity) {
        size_t newCap = inc->capacity ? inc->capacity * 2 : 16;
        FisicsInclude* grown = (FisicsInclude*)realloc(inc->items, newCap * sizeof(FisicsInclude));
        if (!grown) return false;
        inc->items = grown;
        inc->capacity = newCap;
    }
    FisicsInclude* dst = &inc->items[inc->count];
    *dst = *include;
    dst->name = NULL;
    dst->resolved_path = NULL;
    if (include->name) {
        dst->name = cc_strdup(include->name);
        if (!dst->name) return false;
    }
    if (include->resolved_path) {
        dst->resolved_path = cc_strdup(include->resolved_path);
        if (!dst->resolved_path) {
            free((char*)dst->name);
            dst->name = NULL;
            dst->resolved_path = NULL;
            return false;
        }
    }
    inc->count++;
    return true;
}

const FisicsInclude* cc_get_includes(const CompilerContext* ctx, size_t* countOut) {
    if (countOut) {
        *countOut = ctx ? ctx->includes.count : 0;
    }
    return ctx ? ctx->includes.items : NULL;
}

void cc_clear_includes(CompilerContext* ctx) {
    cc_includes_free(ctx ? &ctx->includes : NULL);
}

// ---- Translation unit ----
void cc_set_translation_unit(CompilerContext* ctx, struct ASTNode* root) {
    if (!ctx) return;
    ctx->translationUnit = root;
}

struct ASTNode* cc_get_translation_unit(const CompilerContext* ctx) {
    return ctx ? ctx->translationUnit : NULL;
}
