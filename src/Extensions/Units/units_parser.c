// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* text;
    size_t pos;
} DimCursor;

static void skip_ws(DimCursor* cur) {
    while (cur && cur->text[cur->pos] &&
           (cur->text[cur->pos] == ' ' || cur->text[cur->pos] == '\t' ||
            cur->text[cur->pos] == '\n' || cur->text[cur->pos] == '\r')) {
        cur->pos++;
    }
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '-';
}

static char* dup_slice(const char* start, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char* dup_printf1(const char* fmt, const char* a) {
    size_t need = (size_t)snprintf(NULL, 0, fmt, a ? a : "");
    char* out = (char*)malloc(need + 1);
    if (!out) return NULL;
    snprintf(out, need + 1, fmt, a ? a : "");
    return out;
}

static char* dup_printf2(const char* fmt, const char* a, const char* b) {
    size_t need = (size_t)snprintf(NULL, 0, fmt, a ? a : "", b ? b : "");
    char* out = (char*)malloc(need + 1);
    if (!out) return NULL;
    snprintf(out, need + 1, fmt, a ? a : "", b ? b : "");
    return out;
}

static bool parse_int(DimCursor* cur, int* outValue) {
    skip_ws(cur);
    int sign = 1;
    if (cur->text[cur->pos] == '+') {
        cur->pos++;
    } else if (cur->text[cur->pos] == '-') {
        sign = -1;
        cur->pos++;
    }
    if (!isdigit((unsigned char)cur->text[cur->pos])) {
        return false;
    }
    int value = 0;
    while (isdigit((unsigned char)cur->text[cur->pos])) {
        value = value * 10 + (cur->text[cur->pos] - '0');
        cur->pos++;
    }
    *outValue = value * sign;
    return true;
}

static bool parse_atom(DimCursor* cur, FisicsDim8* outDim, char** outErrorDetail) {
    skip_ws(cur);
    size_t start = cur->pos;
    while (is_ident_char(cur->text[cur->pos])) {
        cur->pos++;
    }
    if (cur->pos == start) {
        if (outErrorDetail) *outErrorDetail = strdup("expected a dimension name");
        return false;
    }
    char* ident = dup_slice(cur->text + start, cur->pos - start);
    if (!ident) {
        if (outErrorDetail) *outErrorDetail = strdup("out of memory");
        return false;
    }

    FisicsDimAtomInfo atomInfo;
    bool ok = fisics_dim_lookup_atom(ident, &atomInfo);
    if (!ok || atomInfo.kind == FISICS_DIM_ATOM_UNKNOWN) {
        if (outErrorDetail) *outErrorDetail = dup_printf1("unknown dimension atom '%s'", ident);
        free(ident);
        return false;
    }
    if (atomInfo.kind == FISICS_DIM_ATOM_DEFERRED_UNIT) {
        if (outErrorDetail) {
            *outErrorDetail = dup_printf2(
                "concrete unit word '%s' belongs in [[fisics::unit(...)]] rather than fisics::dim(...) (%s family)",
                ident,
                atomInfo.family_name ? atomInfo.family_name : "unknown");
        }
        free(ident);
        return false;
    }
    FisicsDim8 base = atomInfo.dim;
    free(ident);

    skip_ws(cur);
    if (cur->text[cur->pos] == '^') {
        cur->pos++;
        int power = 0;
        if (!parse_int(cur, &power)) {
            if (outErrorDetail) *outErrorDetail = strdup("expected integer exponent after '^'");
            return false;
        }
        if (!fisics_dim_scale(base, power, &base)) {
            if (outErrorDetail) *outErrorDetail = strdup("dimension exponent overflow");
            return false;
        }
    }

    *outDim = base;
    return true;
}

static bool extract_cxx_attr_payload(const ASTAttribute* attr,
                                     const char* prefix,
                                     char** outText) {
    if (outText) *outText = NULL;
    if (!attr || attr->syntax != AST_ATTRIBUTE_SYNTAX_CXX || !attr->payload || !prefix) {
        return false;
    }
    size_t prefixLen = strlen(prefix);
    if (strncmp(attr->payload, prefix, prefixLen) != 0) {
        return false;
    }
    size_t payloadLen = strlen(attr->payload);
    if (payloadLen < prefixLen + 1 || attr->payload[payloadLen - 1] != ')') {
        return false;
    }
    if (!outText) {
        return true;
    }
    *outText = dup_slice(attr->payload + prefixLen, payloadLen - prefixLen - 1);
    return *outText != NULL;
}

bool fisics_units_dim_attribute_extract(const ASTAttribute* attr, char** outExprText) {
    return extract_cxx_attr_payload(attr, "fisics::dim(", outExprText);
}

bool fisics_units_unit_attribute_extract(const ASTAttribute* attr, char** outUnitText) {
    return extract_cxx_attr_payload(attr, "fisics::unit(", outUnitText);
}

bool fisics_units_attribute_extract(const ASTAttribute* attr, char** outExprText) {
    if (outExprText) *outExprText = NULL;
    return fisics_units_dim_attribute_extract(attr, outExprText);
}

bool fisics_units_parse_dim_expr(const char* text, FisicsDim8* outDim, char** outErrorDetail) {
    if (outErrorDetail) *outErrorDetail = NULL;
    if (!outDim) return false;

    DimCursor cur = { text ? text : "", 0 };
    skip_ws(&cur);
    if (cur.text[cur.pos] == '\0') {
        *outDim = fisics_dim_zero();
        return true;
    }

    FisicsDim8 accum = fisics_dim_zero();
    bool haveAny = false;
    char pendingOp = '*';

    while (cur.text[cur.pos] != '\0') {
        FisicsDim8 atom = fisics_dim_zero();
        if (!parse_atom(&cur, &atom, outErrorDetail)) {
            return false;
        }
        if (!haveAny) {
            accum = atom;
            haveAny = true;
        } else if (pendingOp == '*') {
            if (!fisics_dim_add(accum, atom, &accum)) {
                if (outErrorDetail) *outErrorDetail = strdup("dimension addition overflow");
                return false;
            }
        } else {
            if (!fisics_dim_sub(accum, atom, &accum)) {
                if (outErrorDetail) *outErrorDetail = strdup("dimension subtraction overflow");
                return false;
            }
        }
        skip_ws(&cur);
        if (cur.text[cur.pos] == '\0') {
            break;
        }
        if (cur.text[cur.pos] != '*' && cur.text[cur.pos] != '/') {
            if (outErrorDetail) *outErrorDetail = strdup("expected '*' or '/' between dimension atoms");
            return false;
        }
        pendingOp = cur.text[cur.pos++];
        skip_ws(&cur);
    }

    *outDim = accum;
    return true;
}
