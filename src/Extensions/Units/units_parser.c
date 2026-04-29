// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_parser.h"

#include <ctype.h>
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

static FisicsDim8 alias_dim(const char* name, bool* ok) {
    *ok = true;
    if (strcmp(name, "1") == 0 || strcmp(name, "dimensionless") == 0) return fisics_dim_zero();
    if (strcmp(name, "m") == 0) return fisics_dim_length();
    if (strcmp(name, "kg") == 0) return fisics_dim_mass();
    if (strcmp(name, "s") == 0) return fisics_dim_time();
    if (strcmp(name, "A") == 0) return fisics_dim_current();
    if (strcmp(name, "K") == 0) return fisics_dim_temperature();
    if (strcmp(name, "mol") == 0) return fisics_dim_amount();
    if (strcmp(name, "cd") == 0) return fisics_dim_luminous();
    if (strcmp(name, "X") == 0) return fisics_dim_custom();
    if (strcmp(name, "velocity") == 0) return fisics_dim_velocity();
    if (strcmp(name, "acceleration") == 0) return fisics_dim_acceleration();
    if (strcmp(name, "force") == 0) return fisics_dim_force();
    if (strcmp(name, "energy") == 0) return fisics_dim_energy();
    if (strcmp(name, "power") == 0) return fisics_dim_power();
    if (strcmp(name, "pressure") == 0) return fisics_dim_pressure();
    if (strcmp(name, "charge") == 0) return fisics_dim_charge();
    if (strcmp(name, "voltage") == 0) return fisics_dim_voltage();
    if (strcmp(name, "resistance") == 0) return fisics_dim_resistance();
    *ok = false;
    return fisics_dim_zero();
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

    bool ok = false;
    FisicsDim8 base = alias_dim(ident, &ok);
    free(ident);
    if (!ok) {
        if (outErrorDetail) *outErrorDetail = strdup("unknown dimension atom");
        return false;
    }

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

bool fisics_units_attribute_extract(const ASTAttribute* attr, char** outExprText) {
    if (outExprText) *outExprText = NULL;
    if (!attr || attr->syntax != AST_ATTRIBUTE_SYNTAX_CXX || !attr->payload) {
        return false;
    }
    const char* prefix = "fisics::dim(";
    size_t prefixLen = strlen(prefix);
    if (strncmp(attr->payload, prefix, prefixLen) != 0) {
        return false;
    }
    size_t payloadLen = strlen(attr->payload);
    if (payloadLen < prefixLen + 1 || attr->payload[payloadLen - 1] != ')') {
        return false;
    }
    if (!outExprText) {
        return true;
    }
    *outExprText = dup_slice(attr->payload + prefixLen, payloadLen - prefixLen - 1);
    return *outExprText != NULL;
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
