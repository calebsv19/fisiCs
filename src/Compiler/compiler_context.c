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
    return c;
}

void cc_destroy(CompilerContext* ctx) {
    if (!ctx) return;
    cc_stringset_free(&ctx->typedef_names);
    cc_stringset_free(&ctx->tag_struct);
    cc_stringset_free(&ctx->tag_union);
    cc_stringset_free(&ctx->tag_enum);
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

static CCStringSet* pick_tag_set(CompilerContext* ctx, CCTagKind k) {
    switch (k) {
        case CC_TAG_STRUCT: return &ctx->tag_struct;
        case CC_TAG_UNION:  return &ctx->tag_union;
        case CC_TAG_ENUM:   return &ctx->tag_enum;
        default: return NULL;
    }
}
static const CCStringSet* pick_tag_set_const(const CompilerContext* ctx, CCTagKind k) {
    switch (k) {
        case CC_TAG_STRUCT: return &ctx->tag_struct;
        case CC_TAG_UNION:  return &ctx->tag_union;
        case CC_TAG_ENUM:   return &ctx->tag_enum;
        default: return NULL;
    }
}

bool cc_has_tag(const CompilerContext* ctx, CCTagKind kind, const char* name) {
    const CCStringSet* s = pick_tag_set_const(ctx, kind);
    return s ? cc_stringset_contains(s, name) : false;
}
bool cc_add_tag(CompilerContext* ctx, CCTagKind kind, const char* name) {
    CCStringSet* s = pick_tag_set(ctx, kind);
    return s ? cc_stringset_add(s, name) : false;
}

bool cc_is_builtin_type(const CompilerContext* ctx, const char* name) {
    (void)ctx;
    return cc_builtins_has(&ctx->builtins, name);
}

