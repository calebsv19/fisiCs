#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *line;
} ScriptLine;

static int parse_uint10(const char *z, int *out_value, int *consumed) {
    int v = 0;
    int n = 0;
    if (*z < '0' || *z > '9') {
        return 0;
    }
    while (*z >= '0' && *z <= '9') {
        v = v * 10 + (*z - '0');
        ++z;
        ++n;
    }
    *out_value = v;
    *consumed = n;
    return 1;
}

static int parse_cmd(const char *line, char *op, int *arg) {
    int value = 0;
    int n = 0;
    if (line[0] == '\0' || line[1] != ':') {
        return 0;
    }
    *op = line[0];
    if (!parse_uint10(line + 2, &value, &n)) {
        return 0;
    }
    if (line[2 + n] != '\0') {
        return 0;
    }
    *arg = value;
    return 1;
}

int main(void) {
    static const ScriptLine rows[] = {
        {"A:7"},
        {"X:12"},
        {"M:5"},
        {"R:3"},
        {"bad"},
        {"A:19"},
        {"X:27"},
        {"M:11"},
        {"R:9"},
        {"A:31"}
    };

    unsigned state = 0x7f4a7c15u;
    unsigned accepted = 0u;

    for (size_t i = 0u; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        char op = '\0';
        int arg = 0;

        if (!parse_cmd(rows[i].line, &op, &arg)) {
            continue;
        }

        if (op == 'A') {
            state += (unsigned)arg * 11u + (unsigned)i * 3u;
        } else if (op == 'X') {
            state ^= (unsigned)arg * 13u + (unsigned)i * 5u;
        } else if (op == 'M') {
            state = state * ((unsigned)arg | 1u) + 17u;
        } else if (op == 'R') {
            unsigned rot = (unsigned)arg & 15u;
            state = (state << rot) | (state >> ((16u - rot) & 15u));
        } else {
            continue;
        }

        state ^= (state >> 11u);
        ++accepted;
    }

    printf("%u %u\n", accepted, state);
    return 0;
}
