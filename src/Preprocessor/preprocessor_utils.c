// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

#include "Compiler/diagnostics.h"
#include "Lexer/tokens.h"

enum {
    PP_INCLUDE_STACK_SLOT_EMPTY = 0,
    PP_INCLUDE_STACK_SLOT_ACTIVE = 1,
    PP_INCLUDE_STACK_SLOT_TOMBSTONE = 2
};

static size_t pp_hash_bytes(const char* text) {
    const unsigned char* bytes = (const unsigned char*)text;
    size_t hash = (size_t)1469598103934665603ull;
    while (bytes && *bytes) {
        hash ^= (size_t)(*bytes++);
        hash *= (size_t)1099511628211ull;
    }
    return hash;
}

static size_t pp_include_stack_find_slot(const Preprocessor* pp,
                                         const char* path,
                                         bool* foundOut) {
    size_t capacity = pp->includeStack.memberCapacity;
    size_t mask = capacity - 1;
    size_t slot = pp_hash_bytes(path) & mask;
    size_t firstTombstone = SIZE_MAX;
    while (true) {
        unsigned char state = pp->includeStack.memberStates[slot];
        if (state == PP_INCLUDE_STACK_SLOT_EMPTY) {
            if (foundOut) *foundOut = false;
            return firstTombstone != SIZE_MAX ? firstTombstone : slot;
        }
        if (state == PP_INCLUDE_STACK_SLOT_TOMBSTONE) {
            if (firstTombstone == SIZE_MAX) {
                firstTombstone = slot;
            }
        } else if (pp->includeStack.memberPaths[slot] &&
                   strcmp(pp->includeStack.memberPaths[slot], path) == 0) {
            if (foundOut) *foundOut = true;
            return slot;
        }
        slot = (slot + 1) & mask;
    }
}

static bool pp_include_stack_member_rehash(Preprocessor* pp, size_t newCapacity) {
    const char** newPaths = calloc(newCapacity, sizeof(*newPaths));
    size_t* newCounts = calloc(newCapacity, sizeof(*newCounts));
    unsigned char* newStates = calloc(newCapacity, sizeof(*newStates));
    if (!newPaths || !newCounts || !newStates) {
        free(newPaths);
        free(newCounts);
        free(newStates);
        return false;
    }

    const char** oldPaths = pp->includeStack.memberPaths;
    size_t* oldCounts = pp->includeStack.memberCounts;
    unsigned char* oldStates = pp->includeStack.memberStates;
    size_t oldCapacity = pp->includeStack.memberCapacity;

    pp->includeStack.memberPaths = newPaths;
    pp->includeStack.memberCounts = newCounts;
    pp->includeStack.memberStates = newStates;
    pp->includeStack.memberCapacity = newCapacity;
    pp->includeStack.memberTombstones = 0;

    for (size_t i = 0; i < oldCapacity; ++i) {
        if (!oldStates || oldStates[i] != PP_INCLUDE_STACK_SLOT_ACTIVE || !oldPaths[i] ||
            oldCounts[i] == 0) {
            continue;
        }
        bool found = false;
        size_t slot = pp_include_stack_find_slot(pp, oldPaths[i], &found);
        pp->includeStack.memberPaths[slot] = oldPaths[i];
        pp->includeStack.memberCounts[slot] = oldCounts[i];
        pp->includeStack.memberStates[slot] = PP_INCLUDE_STACK_SLOT_ACTIVE;
    }

    free(oldPaths);
    free(oldCounts);
    free(oldStates);
    return true;
}

static bool pp_include_stack_member_ensure_capacity(Preprocessor* pp, size_t nextDepth) {
    size_t capacity = pp->includeStack.memberCapacity;
    if (capacity == 0) {
        return pp_include_stack_member_rehash(pp, 16);
    }
    size_t active = pp->includeStack.depth;
    if ((active + pp->includeStack.memberTombstones + 1) * 2 <= capacity &&
        nextDepth * 2 <= capacity) {
        return true;
    }
    size_t newCapacity = capacity;
    while ((nextDepth + 1) * 2 > newCapacity) {
        newCapacity *= 2;
    }
    if ((active + pp->includeStack.memberTombstones + 1) * 2 > newCapacity) {
        newCapacity *= 2;
    }
    return pp_include_stack_member_rehash(pp, newCapacity);
}

