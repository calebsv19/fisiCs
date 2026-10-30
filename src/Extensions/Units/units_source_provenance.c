// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_source_provenance.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_hooks.h"
#include <stdlib.h>
#include <string.h>

static const char* retain(FisicsExtensionState* state,const char* path) {
    if(!path)return NULL;
    for(size_t i=0;i<state->unitsSourceFileCount;++i)
        if(!strcmp(state->unitsSourceFiles[i],path))return state->unitsSourceFiles[i];
    char* copy=malloc(strlen(path)+1);if(!copy)return NULL;strcpy(copy,path);
    char** files=realloc(state->unitsSourceFiles,(state->unitsSourceFileCount+1)*sizeof(*files));
    if(!files){free(copy);return NULL;}
    state->unitsSourceFiles=files;files[state->unitsSourceFileCount++]=copy;return copy;
}
static bool range(FisicsExtensionState* state,SourceRange* value) {
    const char* start=retain(state,value->start.file),*end=retain(state,value->end.file);
    if((value->start.file&&!start)||(value->end.file&&!end))return false;
    value->start.file=start;value->end.file=end;return true;
}
bool fisics_units_retain_source_ranges(CompilerContext* ctx,TokenBuffer* tokens) {
    if(!cc_overlay_physics_units_enabled(ctx))return true;
    if(!tokens || !fisics_extension_state_ensure(ctx))return false;
    for(size_t i=0;i<tokens->count;++i) {
        Token* token=&tokens->tokens[i];
        if(!range(ctx->extensionState,&token->location) ||
           !range(ctx->extensionState,&token->macroCallSite) ||
           !range(ctx->extensionState,&token->macroDefinition))return false;
    }
    return true;
}
