#pragma once

#include <stdbool.h>

#include <llvm-c/Core.h>

// Initialize native target + asm printer once per process.
bool compiler_init_llvm_native(void);

// Emit an object file for the given module. Will set the module's target triple
// and data layout based on the provided strings (or host defaults if omitted).
// On failure, returns false and, if errorOut is non-NULL, allocates an error
// message that the caller must free().
bool compiler_emit_object_file(LLVMModuleRef module,
                               const char* targetTripleOpt,
                               const char* dataLayoutOpt,
                               const char* outputPath,
                               char** errorOut);