static void strip_include_spaces_local(char* name) {
    if (!name) return;
    char* out = name;
    for (char* p = name; *p; ++p) {
        if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            *out++ = *p;
        }
    }
    *out = '\0';
}

static bool is_builtin_probe_macro_name(const char* name, const char** fallbackValue) {
    if (!name || !fallbackValue) return false;
    if (strcmp(name, "__has_builtin") == 0 ||
        strcmp(name, "__has_extension") == 0 ||
        strcmp(name, "__has_feature") == 0 ||
        strcmp(name, "__has_attribute") == 0 ||
        strcmp(name, "__has_c_attribute") == 0 ||
        strcmp(name, "__has_declspec_attribute") == 0 ||
        strcmp(name, "__has_warning") == 0) {
        *fallbackValue = "0";
        return true;
    }
    if (strcmp(name, "__is_identifier") == 0) {
        *fallbackValue = "1";
        return true;
    }
    return false;
}

static bool consume_parenthesized_expression(const Token* tokens,
                                             size_t count,
                                             size_t lparenIndex,
                                             size_t* outNextIndex) {
    if (!tokens || !outNextIndex || lparenIndex >= count || tokens[lparenIndex].type != TOKEN_LPAREN) {
        return false;
    }
    int depth = 0;
    for (size_t i = lparenIndex; i < count; ++i) {
        if (tokens[i].type == TOKEN_EOF) {
            return false;
        }
        if (tokens[i].type == TOKEN_LPAREN) {
            depth++;
            continue;
        }
        if (tokens[i].type == TOKEN_RPAREN) {
            depth--;
            if (depth == 0) {
                *outNextIndex = i + 1;
                return true;
            }
        }
    }
    return false;
}

static bool append_expr_token(Preprocessor* pp, PPTokenBuffer* out, const Token* tok) {
    if (!tok) return true;
    if (tok->type == TOKEN_IDENTIFIER && tok->value) {
        if (strcmp(tok->value, "__LINE__") == 0) {
            char buffer[32];
            const Token remapped = pp_token_clone_remap(pp, tok);
            snprintf(buffer, sizeof(buffer), "%d", remapped.location.start.line);
            return pp_append_number_literal(pp, out, tok, buffer);
        }
        if (strcmp(tok->value, "__COUNTER__") == 0) {
            int value = pp ? pp->counter : 0;
            if (pp) pp->counter++;
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", value);
            return pp_append_number_literal(pp, out, tok, buffer);
        }
        if (strcmp(tok->value, "__FILE__") == 0 ||
            strcmp(tok->value, "__BASE_FILE__") == 0 ||
            strcmp(tok->value, "__DATE__") == 0 ||
            strcmp(tok->value, "__TIME__") == 0) {
            return pp_append_number_literal(pp, out, tok, "0");
        }
    }
    return pp_token_buffer_append_clone_remap(out, pp, tok);
}

