// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static CGScope* cg_scope_create(CGScope* parent);
static void cg_scope_destroy(CGScope* scope);

CodegenContext* codegen_context_create(const char* moduleName, const SemanticModel* semanticModel) {
    ProfilerScope scope = profiler_begin("codegen_context_create");
    CodegenContext* ctx = (CodegenContext*)calloc(1, sizeof(CodegenContext));
    if (!ctx) {
        profiler_end(scope);
        return NULL;
    }

    ctx->llvmContext = LLVMContextCreate();
    if (!ctx->llvmContext) {
        free(ctx);
        profiler_end(scope);
        return NULL;
    }

    const char* name = moduleName ? moduleName : "compiler_module";
    ctx->module = LLVMModuleCreateWithNameInContext(name, ctx->llvmContext);
    if (!ctx->module) {
        LLVMContextDispose(ctx->llvmContext);
        free(ctx);
        profiler_end(scope);
        return NULL;
    }

    ctx->builder = LLVMCreateBuilderInContext(ctx->llvmContext);
    if (!ctx->builder) {
        LLVMDisposeModule(ctx->module);
        LLVMContextDispose(ctx->llvmContext);
        free(ctx);
        profiler_end(scope);
        return NULL;
    }

    const char* verifyEnv = getenv("CODEGEN_VERIFY");
    ctx->verifyFunctions = (verifyEnv && verifyEnv[0] == '1');
    ctx->currentFunctionName = NULL;

    ctx->semanticModel = semanticModel;
    if (semanticModel) {
        ctx->typeCache = cg_type_cache_create(semanticModel);
        if (!ctx->typeCache) {
            LLVMDisposeBuilder(ctx->builder);
            LLVMDisposeModule(ctx->module);
            LLVMContextDispose(ctx->llvmContext);
            free(ctx);
            profiler_end(scope);
            return NULL;
        }

        CompilerContext* cctx = semanticModelGetContext(semanticModel);
        if (cctx) {
            const char* triple = cc_get_target_triple(cctx);
            if (triple && triple[0]) {
                LLVMSetTarget(ctx->module, triple);
            }
            const char* layout = cc_get_data_layout(cctx);
            char* generatedLayout = NULL;
            if (!layout || !layout[0]) {
                const TargetLayout* tl = cc_get_target_layout(cctx);
                generatedLayout = tl_to_llvm_datalayout(tl ? tl : tl_default());
                layout = generatedLayout;
            }
            if (layout && layout[0]) {
                bool printable = true;
                size_t len = 0;
                for (; layout[len]; ++len) {
                    unsigned char ch = (unsigned char)layout[len];
                    if (!isprint(ch)) { printable = false; break; }
                    if (len > 256) { printable = false; break; }
                }
                if (printable && len > 0) {
                    LLVMSetDataLayout(ctx->module, layout);
                } else {
                    LOG_WARN("codegen", "Ignoring invalid data layout string (len=%zu)", len);
                }
            }
            free(generatedLayout);
        }
    }

    profiler_end(scope);
    return ctx;
}

void codegen_context_destroy(CodegenContext* ctx) {
    if (!ctx) return;

    while (ctx->currentScope) {
        cg_scope_pop(ctx);
    }

    free(ctx->loopStack);
    ctx->loopStack = NULL;
    ctx->loopStackSize = 0;
    ctx->loopStackCapacity = 0;

    if (ctx->namedTypes) {
        for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
            free(ctx->namedTypes[i].name);
        }
        free(ctx->namedTypes);
        ctx->namedTypes = NULL;
        ctx->namedTypeCount = 0;
        ctx->namedTypeCapacity = 0;
    }

    if (ctx->structInfos) {
        for (size_t i = 0; i < ctx->structInfoCount; ++i) {
            free(ctx->structInfos[i].name);
            for (size_t j = 0; j < ctx->structInfos[i].fieldCount; ++j) {
                free(ctx->structInfos[i].fields[j].name);
            }
            free(ctx->structInfos[i].fields);
        }
        free(ctx->structInfos);
        ctx->structInfos = NULL;
        ctx->structInfoCount = 0;
        ctx->structInfoCapacity = 0;
    }

    if (ctx->labels) {
        for (size_t i = 0; i < ctx->labelCount; ++i) {
            free(ctx->labels[i].name);
        }
        free(ctx->labels);
        ctx->labels = NULL;
        ctx->labelCount = 0;
        ctx->labelCapacity = 0;
    }

    ctx->semanticModel = NULL;
    if (ctx->typeCache) {
        cg_type_cache_destroy(ctx->typeCache);
        ctx->typeCache = NULL;
    }

    if (ctx->builder) {
        LLVMDisposeBuilder(ctx->builder);
        ctx->builder = NULL;
    }

    if (ctx->module) {
        LLVMDisposeModule(ctx->module);
        ctx->module = NULL;
    }

    if (ctx->llvmContext) {
        /* NOTE:
         * We intentionally avoid LLVMContextDispose() here due a nondeterministic
         * teardown crash observed in optimized builds (SIGSEGV inside
         * llvm::LLVMContextImpl::~LLVMContextImpl during process exit). The
         * context lifetime is process-bounded for CLI invocations, so leaking the
         * context object is safer than hard-crashing during shutdown.
         */
        ctx->llvmContext = NULL;
    }

    free(ctx);
}

