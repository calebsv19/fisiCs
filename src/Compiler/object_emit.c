// SPDX-License-Identifier: Apache-2.0

#include "Compiler/object_emit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Error.h>

static bool set_error(char** out, const char* msg, const char* detail) {
    if (!out) return false;
    size_t len = strlen(msg);
    if (detail && detail[0]) {
        len += 2 + strlen(detail); // ": " + detail
    }
    char* buf = (char*)malloc(len + 1);
    if (!buf) return false;
    if (detail && detail[0]) {
        snprintf(buf, len + 1, "%s: %s", msg, detail);
    } else {
        snprintf(buf, len + 1, "%s", msg);
    }
    *out = buf;
    return true;
}

static void debug_print_selected_libc_signatures(LLVMModuleRef module) {
    if (!module) return;
    const char* traceEnv = getenv("FISICS_VERIFY_IR_TRACE_LIBC");
    if (!traceEnv || !traceEnv[0] || strcmp(traceEnv, "0") == 0) {
        return;
    }

    const char* names[] = {
        "memset",
        "memcpy",
        "memmove",
        "snprintf",
        "vsnprintf",
        "sprintf",
        "__builtin___memset_chk",
        "__builtin___snprintf_chk",
        "__builtin___vsnprintf_chk"
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        LLVMValueRef fn = LLVMGetNamedFunction(module, names[i]);
        if (!fn) continue;

        LLVMTypeRef valueTy = LLVMTypeOf(fn);
        LLVMTypeRef globalTy = LLVMGlobalGetValueType(fn);
        char* valueTyStr = valueTy ? LLVMPrintTypeToString(valueTy) : NULL;
        char* globalTyStr = globalTy ? LLVMPrintTypeToString(globalTy) : NULL;
        fprintf(stderr,
                "[verify-trace] fn=%s value-type=%s global-type=%s\n",
                names[i],
                valueTyStr ? valueTyStr : "<null>",
                globalTyStr ? globalTyStr : "<null>");
        if (valueTyStr) LLVMDisposeMessage(valueTyStr);
        if (globalTyStr) LLVMDisposeMessage(globalTyStr);
    }
}

static void debug_dump_module_on_verify_failure(LLVMModuleRef module) {
    if (!module) return;
    const char* dumpPath = getenv("FISICS_VERIFY_IR_DUMP_PATH");
    if (!dumpPath || !dumpPath[0]) return;
    char* printErr = NULL;
    if (LLVMPrintModuleToFile(module, dumpPath, &printErr) != 0) {
        fprintf(stderr,
                "[verify-trace] failed to write module dump to %s: %s\n",
                dumpPath,
                printErr ? printErr : "unknown");
    } else {
        fprintf(stderr, "[verify-trace] wrote module dump to %s\n", dumpPath);
    }
    if (printErr) {
        LLVMDisposeMessage(printErr);
    }
}

bool compiler_init_llvm_native(void) {
    static bool initialized = false;
    static bool attempted = false;
    if (initialized) return true;
    if (attempted) return false;
    attempted = true;
    if (LLVMInitializeNativeTarget() != 0) {
        fprintf(stderr, "Error: LLVMInitializeNativeTarget failed\n");
        return false;
    }
    if (LLVMInitializeNativeAsmPrinter() != 0) {
        fprintf(stderr, "Error: LLVMInitializeNativeAsmPrinter failed\n");
        return false;
    }
    initialized = true;
    return true;
}