void pp_token_buffer_init_local(PPTokenBuffer* buffer) {
    if (!buffer) return;
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

bool pp_token_buffer_reserve(PPTokenBuffer* buffer, size_t extra) {
    if (!buffer) return false;
    size_t required = buffer->count + extra;
    if (required <= buffer->capacity) {
        return true;
    }
    size_t newCapacity = buffer->capacity ? buffer->capacity * 2 : 16;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    Token* tokens = (Token*)realloc(buffer->tokens, newCapacity * sizeof(Token));
    if (!tokens) {
        return false;
    }
    buffer->tokens = tokens;
    buffer->capacity = newCapacity;
    return true;
}

char* pp_strdup_local(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static void safe_free_token_value(char* value) {
    if (!value) return;
#if defined(__APPLE__)
    if (malloc_zone_from_ptr(value) == NULL) {
        return;
    }
#endif
    free(value);
}

static bool token_value_needs_dup(const char* value) {
    if (!value) return false;
#if defined(__APPLE__)
    return malloc_zone_from_ptr((void*)value) != NULL;
#else
    return true;
#endif
}

void pp_token_free(Token* tok) {
    if (!tok) return;
    safe_free_token_value(tok->value);
    tok->value = NULL;
}

Token pp_token_clone(const Token* tok) {
    Token clone = *tok;
    if (tok->value && token_value_needs_dup(tok->value)) {
        clone.value = pp_strdup_local(tok->value);
    }
    return clone;
}

Token pp_token_clone_remap(Preprocessor* pp, const Token* tok) {
    Token clone = pp_token_clone(tok);
    if (!pp) return clone;
    if (pp->lineRemapActive) {
        const char* logicalFile = pp->logicalFile ? pp->logicalFile : clone.location.start.file;
        if (logicalFile) {
            clone.location.start.file = logicalFile;
            clone.location.end.file = logicalFile;
        }
        if (pp->lineOffset != 0) {
            clone.location.start.line += pp->lineOffset;
            clone.location.end.line += pp->lineOffset;
            if (clone.location.start.line < 1) clone.location.start.line = 1;
            if (clone.location.end.line < 1) clone.location.end.line = 1;
        }
        if (clone.macroCallSite.start.file || clone.macroCallSite.start.line > 0) {
            if (logicalFile) {
                clone.macroCallSite.start.file = logicalFile;
                clone.macroCallSite.end.file = logicalFile;
            }
            if (pp->lineOffset != 0) {
                clone.macroCallSite.start.line += pp->lineOffset;
                clone.macroCallSite.end.line += pp->lineOffset;
                if (clone.macroCallSite.start.line < 1) clone.macroCallSite.start.line = 1;
                if (clone.macroCallSite.end.line < 1) clone.macroCallSite.end.line = 1;
            }
        }
    }
    clone.line = clone.location.start.line;
    return clone;
}

bool pp_token_buffer_append_token(PPTokenBuffer* buffer, Token token) {
    if (!pp_token_buffer_reserve(buffer, 1)) {
        pp_token_free(&token);
        return false;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok) {
    Token clone = pp_token_clone(tok);
    if (!pp_token_buffer_append_token(buffer, clone)) {
        pp_token_free(&clone);
        return false;
    }
    return true;
}

bool pp_token_buffer_append_clone_remap(PPTokenBuffer* buffer,
                                        Preprocessor* pp,
                                        const Token* tok) {
    Token clone = pp_token_clone_remap(pp, tok);
    if (!pp_token_buffer_append_token(buffer, clone)) {
        pp_token_free(&clone);
        return false;
    }
    return true;
}

bool pp_token_buffer_append_clone_remap_span(PPTokenBuffer* buffer,
                                             Preprocessor* pp,
                                             const Token* tokens,
                                             size_t count) {
    if (!buffer || !tokens || count == 0) return true;
    if (!pp_token_buffer_reserve(buffer, count)) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        Token clone = pp_token_clone_remap(pp, &tokens[i]);
        buffer->tokens[buffer->count++] = clone;
    }
    return true;
}

bool pp_token_buffer_append_buffer(PPTokenBuffer* dest, const PPTokenBuffer* src) {
    if (!dest || !src) return true;
    for (size_t i = 0; i < src->count; ++i) {
        if (!pp_token_buffer_append_clone(dest, &src->tokens[i])) {
            return false;
        }
    }
    return true;
}

void pp_token_buffer_reset(PPTokenBuffer* buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->count; ++i) {
        pp_token_free(&buffer->tokens[i]);
    }
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

void skip_to_line_end(const Token* tokens, size_t count, size_t* cursor) {
    size_t i = *cursor;
    int line = tokens[i].line;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
}

bool pp_debug_fail_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char* env = getenv("FISICS_DEBUG_PP_FAIL");
        cached = (env && env[0]) ? 1 : 0;
    }
    return cached != 0;
}

