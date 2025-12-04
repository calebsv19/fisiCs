#include "Compiler/compiler_context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    return c;
}

void cc_destroy(CompilerContext* ctx) {
    if (!ctx) return;
    cc_stringset_free(&ctx->typedef_names);
    cc_tag_table_free(&ctx->tag_struct);
    cc_tag_table_free(&ctx->tag_union);
    cc_tag_table_free(&ctx->tag_enum);
    include_graph_destroy(&ctx->includeGraph);
    free(ctx->targetTriple);
    free(ctx->dataLayout);
    // builtins are static literals; nothing to free.
    free(ctx);
}

void cc_seed_builtins(CompilerContext* ctx) {
    // Nothing to do right now (we baked them in).
    // This exists so you can later extend with platform-specific typedefs.
    (void)ctx;
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
    free(ctx->targetTriple);
    ctx->targetTriple = triple ? strdup(triple) : NULL;
    return true;
}

const char* cc_get_target_triple(const CompilerContext* ctx) {
    return ctx ? ctx->targetTriple : NULL;
}

bool cc_set_data_layout(CompilerContext* ctx, const char* layout) {
    if (!ctx) return false;
    free(ctx->dataLayout);
    ctx->dataLayout = layout ? strdup(layout) : NULL;
    return true;
}

const char* cc_get_data_layout(const CompilerContext* ctx) {
    return ctx ? ctx->dataLayout : NULL;
}