LLVMModuleRef codegen_get_module(const CodegenContext* ctx) {
    return ctx ? ctx->module : NULL;
}

static CGScope* cg_scope_create(CGScope* parent) {
    CGScope* scope = (CGScope*)calloc(1, sizeof(CGScope));
    if (!scope) return NULL;
    scope->parent = parent;
    return scope;
}

static void cg_scope_destroy(CGScope* scope) {
    if (!scope) return;
    for (size_t i = 0; i < scope->count; ++i) {
        free(scope->entries[i].name);
    }
    free(scope->entries);
    free(scope);
}

CGScope* cg_scope_push(CodegenContext* ctx) {
    if (!ctx) return NULL;
    CGScope* scope = cg_scope_create(ctx->currentScope);
    if (!scope) return NULL;
    ctx->currentScope = scope;
    return scope;
}

void cg_scope_pop(CodegenContext* ctx) {
    if (!ctx || !ctx->currentScope) return;
    CGScope* parent = ctx->currentScope->parent;
    cg_scope_destroy(ctx->currentScope);
    ctx->currentScope = parent;
}

void cg_scope_insert(CGScope* scope,
                     const char* name,
                     LLVMValueRef value,
                     LLVMTypeRef type,
                     bool isGlobal,
                     bool addressOnly,
                     LLVMTypeRef elementType,
                     const ParsedType* parsedType) {
    if (!scope || !name) return;

    for (size_t i = 0; i < scope->count; ++i) {
        if (scope->entries[i].name && strcmp(scope->entries[i].name, name) == 0) {
            scope->entries[i].value = value;
            scope->entries[i].type = type;
            scope->entries[i].isGlobal = isGlobal;
            scope->entries[i].addressOnly = addressOnly;
            scope->entries[i].elementType = elementType;
            scope->entries[i].vlaElementCount = NULL;
            scope->entries[i].parsedType = parsedType;
            return;
        }
    }

    if (scope->count == scope->capacity) {
        size_t newCap = scope->capacity ? scope->capacity * 2 : 4;
        NamedValue* resized = (NamedValue*)realloc(scope->entries, newCap * sizeof(NamedValue));
        if (!resized) return;
        scope->entries = resized;
        scope->capacity = newCap;
    }
    NamedValue* entry = &scope->entries[scope->count++];
    entry->name = strdup(name);
    entry->value = value;
    entry->type = type;
    entry->elementType = elementType;
    entry->vlaElementCount = NULL;
    entry->parsedType = parsedType;
    entry->isGlobal = isGlobal;
    entry->addressOnly = addressOnly;
}

NamedValue* cg_scope_lookup(CGScope* scope, const char* name) {
    for (CGScope* iter = scope; iter; iter = iter->parent) {
        for (size_t i = 0; i < iter->count; ++i) {
            if (iter->entries[i].name && strcmp(iter->entries[i].name, name) == 0) {
                return &iter->entries[i];
            }
        }
    }
    return NULL;
}

void cg_loop_push(CodegenContext* ctx, LLVMBasicBlockRef breakBB, LLVMBasicBlockRef continueBB) {
    if (!ctx) return;
    if (ctx->loopStackSize == ctx->loopStackCapacity) {
        size_t newCap = ctx->loopStackCapacity ? ctx->loopStackCapacity * 2 : 4;
        LoopTarget* resized = (LoopTarget*)realloc(ctx->loopStack, newCap * sizeof(LoopTarget));
        if (!resized) return;
        ctx->loopStack = resized;
        ctx->loopStackCapacity = newCap;
    }
    ctx->loopStack[ctx->loopStackSize].breakBB = breakBB;
    ctx->loopStack[ctx->loopStackSize].continueBB = continueBB;
    ctx->loopStackSize += 1;
}

void cg_loop_pop(CodegenContext* ctx) {
    if (!ctx || ctx->loopStackSize == 0) return;
    ctx->loopStackSize -= 1;
}

