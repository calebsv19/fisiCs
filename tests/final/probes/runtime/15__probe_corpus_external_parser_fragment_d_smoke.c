#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *expr;
} ExprLine;

static int parse_uint10(const char *z, int *out_value, int *consumed) {
    int value = 0;
    int n = 0;
    if (*z < '0' || *z > '9') {
        return 0;
    }
    while (*z >= '0' && *z <= '9') {
        value = value * 10 + (*z - '0');
        ++z;
        ++n;
    }
    *out_value = value;
    *consumed = n;
    return 1;
}

static int eval_expr(const char *line, int *out) {
    int lhs = 0;
    int rhs = 0;
    int n = 0;
    char op;

    if (!parse_uint10(line, &lhs, &n)) {
        return 0;
    }
    line += n;
    op = *line;
    if (op == '\0') {
        return 0;
    }
    ++line;
    if (!parse_uint10(line, &rhs, &n)) {
        return 0;
    }
    line += n;
    if (*line != '\0') {
        return 0;
    }

    if (op == '+') {
        *out = lhs + rhs;
        return 1;
    }
    if (op == '-') {
        *out = lhs - rhs;
        return 1;
    }
    if (op == '*') {
        *out = lhs * rhs;
        return 1;
    }
    if (op == '^') {
        *out = lhs ^ rhs;
        return 1;
    }
    if (op == '&') {
        *out = lhs & rhs;
        return 1;
    }
    if (op == '|') {
        *out = lhs | rhs;
        return 1;
    }
    return 0;
}

int main(void) {
    static const ExprLine fragment[] = {
        {"7+9"},
        {"12^3"},
        {"8*11"},
        {"31-4"},
        {"bad"},
        {"90&17"},
        {"25|6"},
    };

    unsigned acc = 0x9e3779b9u;
    unsigned accepted = 0u;
    size_t i;

    for (i = 0; i < sizeof(fragment) / sizeof(fragment[0]); ++i) {
        int v = 0;
        unsigned lane;

        if (!eval_expr(fragment[i].expr, &v)) {
            continue;
        }
        lane = (unsigned)(v + (int)(i * 19 + 11));
        acc ^= lane + accepted * 97u;
        acc = (acc << 7) | (acc >> 25);
        acc ^= (acc >> 11);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
