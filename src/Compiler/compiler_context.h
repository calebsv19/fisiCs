#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Tag namespaces (C keeps these separate from typedef-names) ----
typedef enum {
    CC_TAG_STRUCT = 0,
    CC_TAG_UNION  = 1,
    CC_TAG_ENUM   = 2
} CCTagKind;

// ---- Simple StringSet (O(n) for now; fast enough, easy to swap later) ----
typedef struct {
    char   **items;
    size_t   count;
    size_t   capacity;
} CCStringSet;

// ---- Builtins container (opaque; use cc_is_builtin_type) ----
typedef struct {
    const char **names;
    size_t       count;
} CCBuiltins;

// ---- CompilerContext: shared across the whole translation unit ----
typedef struct CompilerContext {
    CCStringSet typedef_names;  // names introduced by 'typedef'
    CCStringSet tag_struct;     // struct tag names
    CCStringSet tag_union;      // union tag names
    CCStringSet tag_enum;       // enum  tag names
    CCBuiltins  builtins;       // builtin type names (void,int,uint64_t,...)
} CompilerContext;

// ---- Lifecycle ----
CompilerContext* cc_create(void);
void cc_destroy(CompilerContext* ctx);
void cc_seed_builtins(CompilerContext* ctx);

// ---- Typedef-name namespace ----
bool cc_is_typedef(const CompilerContext* ctx, const char* name);
bool cc_add_typedef(CompilerContext* ctx, const char* name);

// ---- Tag namespace (struct/union/enum) ----
bool cc_has_tag(const CompilerContext* ctx, CCTagKind kind, const char* name);
bool cc_add_tag (CompilerContext* ctx, CCTagKind kind, const char* name);

// ---- Builtins query ----
bool cc_is_builtin_type(const CompilerContext* ctx, const char* name);

#ifdef __cplusplus
}
#endif

