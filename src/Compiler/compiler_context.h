#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "Preprocessor/include_resolver.h"

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
struct ASTNode;

typedef struct {
    char* name;
    bool isDefined;
    uint64_t fingerprint;
    struct ASTNode* definition;
    bool hasLayout;
    size_t layoutSize;
    size_t layoutAlign;
    bool computingLayout;
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
    IncludeGraph includeGraph;  // dependency edges (parent -> child)
    char* targetTriple;
    char* dataLayout;
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
CCTagDefineResult cc_define_tag(CompilerContext* ctx,
                                CCTagKind kind,
                                const char* name,
                                uint64_t fingerprint,
                                struct ASTNode* definition);
bool cc_tag_is_defined(const CompilerContext* ctx, CCTagKind kind, const char* name);
struct ASTNode* cc_tag_definition(const CompilerContext* ctx, CCTagKind kind, const char* name);
bool cc_set_tag_layout(CompilerContext* ctx, CCTagKind kind, const char* name, size_t size, size_t align);
bool cc_get_tag_layout(const CompilerContext* ctx, CCTagKind kind, const char* name, size_t* sizeOut, size_t* alignOut);
bool cc_tag_mark_computing(CompilerContext* ctx, CCTagKind kind, const char* name, bool computing);
bool cc_tag_is_computing(const CompilerContext* ctx, CCTagKind kind, const char* name);

// ---- Builtins query ----
bool cc_is_builtin_type(const CompilerContext* ctx, const char* name);

// Include graph access
bool cc_set_include_graph(CompilerContext* ctx, const IncludeGraph* graph);
const IncludeGraph* cc_get_include_graph(const CompilerContext* ctx);

// Target recording (threaded to codegen/layout consumers)
bool cc_set_target_triple(CompilerContext* ctx, const char* triple);
const char* cc_get_target_triple(const CompilerContext* ctx);
bool cc_set_data_layout(CompilerContext* ctx, const char* layout);
const char* cc_get_data_layout(const CompilerContext* ctx);

#ifdef __cplusplus
}
#endif
