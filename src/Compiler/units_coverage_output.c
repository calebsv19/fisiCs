// SPDX-License-Identifier: Apache-2.0
#include "Compiler/units_coverage_output.h"
#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static bool json_string(FILE* out, const char* value) {
    if (fputc('"', out) == EOF) return false;
    const unsigned char* cursor = (const unsigned char*)(value ? value : "");
    for (; *cursor; ++cursor) {
        switch (*cursor) {
            case '"': if (fputs("\\\"", out) == EOF) return false; break;
            case '\\': if (fputs("\\\\", out) == EOF) return false; break;
            case '\b': if (fputs("\\b", out) == EOF) return false; break;
            case '\f': if (fputs("\\f", out) == EOF) return false; break;
            case '\n': if (fputs("\\n", out) == EOF) return false; break;
            case '\r': if (fputs("\\r", out) == EOF) return false; break;
            case '\t': if (fputs("\\t", out) == EOF) return false; break;
            default:
                if (*cursor < 0x20u) {
                    if (fprintf(out, "\\u%04x", (unsigned)*cursor) < 0) return false;
                } else if (fputc(*cursor, out) == EOF) return false;
        }
    }
    return fputc('"', out) != EOF;
}

static char* canonical_destination(const char* path) {
    char* existing = realpath(path, NULL);
    if (existing) return existing;
    const char* slash = strrchr(path, '/');
    const char* leaf = path;
    char* directory = NULL;
    if (slash) {
        size_t count = (size_t)(slash - path);
        directory = count ? (char*)malloc(count + 1u) : strdup("/");
        if (directory && count) { memcpy(directory, path, count); directory[count] = '\0'; }
        leaf = slash + 1;
    } else directory = strdup(".");
    if (!directory || !leaf[0]) { free(directory); return NULL; }
    char* base = realpath(directory, NULL);
    free(directory);
    if (!base) return NULL;
    size_t count = strlen(base) + strlen(leaf) + 2u;
    char* result = (char*)malloc(count);
    if (result) snprintf(result, count, "%s%s%s", base,
                         strcmp(base, "/") == 0 ? "" : "/", leaf);
    free(base);
    return result;
}


