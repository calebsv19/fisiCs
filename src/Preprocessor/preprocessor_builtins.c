// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compiler/compiler_context.h"
#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"

static bool tokenize_macro_replacement(const char* value, PPTokenBuffer* out) {
    if (!out) return false;
    pp_token_buffer_init_local(out);
    if (!value || value[0] == '\0') {
        return true;
    }

    Lexer lexer;
    initLexer(&lexer, value, "<command-line-macro>", false);

    TokenBuffer raw = {0};
    token_buffer_init(&raw);
    bool ok = token_buffer_fill_from_lexer(&raw, &lexer);
    destroyLexer(&lexer);
    if (!ok) {
        token_buffer_destroy(&raw);
        return false;
    }

    for (size_t i = 0; i < raw.count; ++i) {
        if (raw.tokens[i].type == TOKEN_EOF) break;
        if (!pp_token_buffer_append_clone(out, &raw.tokens[i])) {
            ok = false;
            break;
        }
    }
    token_buffer_destroy(&raw);
    if (!ok) {
        pp_token_buffer_reset(out);
    }
    return ok;
}

static char* normalize_cli_macro_value(const char* value) {
    if (!value) return NULL;
    size_t len = strlen(value);
    char* normalized = (char*)malloc(len + 1);
    if (!normalized) return NULL;

    size_t w = 0;
    for (size_t i = 0; i < len; ++i) {
        if (value[i] == '\\') {
            size_t j = i;
            while (j < len && value[j] == '\\') {
                ++j;
            }
            if (j < len && value[j] == '"') {
                normalized[w++] = '"';
                i = j;
                continue;
            }
        }
        normalized[w++] = value[i];
    }
    normalized[w] = '\0';
    return normalized;
}

static bool try_tokenize_cli_quoted_string(const char* value, PPTokenBuffer* out) {
    if (!value || !out) return false;
    size_t len = strlen(value);
    if (len < 2) return false;
    if (value[0] != '"' || value[len - 1] != '"') return false;

    Token tok = {0};
    tok.type = TOKEN_STRING;
    tok.value = (char*)malloc(len - 1);
    if (!tok.value) return false;
    memcpy(tok.value, value + 1, len - 2);
    tok.value[len - 2] = '\0';
    if (!pp_token_buffer_append_token(out, tok)) {
        pp_token_free(&tok);
        return false;
    }
    return true;
}

static void define_builtin_object(Preprocessor* pp, const char* name, const char* value) {
    if (!pp || !name) return;
    PPTokenBuffer body = {0};
    if (!tokenize_macro_replacement(value, &body)) {
        return;
    }
    macro_table_define_object(pp->table, name, body.tokens, body.count, (SourceRange){0});
    pp_token_buffer_reset(&body);
}

static void define_builtin_function(Preprocessor* pp,
                                    const char* name,
                                    const char* const* params,
                                    size_t paramCount,
                                    bool variadic,
                                    const char* value) {
    if (!pp || !name) return;
    PPTokenBuffer body = {0};
    if (!tokenize_macro_replacement(value, &body)) {
        return;
    }
    macro_table_define_function(pp->table,
                                name,
                                params,
                                paramCount,
                                variadic,
                                false,
                                body.tokens,
                                body.count,
                                (SourceRange){0});
    pp_token_buffer_reset(&body);
}

static void define_builtin_empty_macro(Preprocessor* pp, const char* name) {
    define_builtin_object(pp, name, "");
    macro_table_define_function(pp->table,
                                name,
                                NULL,
                                0,
                                true,
                                false,
                                NULL,
                                0,
                                (SourceRange){0});
}