void pp_debug_fail(const char* label, const Token* tok) {
    if (!pp_debug_fail_enabled()) return;
    const char* file = tok && tok->location.start.file ? tok->location.start.file : "<unknown>";
    int line = tok ? tok->location.start.line : 0;
    int col = tok ? tok->location.start.column : 0;
    fprintf(stderr, "[PP-DEBUG] failure at %s %s:%d:%d (tok type=%d)\n",
            label ? label : "<unknown>", file, line, col, tok ? tok->type : -1);
}

const char* token_file(const Token* tok) {
    if (!tok || !tok->location.start.file) return NULL;
    return tok->location.start.file;
}

static bool pp_record_logical_file(Preprocessor* pp, const char* path, const char** outPtr) {
    if (!pp || !path) return false;
    if (pp->logicalFileCount == pp->logicalFileCap) {
        size_t newCap = pp->logicalFileCap ? pp->logicalFileCap * 2 : 4;
        char** files = realloc(pp->logicalFilePool, newCap * sizeof(char*));
        if (!files) return false;
        pp->logicalFilePool = files;
        pp->logicalFileCap = newCap;
    }
    char* copy = pp_strdup_local(path);
    if (!copy) return false;
    pp->logicalFilePool[pp->logicalFileCount++] = copy;
    if (outPtr) *outPtr = copy;
    return true;
}

bool pp_set_base_file(Preprocessor* pp, const char* path) {
    if (!pp) return false;
    if (pp->baseFile) return true;
    pp->baseFile = path ? pp_strdup_local(path) : NULL;
    return path ? pp->baseFile != NULL : true;
}

bool pp_set_logical_file(Preprocessor* pp, const char* path) {
    if (!pp || !path) return false;
    const char* stored = NULL;
    if (!pp_record_logical_file(pp, path, &stored)) {
        return false;
    }
    pp->logicalFile = (char*)stored;
    return true;
}

bool pp_append_number_literal(Preprocessor* pp,
                              PPTokenBuffer* buffer,
                              const Token* base,
                              const char* text) {
    Token clone = pp_token_clone_remap(pp, base);
    clone.type = TOKEN_NUMBER;
    if (clone.value) free(clone.value);
    clone.value = text ? pp_strdup_local(text) : NULL;
    bool ok = pp_token_buffer_append_token(buffer, clone);
    if (!ok) {
        pp_token_free(&clone);
    }
    return ok;
}

