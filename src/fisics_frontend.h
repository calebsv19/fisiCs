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
    FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS = (1ULL << 6)
} FisicsContractCapability;

enum {
    FISICS_UNITS_DIM_SLOTS = 8
};

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
    const char* symbol_name;
    const char* dim_text;
    int8_t dim[FISICS_UNITS_DIM_SLOTS];
    bool resolved;
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

#ifdef __cplusplus
}
#endif