static const char* state(FisicsCoverageState s) {
    static const char* names[]={"checked","trusted_boundary","unchecked","contradictory"};
    return s<=FISICS_COVERAGE_CONTRADICTORY?names[s]:"invalid";
}
static const char* boundary(FisicsCoverageBoundary b) {
    static const char* names[]={"initializer","assignment","argument","return","operation"};
    return b<=FISICS_COVERAGE_OPERATION?names[b]:"invalid";
}
static const char* reason(FisicsCoverageReason r) {
    static const char* names[]={"none","overlay_disabled","missing_annotation","unannotated_storage",
        "uncontracted_call","unresolved_annotation","no_inference","untracked_provenance",
        "declared_interface","assumed_representation","dimension_mismatch","unit_mismatch","missing_unit"};
    return r<=FISICS_COVERAGE_MISSING_UNIT?names[r]:"invalid";
}
static void location(FILE* out,const ASTNode* node) {
    if(!node){fputs("null",out);return;}
    fputs("{\"file\":",out);json_string(out,node->location.start.file);
    fprintf(out,",\"line\":%d,\"column\":%d,\"end_line\":%d,\"end_column\":%d}",
        node->location.start.line,node->location.start.column,node->location.end.line,node->location.end.column);
}
static size_t expression_id(const FisicsCoverageReport* r,const ASTNode* n) {
    for(size_t i=0;i<r->expressionCount;++i)if(r->expressions[i].node==n)return i;
    return SIZE_MAX;
}
static void fact(FILE* out,const FisicsCoverageFact* f) {
    fprintf(out,"{\"origin\":\"%s\",\"reason\":\"%s\",\"dimension\":",
        f->origin==FISICS_COVERAGE_DECLARED?"declared":f->origin==FISICS_COVERAGE_INFERRED?"inferred":"unknown",reason(f->reason));
    if(!f->hasDimension)fputs("null",out);
    else {fputc('[',out);for(int i=0;i<8;++i)fprintf(out,"%s%d",i?",":"",f->dimension.e[i]);fputc(']',out);}
    fputs(",\"unit\":",out);if(f->hasUnit&&f->unit)json_string(out,f->unit->name);else fputs("null",out);
    fprintf(out,",\"provenance_complete\":%s,\"trusted\":%s,\"site\":",f->provenanceComplete?"true":"false",f->trusted?"true":"false");
    location(out,f->site);fputs(",\"origin_location\":",out);location(out,f->originNode);fputc('}',out);
}
static void text_fact(FILE* out,const char* name,const FisicsCoverageFact* f) {
    char* dimension=f->hasDimension?fisics_dim_to_string(f->dimension):NULL;
    fprintf(out,"    %s: %s; unit=%s; origin=",name,dimension?dimension:"unknown",
        f->hasUnit&&f->unit?f->unit->name:"unspecified");
    free(dimension);
    if(f->originNode)fprintf(out,"%s:%d:%d",f->originLocation.start.file?f->originLocation.start.file:"?",
        f->originLocation.start.line,f->originLocation.start.column);
    else fputs("unavailable",out);
    fprintf(out,"; provenance=%s\n",f->provenanceComplete?"complete":"incomplete");
}
static const char* dependency_role(const ASTNode* node,size_t i) {
    if(node->type==AST_ARRAY_ACCESS)return i?"index":"storage_base";
    if(node->type==AST_DOT_ACCESS || node->type==AST_POINTER_ACCESS || node->type==AST_POINTER_DEREFERENCE)return "storage_base";
    if(node->type==AST_TERNARY_EXPRESSION && !i)return "condition";
    if(node->type==AST_FUNCTION_CALL)return "argument";
    return "value";
}
static bool document(FILE* out,const char* source,const CompileResult* result,
    const FisicsCoverageReport* r,const CompileOptions* options,const FisicsCoverageDecision* decision,bool json) {
    const FisicsCoverageSummary* s=&r->summary;
    if(!json) {
        fprintf(out,"fisiCs units coverage v1: %s :: %s\nEnumeration complete: %s; compilation errors: %zu\n",
            source,r->functionName,r->enumerationComplete?"yes":"no",result->semanticErrors);
        fprintf(out,"Selected obligations: %zu; checked %zu; trusted %zu; unchecked %zu; contradictory %zu\n",
            s->total,s->checked,s->trusted,s->unchecked,s->contradictory);
        for(size_t i=0;i<r->rowCount;++i) {
            const FisicsCoverageAssessment* a=&r->rows[i].assessment;const ASTNode* n=a->obligation.site;
            fprintf(out,"r%zu %d:%d %-18s %-16s %s",i,n->location.start.line,n->location.start.column,
                state(a->state),r->rows[i].operation,reason(a->reason));
            if(a->lossNode)fprintf(out,"; loss %s:%d:%d",a->lossNode->location.start.file?a->lossNode->location.start.file:"?",a->lossNode->location.start.line,a->lossNode->location.start.column);
            if(decision->policy.enabled)fprintf(out,"; strict=%s",r->rows[i].scalarControl?(r->rows[i].controlReason?r->rows[i].controlReason:"ordinary_loop_control"):r->rows[i].cInterface?"declared_c_interface":"required");
            fputc('\n',out);
            text_fact(out,"expected",&a->obligation.expected);text_fact(out,"actual",&a->obligation.actual);
        }
        if(decision->policy.enabled)fprintf(out,"Strict policy selected_function_v1: %s; required %zu; checked %zu; ordinary controls %zu; permitted trust %zu; blocked %zu\n",
            decision->accepted && !result->semanticErrors?"accepted":"rejected",decision->required,decision->checked,decision->controls,decision->acceptedTrust,decision->blocked);
        else fputs("Counts describe the selected AST obligations; trust and unknown are not checked coverage. No strict enforcement.\n",out);
        return !ferror(out);
    }
    fputs("{\n  \"schema\":\"fisiCs.units_coverage\",\"schema_version\":1,\n  \"source\":",out);json_string(out,source);
    fprintf(out,",\"source_hash_fnv1a64\":\"%016" PRIx64 "\",\"source_length_bytes\":%zu,\n",result->analyzedSourceHash,result->analyzedSourceLength);
    fprintf(out,"  \"analysis_configuration\":{\"dialect\":%d,\"compat_features\":%" PRIu64 ",\"macro_defines\":[",(int)options->dialect,(uint64_t)options->compatFeatures);
    for(size_t i=0;i<options->macroDefineCount;++i){if(i)fputc(',',out);json_string(out,options->macroDefines[i]);}
    fputs("],\"include_paths\":[",out);
    for(size_t i=0;i<options->includePathCount;++i){if(i)fputc(',',out);json_string(out,options->includePaths[i]);}
    fputs("],\"forced_includes\":[",out);
    for(size_t i=0;i<options->forcedIncludeCount;++i){if(i)fputc(',',out);json_string(out,options->forcedIncludes[i]);}
    fputs("]},\n",out);
    fputs("  \"scope\":{\"kind\":\"function\",\"name\":",out);json_string(out,r->functionName);
    fputs(",\"definition_location\":",out);location(out,r->function->functionDef.funcName);
    fprintf(out,",\"enumeration_complete\":%s,\"unsupported_nodes\":%zu},\n",r->enumerationComplete?"true":"false",r->unsupportedNodes);
    fprintf(out,"  \"compilation_errors\":%zu,\"strict_enforcement\":%s,\n",result->semanticErrors,decision->policy.enabled?"true":"false");
    if(decision->policy.enabled)fprintf(out,"  \"strict_policy\":{\"contract\":\"selected_function_v1\",\"allow_declared_boundaries\":%s,\"allow_literal_assumptions\":%s,\"accepted\":%s,\"required\":%zu,\"ordinary_controls\":%zu,\"checked\":%zu,\"permitted_trust\":%zu,\"blocked\":%zu},\n",
        decision->policy.allowDeclaredBoundaries?"true":"false",decision->policy.allowLiteralAssumptions?"true":"false",
        decision->accepted && !result->semanticErrors?"true":"false",decision->required,decision->controls,decision->checked,decision->acceptedTrust,decision->blocked);
    fprintf(out,"  \"denominator\":{\"contract\":\"selected_ast_obligations_v1\",\"total\":%zu,\"checked\":%zu,\"trusted\":%zu,\"unchecked\":%zu,\"contradictory\":%zu},\n  \"expressions\":[\n",
        s->total,s->checked,s->trusted,s->unchecked,s->contradictory);
    for(size_t i=0;i<r->expressionCount;++i) {
        const FisicsCoverageExpression* e=&r->expressions[i];fprintf(out,"    {\"id\":\"e%zu\",\"fact\":",i);fact(out,&e->fact);
        fputs(",\"loss_location\":",out);location(out,e->loss);fputs(",\"dependencies\":[",out);
        bool first=true;for(size_t j=0;j<e->dependencyCount;++j){size_t k=expression_id(r,e->dependencies[j]);if(k==SIZE_MAX)continue;
            fprintf(out,"%s{\"expression\":\"e%zu\",\"role\":\"%s\"}",first?"":",",k,dependency_role(e->node,j));first=false;}
        fprintf(out,"]}%s\n",i+1<r->expressionCount?",":"");
    }
    fputs("  ],\n  \"obligations\":[\n",out);
    for(size_t i=0;i<r->rowCount;++i) {
        const FisicsCoverageRow* row=&r->rows[i];const FisicsCoverageAssessment* a=&row->assessment;
        fprintf(out,"    {\"id\":\"r%zu\",\"boundary\":\"%s\",\"slot\":",i,boundary(a->obligation.boundary));
        if(a->obligation.slot==SIZE_MAX)fputs("null",out);else fprintf(out,"%zu",a->obligation.slot);
        fputs(",\"operation\":",out);json_string(out,row->operation);
        if(decision->policy.enabled)fprintf(out,",\"strict_required\":%s",row->scalarControl?"false":"true");
        if(decision->policy.enabled && (row->controlReason || row->cInterface)) {
            fputs(",\"strict_basis\":",out);json_string(out,row->cInterface?"declared_c_interface":row->controlReason);
        }
        if(decision->policy.enabled && row->cInterface && a->obligation.site &&
            a->obligation.site->type==AST_FUNCTION_CALL && a->obligation.site->functionCall.callee &&
            a->obligation.site->functionCall.callee->type==AST_IDENTIFIER) {
            fputs(",\"interface_callee\":",out);json_string(out,a->obligation.site->functionCall.callee->valueNode.value);
        }
        fprintf(out,",\"state\":\"%s\",\"reason\":\"%s\",\"dimension_compared\":%s,\"unit_compared\":%s,\"site\":",
            state(a->state),reason(a->reason),a->dimensionCompared?"true":"false",a->unitCompared?"true":"false");location(out,a->obligation.site);
        fputs(",\"loss_location\":",out);location(out,a->lossNode);fputs(",\"expected\":",out);fact(out,&a->obligation.expected);
        fputs(",\"actual\":",out);fact(out,&a->obligation.actual);
        fputs(",\"result\":",out);if(row->expression<r->expressionCount)fprintf(out,"\"e%zu\"",row->expression);else fputs("null",out);
        fprintf(out,"}%s\n",i+1<r->rowCount?",":"");
    }
    fputs("  ]\n}\n",out);return !ferror(out);
}
static bool aliases(const char* a,const char* b) {
    if(!a||!b)return false;
    char* ca=canonical_destination(a);char* cb=canonical_destination(b);
    bool same=ca&&cb&&!strcmp(ca,cb);free(ca);free(cb);
    struct stat sa,sb;if(!same && !stat(a,&sa) && !stat(b,&sb))same=sa.st_dev==sb.st_dev && sa.st_ino==sb.st_ino;
    return same;
}
bool fisics_units_coverage_destination(const char* path,const char* other,const char* source,
    const CompileResult* result) {
    if(!path || !path[0] || !strcmp(path,"-") || aliases(path,source) || aliases(path,other))return false;
    char* canonical=canonical_destination(path);
    if(!canonical)return false;
    free(canonical);
    const IncludeGraph* g=cc_get_include_graph(result->compilerCtx);
    if(g)for(size_t i=0;i<g->count;++i)
        if(aliases(path,g->edges[i].from)||aliases(path,g->edges[i].to))return false;
    return true;
}
bool fisics_units_coverage_write(const char* path,const char* other,const char* source,
    const CompileResult* result,const FisicsCoverageReport* report,const CompileOptions* options,const FisicsCoverageDecision* decision,bool json) {
    if(!fisics_units_coverage_destination(path,other,source,result))return false;
    size_t len=strlen(path);char* temp=malloc(len+sizeof(".tmp.XXXXXX"));if(!temp)return false;
    snprintf(temp,len+sizeof(".tmp.XXXXXX"),"%s.tmp.XXXXXX",path);int fd=mkstemp(temp);
    if(fd<0){free(temp);return false;}FILE* out=fdopen(fd,"w");
    bool ok=out&&document(out,source,result,report,options,decision,json);
    if(getenv("FISICS_TEST_FAIL_UNITS_COVERAGE_WRITE"))ok=false;
    if(out){if(ok&&fflush(out))ok=false;if(ok&&fsync(fd))ok=false;if(fclose(out))ok=false;}else close(fd);
    if(ok&&rename(temp,path))ok=false;if(!ok)unlink(temp);free(temp);return ok;
}