static void define_default_builtins(Preprocessor* pp) {
    define_builtin_object(pp, "__APPLE__", "1");
    define_builtin_object(pp, "__MACH__", "1");
    define_builtin_object(pp, "__APPLE_CC__", "1");
    define_builtin_object(pp, "__APPLE_CPP__", "1");
    define_builtin_object(pp, "__arm64__", "1");
    define_builtin_object(pp, "__LP64__", "1");
    define_builtin_object(pp, "__clang__", "1");
    define_builtin_object(pp, "__GNUC__", "4");
    define_builtin_object(pp, "__GNUC_MINOR__", "2");
    define_builtin_object(pp, "__GNUC_PATCHLEVEL__", "1");
    define_builtin_object(pp, "__STDC_HOSTED__", "1");
    {
        long stdc = 199901L;
        if (pp->ctx) {
            stdc = cc_dialect_stdc_version(cc_get_language_dialect(pp->ctx));
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%ldL", stdc);
        define_builtin_object(pp, "__STDC_VERSION__", buf);
        define_builtin_object(pp, "__FISICS__", "1");
        snprintf(buf, sizeof(buf), "%ldL", stdc);
        define_builtin_object(pp, "__FISICS_DIALECT__", buf);
        if (pp->ctx && cc_has_any_compat_features(pp->ctx)) {
            define_builtin_object(pp, "__FISICS_EXTENSIONS__", "1");
        }
    }
    define_builtin_object(pp, "MAC_OS_X_VERSION_MIN_REQUIRED", "130000");
    define_builtin_object(pp, "__FLT_MAX__", "3.402823466e+38F");
    define_builtin_object(pp, "__DBL_MAX__", "1.7976931348623157e+308");
    define_builtin_object(pp, "__LDBL_MAX__", "1.189731495357231765e+4932L");
    define_builtin_object(pp, "__FLT_MIN__", "1.175494351e-38F");
    define_builtin_object(pp, "__DBL_MIN__", "2.2250738585072014e-308");
    define_builtin_object(pp, "__LDBL_MIN__", "3.3621031431120935063e-4932L");
    define_builtin_object(pp, "__FLT_EPSILON__", "1.19209290e-7F");
    define_builtin_object(pp, "__DBL_EPSILON__", "2.2204460492503131e-16");
    define_builtin_object(pp, "__LDBL_EPSILON__", "1.0842021724855044e-19L");
    define_builtin_object(pp, "__ATOMIC_RELAXED", "0");
    define_builtin_object(pp, "__ATOMIC_CONSUME", "1");
    define_builtin_object(pp, "__ATOMIC_ACQUIRE", "2");
    define_builtin_object(pp, "__ATOMIC_RELEASE", "3");
    define_builtin_object(pp, "__ATOMIC_ACQ_REL", "4");
    define_builtin_object(pp, "__ATOMIC_SEQ_CST", "5");
    define_builtin_object(pp, "__ORDER_LITTLE_ENDIAN__", "1234");
    define_builtin_object(pp, "__ORDER_BIG_ENDIAN__", "4321");
    define_builtin_object(pp, "__ORDER_PDP_ENDIAN__", "3412");
    define_builtin_object(pp, "__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__");
    define_builtin_object(pp, "__LITTLE_ENDIAN__", "1");
    define_builtin_object(pp, "_LITTLE_ENDIAN", "1");
    define_builtin_object(pp, "LITTLE_ENDIAN", "1234");
    define_builtin_object(pp, "BIG_ENDIAN", "4321");
    define_builtin_object(pp, "BYTE_ORDER", "LITTLE_ENDIAN");

    {
        const char* params[] = {"x"};
        define_builtin_function(pp, "__has_builtin", params, 1, false, "0");
        define_builtin_function(pp, "__has_extension", params, 1, false, "0");
        define_builtin_function(pp, "__has_feature", params, 1, false, "0");
        define_builtin_function(pp, "__has_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_c_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_declspec_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_warning", params, 1, false, "0");
        define_builtin_function(pp, "__is_identifier", params, 1, false, "1");
    }
}

static void define_noop_macro_groups(Preprocessor* pp) {
    const char* libc_bounds_macros[] = {
        "_LIBC_SINGLE_BY_DEFAULT",
        "_LIBC_PTRCHECK_REPLACED",
        "_LIBC_COUNT",
        "_LIBC_COUNT_OR_NULL",
        "_LIBC_SIZE",
        "_LIBC_SIZE_OR_NULL",
        "_LIBC_ENDED_BY",
        "_LIBC_SINGLE",
        "_LIBC_UNSAFE_INDEXABLE",
        "_LIBC_CSTR",
        "_LIBC_NULL_TERMINATED",
        "_LIBC_FLEX_COUNT",
        NULL};
    for (size_t i = 0; libc_bounds_macros[i]; ++i) {
        define_builtin_empty_macro(pp, libc_bounds_macros[i]);
    }

    const char* availability_macros[] = {
        "__API_AVAILABLE",
        "__API_DEPRECATED",
        "__API_DEPRECATED_WITH_REPLACEMENT",
        "__API_UNAVAILABLE",
        "__API_AVAILABLE_BEGIN",
        "__API_AVAILABLE_END",
        "__OSX_AVAILABLE_STARTING",
        "__OSX_AVAILABLE_BUT_DEPRECATED",
        "__OSX_DEPRECATED",
        "__IOS_AVAILABLE",
        "__IOS_DEPRECATED",
        "__IOS_UNAVAILABLE",
        "__TVOS_AVAILABLE",
        "__TVOS_DEPRECATED",
        "__TVOS_UNAVAILABLE",
        "__WATCHOS_AVAILABLE",
        "__WATCHOS_DEPRECATED",
        "__WATCHOS_UNAVAILABLE",
        "__WATCHOS_PROHIBITED",
        "__TVOS_PROHIBITED",
        "__IOS_PROHIBITED",
        "__MAC_OS_X_VERSION_MIN_REQUIRED",
        "__MAC_OS_X_VERSION_MAX_ALLOWED",
        NULL};
    for (size_t i = 0; availability_macros[i]; ++i) {
        define_builtin_empty_macro(pp, availability_macros[i]);
    }
}

static void define_cli_macros(Preprocessor* pp,
                              const char* const* macroDefines,
                              size_t macroDefineCount) {
    for (size_t i = 0; i < macroDefineCount; ++i) {
        const char* def = macroDefines ? macroDefines[i] : NULL;
        if (!def || def[0] == '\0') continue;
        if (def[0] == '-' && def[1] == 'D') {
            def += 2;
            if (def[0] == '\0') continue;
        }
        const char* eq = strchr(def, '=');
        char* name = NULL;
        const char* value = NULL;
        if (eq) {
            size_t nameLen = (size_t)(eq - def);
            name = (char*)malloc(nameLen + 1);
            if (!name) continue;
            memcpy(name, def, nameLen);
            name[nameLen] = '\0';
            value = eq + 1;
        } else {
            name = strdup(def);
            if (!name) continue;
        }

        char* normalizedValue = normalize_cli_macro_value(value);
        const char* valueForLex = normalizedValue ? normalizedValue : value;
        PPTokenBuffer body = {0};
        if (valueForLex && valueForLex[0] != '\0' &&
            try_tokenize_cli_quoted_string(valueForLex, &body)) {
            /* already tokenized as a quoted string replacement */
        } else if (!tokenize_macro_replacement(valueForLex, &body)) {
            free(normalizedValue);
            free(name);
            continue;
        }

        macro_table_define_object(pp->table,
                                  name,
                                  body.tokens,
                                  body.count,
                                  (SourceRange){0});
        pp_token_buffer_reset(&body);
        free(normalizedValue);
        free(name);
    }
}

void pp_define_predefined_macros(Preprocessor* pp,
                                 const char* const* macroDefines,
                                 size_t macroDefineCount) {
    if (!pp || !pp->table) return;
    define_default_builtins(pp);
    define_noop_macro_groups(pp);
    define_cli_macros(pp, macroDefines, macroDefineCount);
}