bool pp_prepare_expr_tokens(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            PPTokenBuffer* out) {
    pp_token_buffer_init_local(out);
    if (!tokens || count == 0) return true;

    size_t i = 0;
    while (i < count) {
        const Token* tok = &tokens[i];
        if (pp && tok->value &&
            (strcmp(tok->value, "__has_include") == 0 ||
             strcmp(tok->value, "__has_include_next") == 0)) {
            bool isIncludeNext = strcmp(tok->value, "__has_include_next") == 0;
            size_t j = i + 1;
            bool hasParen = false;
            if (j < count && tokens[j].type == TOKEN_LPAREN) {
                hasParen = true;
                j++;
            }
            bool parsed = false;
            bool isSystem = false;
            char nameBuf[1024];
            nameBuf[0] = '\0';

            if (j < count && tokens[j].type == TOKEN_STRING && tokens[j].value) {
                snprintf(nameBuf, sizeof(nameBuf), "%s", tokens[j].value);
                strip_include_spaces_local(nameBuf);
                j++;
                parsed = true;
            } else if (j < count && tokens[j].type == TOKEN_LESS) {
                j++;
                size_t len = 0;
                bool closed = false;
                while (j < count && tokens[j].type != TOKEN_EOF) {
                    if (tokens[j].type == TOKEN_GREATER) {
                        closed = true;
                        j++;
                        break;
                    }
                    const char* piece = tokens[j].value ? tokens[j].value : "";
                    size_t pieceLen = strlen(piece);
                    if (len + pieceLen + 1 >= sizeof(nameBuf)) {
                        parsed = false;
                        break;
                    }
                    memcpy(nameBuf + len, piece, pieceLen);
                    len += pieceLen;
                    j++;
                }
                if (closed) {
                    nameBuf[len] = '\0';
                    strip_include_spaces_local(nameBuf);
                    isSystem = true;
                    parsed = true;
                }
            }
            if (hasParen && j < count && tokens[j].type == TOKEN_RPAREN) {
                j++;
            }

            int present = 0;
            if (parsed && pp && pp->resolver && nameBuf[0]) {
                IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
                size_t originIndex = (size_t)-1;
                const char* parent = token_file(tok);
                const IncludeFile* inc = include_resolver_load(pp->resolver,
                                                               parent,
                                                               nameBuf,
                                                               isSystem,
                                                               isIncludeNext,
                                                               &origin,
                                                               &originIndex);
                present = (inc != NULL) ? 1 : 0;
            }
            if (!pp_append_number_literal(pp, out, tok, present ? "1" : "0")) {
                return false;
            }
            i = (j > i) ? j : (i + 1);
            continue;
        }
        if (tok->value) {
            const char* fallback = NULL;
            if (is_builtin_probe_macro_name(tok->value, &fallback)) {
                const MacroDefinition* def = (pp && pp->table)
                    ? macro_table_lookup(pp->table, tok->value)
                    : NULL;
                if (!def) {
                    size_t next = i + 1;
                    size_t afterCall = 0;
                    if (next < count &&
                        tokens[next].type == TOKEN_LPAREN &&
                        consume_parenthesized_expression(tokens, count, next, &afterCall)) {
                        if (!pp_append_number_literal(pp, out, tok, fallback)) {
                            return false;
                        }
                        i = afterCall;
                        continue;
                    }
                }
            }
        }
        // Preserve defined operand without expanding it.
        if (tok->type == TOKEN_IDENTIFIER && tok->value &&
            strcmp(tok->value, "defined") == 0) {
            if (!append_expr_token(pp, out, tok)) return false;
            i++;
            if (i < count && tokens[i].type == TOKEN_LPAREN) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            if (i < count) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            if (i < count && tokens[i].type == TOKEN_RPAREN) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            continue;
        }

        // Collect a span until the next defined and expand macros inside it.
        size_t start = i;
        while (i < count) {
            const Token* t = &tokens[i];
            if (t->value &&
                (strcmp(t->value, "defined") == 0 ||
                 strcmp(t->value, "__has_include") == 0 ||
                 strcmp(t->value, "__has_include_next") == 0)) {
                break;
            }
            if (t->type == TOKEN_EOF) {
                break;
            }
            i++;
        }
        size_t span = i - start;
        if (span > 0) {
            PPTokenBuffer expanded = {0};
            macro_expander_set_preserve_defined_operands(&pp->expander, true);
            if (!macro_expander_expand(&pp->expander, tokens + start, span, &expanded)) {
                macro_expander_set_preserve_defined_operands(&pp->expander, false);
                pp_token_buffer_destroy(&expanded);
                return false;
            }
            macro_expander_set_preserve_defined_operands(&pp->expander, false);
            for (size_t j = 0; j < expanded.count; ++j) {
                if (!append_expr_token(pp, out, &expanded.tokens[j])) {
                    pp_token_buffer_destroy(&expanded);
                    return false;
                }
            }
            pp_token_buffer_destroy(&expanded);
        }
    }
    const char* debugExpr = getenv("DEBUG_PP_EXPR");
    if (debugExpr && debugExpr[0] != '\0') {
        fprintf(stderr, "PP expr tokens (%zu):\n", out->count);
        for (size_t k = 0; k < out->count; ++k) {
            const Token* t = &out->tokens[k];
            fprintf(stderr, "  [%zu] type=%d val=%s\n",
                    k,
                    t->type,
                    t->value ? t->value : "<null>");
        }
    }
    return true;
}