bool compiler_emit_object_file(LLVMModuleRef module,
                               const char* targetTripleOpt,
                               const char* dataLayoutOpt,
                               const char* outputPath,
                               char** errorOut) {
    const char* debugProgressEnv = getenv("FISICS_DEBUG_PROGRESS");
    bool debugProgress = debugProgressEnv && debugProgressEnv[0] && strcmp(debugProgressEnv, "0") != 0;
    if (errorOut) *errorOut = NULL;
    if (!module || !outputPath) {
        set_error(errorOut, "Invalid arguments to compiler_emit_object_file", NULL);
        return false;
    }
    if (!compiler_init_llvm_native()) {
        set_error(errorOut, "Failed to initialize LLVM native target", NULL);
        return false;
    }
    if (debugProgress) {
        fprintf(stderr, "[emit] begin output=%s\n", outputPath);
    }

    char* triple = NULL;
    bool tripleNeedsDispose = false;
    bool tripleNeedsFree = false;
    if (targetTripleOpt && targetTripleOpt[0]) {
        triple = strdup(targetTripleOpt);
        tripleNeedsFree = true;
    } else {
        triple = LLVMGetDefaultTargetTriple();
        tripleNeedsDispose = true;
    }
    if (!triple) {
        set_error(errorOut, "Failed to determine target triple", NULL);
        return false;
    }

    LLVMSetTarget(module, triple);

    LLVMTargetRef target = NULL;
    char* targetErr = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &targetErr) != 0) {
        set_error(errorOut, "LLVMGetTargetFromTriple failed", targetErr);
        if (targetErr) LLVMDisposeMessage(targetErr);
        if (tripleNeedsDispose) LLVMDisposeMessage(triple);
        if (tripleNeedsFree) free(triple);
        return false;
    }
    if (targetErr) LLVMDisposeMessage(targetErr);

    /*
     * Default to O0 for stability across the final/runtime conformance lane.
     * LLVM 20's LowerExpectIntrinsic pass can crash on some legal IR patterns
     * we emit in stress/control-flow tests. Keep opt-level override support so
     * callers can opt into O1/O2/O3 when they explicitly need it.
     */
    LLVMCodeGenOptLevel optLevel = LLVMCodeGenLevelDefault;
    LLVMRelocMode reloc = LLVMRelocDefault;
    LLVMCodeModel codeModel = LLVMCodeModelDefault;
    const char* cpu = "";
    const char* features = "";
    int requestedOptLevel = 0;
    const char* optEnv = getenv("FISICS_LLVM_OPT_LEVEL");
    if (optEnv && optEnv[0]) {
        int level = atoi(optEnv);
        switch (level) {
            case 0:
            case 1:
            case 2:
            case 3:
                requestedOptLevel = level;
                break;
            default: break;
        }
    }
    switch (requestedOptLevel) {
        case 0: optLevel = LLVMCodeGenLevelNone; break;
        case 1: optLevel = LLVMCodeGenLevelLess; break;
        case 2: optLevel = LLVMCodeGenLevelDefault; break;
        case 3: optLevel = LLVMCodeGenLevelAggressive; break;
        default: break;
    }

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(target,
                                                      triple,
                                                      cpu,
                                                      features,
                                                      optLevel,
                                                      reloc,
                                                      codeModel);
    if (!tm) {
        set_error(errorOut, "LLVMCreateTargetMachine failed", NULL);
        if (tripleNeedsDispose) LLVMDisposeMessage(triple);
        if (tripleNeedsFree) free(triple);
        return false;
    }

    const char* globalIselEnv = getenv("FISICS_LLVM_GLOBAL_ISEL");
    bool enableGlobalIsel = globalIselEnv && globalIselEnv[0] && strcmp(globalIselEnv, "0") != 0;
    if (enableGlobalIsel) {
        LLVMSetTargetMachineGlobalISel(tm, 1);
        LLVMSetTargetMachineGlobalISelAbort(tm, LLVMGlobalISelAbortDisableWithDiag);
        if (debugProgress) {
            fprintf(stderr, "[emit] target machine global-isel enabled\n");
        }
    }

    const char* fastIselEnv = getenv("FISICS_LLVM_FAST_ISEL");
    bool enableFastIsel = fastIselEnv && fastIselEnv[0] && strcmp(fastIselEnv, "0") != 0;
    if (enableFastIsel) {
        LLVMSetTargetMachineFastISel(tm, 1);
        if (debugProgress) {
            fprintf(stderr, "[emit] target machine fast-isel enabled\n");
        }
    }

    if (dataLayoutOpt && dataLayoutOpt[0]) {
        LLVMSetDataLayout(module, dataLayoutOpt);
    } else {
        LLVMTargetDataRef layout = LLVMCreateTargetDataLayout(tm);
        char* layoutStr = LLVMCopyStringRepOfTargetData(layout);
        if (layoutStr) {
            LLVMSetDataLayout(module, layoutStr);
            LLVMDisposeMessage(layoutStr);
        }
        LLVMDisposeTargetData(layout);
    }

    const char* verifyEnv = getenv("FISICS_VERIFY_IR");
    bool verifyIR = verifyEnv && verifyEnv[0] && strcmp(verifyEnv, "0") != 0;
    if (verifyIR) {
        char* verifyErr = NULL;
        if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verifyErr) != 0) {
            debug_print_selected_libc_signatures(module);
            debug_dump_module_on_verify_failure(module);
            set_error(errorOut, "LLVMVerifyModule failed", verifyErr);
            if (verifyErr) {
                LLVMDisposeMessage(verifyErr);
            }
            LLVMDisposeTargetMachine(tm);
            if (tripleNeedsDispose) LLVMDisposeMessage(triple);
            if (tripleNeedsFree) free(triple);
            return false;
        }
        if (verifyErr) {
            LLVMDisposeMessage(verifyErr);
        }
    }

    /*
     * Run optimization pipeline before object emission.
     * - O0: keep behavior close to debug builds.
     * - O1/O2/O3: run standard LLVM pass pipelines for runtime viability.
     */
    {
        LLVMPassBuilderOptionsRef pbOptions = LLVMCreatePassBuilderOptions();
        if (pbOptions) {
            const char* skipGlobalDceEnv = getenv("FISICS_SKIP_GLOBALDCE");
            bool skipGlobalDce = skipGlobalDceEnv && skipGlobalDceEnv[0] && strcmp(skipGlobalDceEnv, "0") != 0;
            const char* pipeline = NULL;
            if (requestedOptLevel == 1) {
                pipeline = "default<O1>";
            } else if (requestedOptLevel == 2) {
                pipeline = "default<O2>";
            } else if (requestedOptLevel >= 3) {
                pipeline = "default<O3>";
            }
            if (pipeline) {
                if (debugProgress) {
                    fprintf(stderr, "[emit] run pass pipeline=%s\n", pipeline);
                }
                LLVMErrorRef passErr = LLVMRunPasses(module, pipeline, tm, pbOptions);
                if (passErr) {
                    char* passErrMsg = LLVMGetErrorMessage(passErr);
                    if (passErrMsg) {
                        fprintf(stderr, "Warning: pre-emit %s pipeline failed: %s\n", pipeline, passErrMsg);
                        LLVMDisposeErrorMessage(passErrMsg);
                    }
                }
                if (debugProgress) {
                    fprintf(stderr, "[emit] pass pipeline complete=%s\n", pipeline);
                }
            }

            /*
             * Drop unreferenced internal globals/functions before object emission.
             * This aligns our object surface with clang for TU-level header-heavy builds
             * (for example static inline helpers from SDL headers) and prevents link
             * failures from dead, never-called helper bodies.
             */
            if (!skipGlobalDce) {
                if (debugProgress) {
                    fprintf(stderr, "[emit] run pass pipeline=globaldce\n");
                }
                LLVMErrorRef dceErr = LLVMRunPasses(module, "globaldce", tm, pbOptions);
                if (dceErr) {
                    char* dceErrMsg = LLVMGetErrorMessage(dceErr);
                    if (dceErrMsg) {
                        fprintf(stderr, "Warning: pre-emit globaldce pass failed: %s\n", dceErrMsg);
                        LLVMDisposeErrorMessage(dceErrMsg);
                    }
                }
                if (debugProgress) {
                    fprintf(stderr, "[emit] pass pipeline complete=globaldce\n");
                }
            } else if (debugProgress) {
                fprintf(stderr, "[emit] skip pass pipeline=globaldce\n");
            }

            LLVMDisposePassBuilderOptions(pbOptions);
        }
    }

    char* emitErr = NULL;
    if (debugProgress) {
        fprintf(stderr, "[emit] target machine emit start output=%s\n", outputPath);
    }
    bool ok = (LLVMTargetMachineEmitToFile(tm,
                                           module,
                                           (char*)outputPath,
                                           LLVMObjectFile,
                                           &emitErr) == 0);
    if (debugProgress) {
        fprintf(stderr, "[emit] target machine emit done ok=%d output=%s\n", ok ? 1 : 0, outputPath);
    }
    if (!ok) {
        set_error(errorOut, "LLVMTargetMachineEmitToFile failed", emitErr);
    }
    if (emitErr) {
        LLVMDisposeMessage(emitErr);
    }

    LLVMDisposeTargetMachine(tm);
    if (tripleNeedsDispose) LLVMDisposeMessage(triple);
    if (tripleNeedsFree) free(triple);

    return ok;
}
