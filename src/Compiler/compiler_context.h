#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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
typedef struct {
    char* name;
    bool isDefined;
    uint64_t fingerprint;
} CCTagRecord;

typedef struct {
    CCTagRecord* records;
    size_t count;
    size_t capacity;
} CCTagTable;

typedef struct CompilerContext {
    CCStringSet typedef_names;  // names introduced by 'typedef'
    CCTagTable tag_struct;      // struct tag metadata
    CCTagTable tag_union;       // union tag metadata
    CCTagTable tag_enum;        // enum tag metadata
    CCBuiltins  builtins;       // builtin type names (void,int,uint64_t,...)
} CompilerContext;

typedef enum {
    CC_TAGDEF_ADDED = 0,
    CC_TAGDEF_MATCHING,
    CC_TAGDEF_CONFLICT
} CCTagDefineResult;

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
CCTagDefineResult cc_define_tag(CompilerContext* ctx, CCTagKind kind, const char* name, uint64_t fingerprint);
bool cc_tag_is_defined(const CompilerContext* ctx, CCTagKind kind, const char* name);

// ---- Builtins query ----
bool cc_is_builtin_type(const CompilerContext* ctx, const char* name);

#ifdef __cplusplus
}
#endif