bool pp_include_stack_push(Preprocessor* pp,
                           const char* path,
                           IncludeSearchOrigin origin,
                           size_t originIndex) {
    if (!pp || !path) return false;
    if (pp->includeStack.depth == pp->includeStack.capacity) {
        size_t newCap = pp->includeStack.capacity ? pp->includeStack.capacity * 2 : 4;
        PPIncludeFrame* frames = realloc(pp->includeStack.frames, newCap * sizeof(PPIncludeFrame));
        if (!frames) return false;
        pp->includeStack.frames = frames;
        pp->includeStack.capacity = newCap;
    }
    if (!pp_include_stack_member_ensure_capacity(pp, pp->includeStack.depth + 1)) {
        return false;
    }
    pp->includeStack.frames[pp->includeStack.depth].path = path;
    pp->includeStack.frames[pp->includeStack.depth].origin = origin;
    pp->includeStack.frames[pp->includeStack.depth].originIndex = originIndex;
    bool found = false;
    size_t slot = pp_include_stack_find_slot(pp, path, &found);
    if (!found && pp->includeStack.memberStates[slot] == PP_INCLUDE_STACK_SLOT_TOMBSTONE &&
        pp->includeStack.memberTombstones > 0) {
        pp->includeStack.memberTombstones--;
    }
    pp->includeStack.memberPaths[slot] = path;
    pp->includeStack.memberCounts[slot]++;
    pp->includeStack.memberStates[slot] = PP_INCLUDE_STACK_SLOT_ACTIVE;
    pp->includeStack.depth++;
    return true;
}

void pp_include_stack_pop(Preprocessor* pp) {
    if (!pp || pp->includeStack.depth == 0) return;
    const char* path = pp->includeStack.frames[pp->includeStack.depth - 1].path;
    if (path && pp->includeStack.memberCapacity > 0) {
        bool found = false;
        size_t slot = pp_include_stack_find_slot(pp, path, &found);
        if (found && pp->includeStack.memberCounts[slot] > 0) {
            pp->includeStack.memberCounts[slot]--;
            if (pp->includeStack.memberCounts[slot] == 0) {
                pp->includeStack.memberPaths[slot] = NULL;
                pp->includeStack.memberStates[slot] = PP_INCLUDE_STACK_SLOT_TOMBSTONE;
                pp->includeStack.memberTombstones++;
            }
        }
    }
    pp->includeStack.depth--;
}

const PPIncludeFrame* pp_include_stack_top(const Preprocessor* pp) {
    if (!pp || pp->includeStack.depth == 0) return NULL;
    return &pp->includeStack.frames[pp->includeStack.depth - 1];
}

bool pp_include_stack_contains(const Preprocessor* pp, const char* path) {
    if (!pp || !path || pp->includeStack.memberCapacity == 0) return false;
    bool found = false;
    size_t slot = pp_include_stack_find_slot(pp, path, &found);
    return found && pp->includeStack.memberCounts[slot] > 0;
}

void pp_report_diag(Preprocessor* pp,
                    const Token* tok,
                    DiagKind kind,
                    int code,
                    const char* fmt,
                    ...) {
    if (!pp || !pp->ctx || !fmt) return;
    SourceRange loc = (SourceRange){0};
    if (tok) {
        Token remapped = pp_token_clone_remap(pp, tok);
        loc = remapped.location;
        pp_token_free(&remapped);
    }
    if (!loc.start.file) {
        const PPIncludeFrame* top = pp_include_stack_top(pp);
        if (top && top->path) {
            loc.start.file = top->path;
            loc.end.file = top->path;
        }
    }
    va_list args;
    va_start(args, fmt);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    compiler_report_diag(pp->ctx,
                         loc,
                         kind,
                         code,
                         NULL,
                         "%s",
                         buffer);
}
