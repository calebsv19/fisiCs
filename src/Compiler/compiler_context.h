// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "Preprocessor/include_resolver.h"
#include "Compiler/diagnostics.h"
#include "Syntax/target_layout.h"

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
struct CCTagFieldLayout;

typedef struct {
    char* name;
    bool isDefined;
    uint64_t fingerprint;
    struct ASTNode* definition;
    bool hasLayout;
    size_t layoutSize;
    size_t layoutAlign;
    bool computingLayout;
    struct CCTagFieldLayout* fieldLayouts;
    size_t fieldLayoutCount;
    size_t fieldLayoutCapacity;
} CCTagRecord;

typedef struct {
    CCTagRecord* records;
    size_t count;
    size_t capacity;
} CCTagTable;

typedef struct {
    FisicsDiagnostic* items;
    size_t count;
    size_t capacity;
} CompilerDiagnostics;

typedef enum {
    FISICS_TOK_IDENTIFIER,
    FISICS_TOK_KEYWORD,
    FISICS_TOK_NUMBER,
    FISICS_TOK_STRING,
    FISICS_TOK_CHAR,
    FISICS_TOK_OPERATOR,
    FISICS_TOK_PUNCT,
    FISICS_TOK_COMMENT,
    FISICS_TOK_WHITESPACE
} FisicsTokenKind;

typedef struct {
    int line;
    int column;
    int length;
    FisicsTokenKind kind;
} FisicsTokenSpan;

typedef enum {
    FISICS_SYMBOL_UNKNOWN = 0,
    FISICS_SYMBOL_FUNCTION,
    FISICS_SYMBOL_STRUCT,
    FISICS_SYMBOL_UNION,
    FISICS_SYMBOL_ENUM,
    FISICS_SYMBOL_TYPEDEF,
    FISICS_SYMBOL_VARIABLE,
    FISICS_SYMBOL_FIELD,
    FISICS_SYMBOL_ENUM_MEMBER,
    FISICS_SYMBOL_MACRO
} FisicsSymbolKind;

typedef struct {
    const char* name;
    const char* file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
    FisicsSymbolKind kind;
    const char* parent_name;
    FisicsSymbolKind parent_kind;
    bool is_definition;
    bool is_variadic;
    const char* return_type;
    const char** param_types;
    const char** param_names;
    size_t param_count;
} FisicsSymbol;

typedef struct {
    FisicsTokenSpan* items;
    size_t count;
    size_t capacity;
} CompilerTokenSpans;

typedef struct {
    FisicsSymbol* items;
    size_t count;
    size_t capacity;
} CompilerSymbols;

typedef enum {
    FISICS_INCLUDE_LOCAL = 0,
    FISICS_INCLUDE_SYSTEM = 1
} FisicsIncludeKind;

typedef enum {
    FISICS_INCLUDE_PROJECT = 0,
    FISICS_INCLUDE_SYSTEM_ORIGIN = 1,
    FISICS_INCLUDE_EXTERNAL = 2,
    FISICS_INCLUDE_UNRESOLVED = 3
} FisicsIncludeOrigin;

typedef struct {
    const char* name;          // as written in source
    const char* resolved_path; // nullable if unresolved
    FisicsIncludeKind kind;    // angle vs quoted
    FisicsIncludeOrigin origin;
    bool resolved;
    int line;
    int column;
} FisicsInclude;

typedef struct {
    FisicsInclude* items;
    size_t count;
    size_t capacity;
} CompilerIncludes;

typedef struct CompilerContext {
    CCStringSet typedef_names;  // names introduced by 'typedef'
    CCTagTable tag_struct;      // struct tag metadata
    CCTagTable tag_union;       // union tag metadata
    CCTagTable tag_enum;        // enum tag metadata
    CCBuiltins  builtins;       // builtin type names (void,int,uint64_t,...)
    IncludeGraph includeGraph;  // dependency edges (parent -> child)
    uint64_t dl_canary_front;   // guard against overwrite
    char* targetTriple;
    TargetLayout* targetLayout;
    char* dataLayout;
    bool warnIgnoredInterop;
    bool errorIgnoredInterop;
    uint64_t dl_canary_back;    // guard against overwrite
    CompilerDiagnostics diags;  // Diagnostics recorded during lex/parse/sema.
    CompilerTokenSpans tokenSpans;
    CompilerSymbols symbols;
    CompilerIncludes includes;
    struct ASTNode* translationUnit;
    int languageDialect;        // CCDialect encoded as int to avoid circular include
    bool enableExtensions;
} CompilerContext;

