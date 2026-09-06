// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "Compiler/diagnostics.h"
#include "Compiler/compiler_context.h"

typedef enum {
    FISICS_ANALYSIS_MODE_STRICT = 0,
    FISICS_ANALYSIS_MODE_LENIENT = 1
} FisicsAnalysisMode;

typedef enum {
    FISICS_CONTRACT_CAP_DIAGNOSTICS = (1ULL << 0),
    FISICS_CONTRACT_CAP_INCLUDES = (1ULL << 1),
    FISICS_CONTRACT_CAP_SYMBOLS = (1ULL << 2),
    FISICS_CONTRACT_CAP_TOKENS = (1ULL << 3),
    FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID = (1ULL << 4),
    FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY = (1ULL << 5),
    FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS = (1ULL << 6),
    FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE = (1ULL << 7),
    FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA = (1ULL << 8)
} FisicsContractCapability;

enum {
    FISICS_UNITS_DIM_SLOTS = 8,
    FISICS_ANALYSIS_RESULT_EX_ABI_VERSION = 1,
    FISICS_SEMANTIC_METADATA_SCHEMA_VERSION = 1
};

#define FISICS_SEMANTIC_METADATA_SCHEMA_ID "fisiCs.semantic_metadata"

typedef struct {
    char contract_id[64];
    uint16_t contract_major;
    uint16_t contract_minor;
    uint16_t contract_patch;

    char producer_name[32];
    char producer_version[32];

    FisicsAnalysisMode mode;
    bool partial;
    bool fatal;

    uint64_t source_hash;
    uint64_t source_length;
    uint64_t capabilities;
} FisicsAnalysisContract;

typedef struct {
    uint64_t symbol_stable_id;
    bool has_symbol_stable_id;
    const char* symbol_name;
    const char* source_file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
    const char* dim_text;
    int8_t dim[FISICS_UNITS_DIM_SLOTS];
    bool resolved;
    const char* unit_source_text;
    const char* unit_name;
    const char* unit_symbol;
    const char* unit_family;
    bool unit_resolved;
} FisicsUnitsAttachment;

typedef struct {
    FisicsAnalysisContract contract;

    FisicsDiagnostic* diagnostics;
    size_t diag_count;

    FisicsTokenSpan* tokens;
    size_t token_count;

    FisicsSymbol* symbols;
    size_t symbol_count;

    FisicsInclude* includes;
    size_t include_count;

    /*
     * Optional extension-owned declaration/symbol units lane.
     * Phase 4 reserves the public shape now; producers only advertise the
     * capability when this lane is populated with valid attachment data.
     */
    FisicsUnitsAttachment* units_attachments;
    size_t units_attachment_count;
} FisicsAnalysisResult;

// Ownership: all arrays/strings in FisicsAnalysisResult are allocated by the
// frontend and must be released with fisics_free_analysis_result.

typedef enum {
    FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION = 1,
    FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE = 2
} FisicsFrontendSemanticMetadataSymbolKind;

typedef enum {
    FISICS_SEMANTIC_METADATA_EDGE_PRODUCES = 1,
    FISICS_SEMANTIC_METADATA_EDGE_CONSUMES = 2
} FisicsFrontendSemanticMetadataEdgeKind;

typedef enum {
    FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL = 1,
    FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED = 2
} FisicsFrontendSemanticMetadataReferenceState;

typedef struct {
    const char* source_identity;
    const char* authored_id;

    uint64_t symbol_stable_id;
    bool has_symbol_stable_id;
    const char* symbol_name;
    FisicsFrontendSemanticMetadataSymbolKind symbol_kind;

    const char* source_file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    const char* domain;
    const char* role;
} FisicsFrontendSemanticMetadataRecord;

typedef struct {
    size_t source_record_index;
    FisicsFrontendSemanticMetadataEdgeKind kind;
    const char* target_id;
    FisicsFrontendSemanticMetadataReferenceState resolution;

    size_t target_record_index;
    bool has_target_record_index;

    const char* source_file_path;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
} FisicsFrontendSemanticMetadataEdge;

typedef struct {
    uint32_t abi_version;
    size_t struct_size;

    FisicsAnalysisResult analysis;

    FisicsFrontendSemanticMetadataRecord* semantic_metadata_records;
    size_t semantic_metadata_record_count;

    FisicsFrontendSemanticMetadataEdge* semantic_metadata_edges;
    size_t semantic_metadata_edge_count;
} FisicsAnalysisResultEx;

typedef struct {
    // Array of include search paths (equivalent to -I). Optional; pass NULL/0 to use defaults.
    const char* const* include_paths;
    size_t include_path_count;

    // Array of predefined macros (equivalent to -DNAME or -DNAME=VALUE). Optional.
    const char* const* macro_defines;
    size_t macro_define_count;

    // Lenient mode toggle: 0 = default (lenient), >0 = force lenient, <0 = force strict.
    // Lenient is recommended for IDE usage to continue after missing headers/malformed macros.
    int lenient_mode;

    // Optional overlay feature flags (same bitmask family as CLI --overlay=).
    // Default is FISICS_OVERLAY_NONE.
    FisicsOverlayFeatures overlay_features;

    // When false, only symbols defined in the analyzed file are emitted.
    bool include_system_symbols;
} FisicsFrontendOptions;

bool fisics_analyze_buffer(const char* file_path,
                           const char* source,
                           size_t length,
                           const FisicsFrontendOptions* opts,
                           FisicsAnalysisResult* out);

void fisics_free_analysis_result(FisicsAnalysisResult* result);

bool fisics_analyze_buffer_ex(const char* file_path,
                              const char* source,
                              size_t length,
                              const FisicsFrontendOptions* opts,
                              FisicsAnalysisResultEx* out);

void fisics_free_analysis_result_ex(FisicsAnalysisResultEx* result);

#ifdef __cplusplus
}
#endif
