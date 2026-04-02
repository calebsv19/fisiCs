#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *line;
} RawLine;

typedef struct {
    const char *key;
    int ceiling;
    unsigned weight;
} Rule;

static int ascii_tolower(int ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 'a';
    }
    return ch;
}

static int key_eq_ci(const char *lhs, const char *rhs) {
    while (*lhs != '\0' && *rhs != '\0') {
        if (ascii_tolower((unsigned char)*lhs) != ascii_tolower((unsigned char)*rhs)) {
            return 0;
        }
        ++lhs;
        ++rhs;
    }
    return *lhs == '\0' && *rhs == '\0';
}

static int parse_uint10(const char *z, int *out_value) {
    int value = 0;
    if (*z == '\0') {
        return 0;
    }
    while (*z != '\0') {
        char c = *z;
        if (c < '0' || c > '9') {
            return 0;
        }
        value = value * 10 + (c - '0');
        ++z;
    }
    *out_value = value;
    return 1;
}

int main(void) {
    static const RawLine fragment[] = {
        {"[network]"},
        {"host=alpha"},
        {"port=8080"},
        {"retry=3"},
        {"[cache]"},
        {"size_kb=256"},
        {"size_kb=40960"},
        {"retry=oops"},
        {";comment"},
        {"PORT=123"},
    };
    static const Rule rules[] = {
        {"port", 65535, 3u},
        {"retry", 16, 5u},
        {"size_kb", 8192, 7u},
    };

    unsigned acc = 2166136261u;
    unsigned applied = 0u;
    size_t i;

    for (i = 0; i < sizeof(fragment) / sizeof(fragment[0]); ++i) {
        const char *line = fragment[i].line;
        const char *eq = line;
        char key_buf[32];
        size_t key_len = 0;
        int parsed_value = 0;
        size_t r;

        if (line[0] == '\0' || line[0] == '[' || line[0] == ';' || line[0] == '#') {
            continue;
        }

        while (*eq != '\0' && *eq != '=') {
            if (key_len + 1 < sizeof(key_buf)) {
                key_buf[key_len++] = *eq;
            }
            ++eq;
        }
        if (*eq != '=') {
            continue;
        }
        key_buf[key_len] = '\0';
        if (!parse_uint10(eq + 1, &parsed_value)) {
            continue;
        }

        for (r = 0; r < sizeof(rules) / sizeof(rules[0]); ++r) {
            if (key_eq_ci(key_buf, rules[r].key)) {
                int v = parsed_value;
                unsigned lane;
                if (v < 0) {
                    v = 0;
                } else if (v > rules[r].ceiling) {
                    v = rules[r].ceiling;
                }
                lane = (unsigned)(v + 11) * (rules[r].weight + (unsigned)(r * 13 + 7));
                acc ^= lane;
                acc *= 16777619u;
                acc ^= (acc >> 15);
                ++applied;
                break;
            }
        }
    }

    printf("%u %u\n", applied, acc);
    return 0;
}