LoopTarget cg_loop_peek(const CodegenContext* ctx) {
    LoopTarget target = {0};
    if (!ctx || ctx->loopStackSize == 0) {
        return target;
    }
    return ctx->loopStack[ctx->loopStackSize - 1];
}

LoopTarget cg_loop_peek_for_continue(const CodegenContext* ctx) {
    LoopTarget target = {0};
    if (!ctx || ctx->loopStackSize == 0) {
        return target;
    }
    for (size_t i = ctx->loopStackSize; i > 0; --i) {
        LoopTarget candidate = ctx->loopStack[i - 1];
        if (candidate.continueBB) {
            return candidate;
        }
    }
    return target;
}

LabelBinding* cg_lookup_label(CodegenContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    for (size_t i = 0; i < ctx->labelCount; ++i) {
        if (ctx->labels[i].name && strcmp(ctx->labels[i].name, name) == 0) {
            return &ctx->labels[i];
        }
    }
    return NULL;
}

void cg_clear_labels(CodegenContext* ctx) {
    if (!ctx) return;
    if (ctx->labels) {
        for (size_t i = 0; i < ctx->labelCount; ++i) {
            free(ctx->labels[i].name);
            ctx->labels[i].name = NULL;
            ctx->labels[i].block = NULL;
            ctx->labels[i].defined = false;
        }
        free(ctx->labels);
        ctx->labels = NULL;
    }
    ctx->labelCount = 0;
    ctx->labelCapacity = 0;
}

LabelBinding* cg_ensure_label(CodegenContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    LabelBinding* existing = cg_lookup_label(ctx, name);
    if (existing) {
        return existing;
    }

    if (ctx->labelCount == ctx->labelCapacity) {
        size_t newCap = ctx->labelCapacity ? ctx->labelCapacity * 2 : 4;
        LabelBinding* resized = (LabelBinding*)realloc(ctx->labels, newCap * sizeof(LabelBinding));
        if (!resized) return NULL;
        ctx->labels = resized;
        ctx->labelCapacity = newCap;
    }

    LabelBinding* entry = &ctx->labels[ctx->labelCount++];
    entry->name = strdup(name);
    entry->block = NULL;
    entry->defined = false;
    return entry;
}

LLVMBasicBlockRef cg_label_ensure_block(CodegenContext* ctx,
                                        LabelBinding* binding,
                                        LLVMValueRef func) {
    if (!ctx || !binding || !func) {
        return NULL;
    }

    if (binding->block) {
        return binding->block;
    }

    binding->block = LLVMAppendBasicBlock(func, binding->name ? binding->name : "label");
    return binding->block;
}

LLVMContextRef cg_context_get_llvm_context(CodegenContext* ctx) {
    return ctx ? ctx->llvmContext : NULL;
}

LLVMModuleRef cg_context_get_module(CodegenContext* ctx) {
    return ctx ? ctx->module : NULL;
}

LLVMBuilderRef cg_context_get_builder(CodegenContext* ctx) {
    return ctx ? ctx->builder : NULL;
}

const SemanticModel* cg_context_get_semantic_model(CodegenContext* ctx) {
    return ctx ? ctx->semanticModel : NULL;
}

CGTypeCache* cg_context_get_type_cache(CodegenContext* ctx) {
    return ctx ? ctx->typeCache : NULL;
}

const TargetLayout* cg_context_get_target_layout(CodegenContext* ctx) {
    if (!ctx || !ctx->semanticModel) return NULL;
    CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
    if (!cctx) return NULL;
    return cc_get_target_layout(cctx);
}

LLVMTypeRef cg_context_lookup_named_type(CodegenContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
        if (ctx->namedTypes[i].name && strcmp(ctx->namedTypes[i].name, name) == 0) {
            return ctx->namedTypes[i].type;
        }
    }
    return NULL;
}

void cg_context_cache_named_type(CodegenContext* ctx, const char* name, LLVMTypeRef type) {
    if (!ctx || !name || !type) return;
    for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
        if (ctx->namedTypes[i].name && strcmp(ctx->namedTypes[i].name, name) == 0) {
            ctx->namedTypes[i].type = type;
            return;
        }
    }

    if (ctx->namedTypeCount == ctx->namedTypeCapacity) {
        size_t newCap = ctx->namedTypeCapacity ? ctx->namedTypeCapacity * 2 : 4;
        NamedType* resized = (NamedType*)realloc(ctx->namedTypes, newCap * sizeof(NamedType));
        if (!resized) return;
        ctx->namedTypes = resized;
        ctx->namedTypeCapacity = newCap;
    }

    ctx->namedTypes[ctx->namedTypeCount].name = strdup(name);
    ctx->namedTypes[ctx->namedTypeCount].type = type;
    ctx->namedTypeCount += 1;
}