typedef enum {
    CC_DIALECT_C99 = 0,
    CC_DIALECT_C11,
    CC_DIALECT_C17
} CCDialect;

typedef enum {
    CC_TAGDEF_ADDED = 0,
    CC_TAGDEF_MATCHING,
    CC_TAGDEF_CONFLICT
} CCTagDefineResult;

// ---- Lifecycle ----
CompilerContext* cc_create(void);
void cc_destroy(CompilerContext* ctx);
void cc_seed_builtins(CompilerContext* ctx);
void cc_set_language_dialect(CompilerContext* ctx, CCDialect dialect);
CCDialect cc_get_language_dialect(const CompilerContext* ctx);
void cc_set_extensions_enabled(CompilerContext* ctx, bool enabled);
bool cc_extensions_enabled(const CompilerContext* ctx);
long cc_dialect_stdc_version(CCDialect dialect);

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
typedef struct CCTagFieldLayout {
    char* name;               // nullable for unnamed bitfields
    size_t byteOffset;        // byte offset from struct base
    size_t bitOffset;         // bit offset within storageUnitBytes (0-based, endianness-aware)
    size_t widthBits;         // 0 for zero-width or non-bitfield
    size_t storageUnitBytes;  // size of the storage unit for the bitfield (or field size for non-bitfield)
    bool isBitfield;
    bool isZeroWidth;
    bool isSigned;
} CCTagFieldLayout;
bool cc_set_tag_field_layouts(CompilerContext* ctx,
                              CCTagKind kind,
                              const char* name,
                              const CCTagFieldLayout* layouts,
                              size_t count);
bool cc_get_tag_field_layouts(const CompilerContext* ctx,
                              CCTagKind kind,
                              const char* name,
                              const CCTagFieldLayout** layoutsOut,
                              size_t* countOut);

// ---- Builtins query ----
bool cc_is_builtin_type(const CompilerContext* ctx, const char* name);

// Include graph access
bool cc_set_include_graph(CompilerContext* ctx, const IncludeGraph* graph);
const IncludeGraph* cc_get_include_graph(const CompilerContext* ctx);

// Target recording (threaded to codegen/layout consumers)
bool cc_set_target_triple(CompilerContext* ctx, const char* triple);
const char* cc_get_target_triple(const CompilerContext* ctx);
bool cc_set_target_layout(CompilerContext* ctx, const TargetLayout* layout);
const TargetLayout* cc_get_target_layout(const CompilerContext* ctx);
bool cc_set_data_layout(CompilerContext* ctx, const char* layout);
const char* cc_get_data_layout(const CompilerContext* ctx);
void cc_set_interop_diag(CompilerContext* ctx, bool warnIgnored, bool errorIgnored);
bool cc_warn_ignored_interop(const CompilerContext* ctx);
bool cc_error_ignored_interop(const CompilerContext* ctx);

// Token span helpers
bool cc_set_token_spans(CompilerContext* ctx, const FisicsTokenSpan* spans, size_t count);
const FisicsTokenSpan* cc_get_token_spans(const CompilerContext* ctx, size_t* countOut);
void cc_clear_token_spans(CompilerContext* ctx);
bool cc_append_token_span(CompilerContext* ctx, const FisicsTokenSpan* span);

// Symbol helpers
bool cc_set_symbols(CompilerContext* ctx, const FisicsSymbol* symbols, size_t count);
const FisicsSymbol* cc_get_symbols(const CompilerContext* ctx, size_t* countOut);
void cc_clear_symbols(CompilerContext* ctx);

// Include helpers
bool cc_set_includes(CompilerContext* ctx, const FisicsInclude* includes, size_t count);
const FisicsInclude* cc_get_includes(const CompilerContext* ctx, size_t* countOut);
void cc_clear_includes(CompilerContext* ctx);
bool cc_append_include(CompilerContext* ctx, const FisicsInclude* include);

// Translation unit pointer access
void cc_set_translation_unit(CompilerContext* ctx, struct ASTNode* root);
struct ASTNode* cc_get_translation_unit(const CompilerContext* ctx);

#ifdef __cplusplus
}
#endif
