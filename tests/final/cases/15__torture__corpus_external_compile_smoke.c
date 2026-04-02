#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *key;
    int limit;
    unsigned weight;
} Rule;

typedef struct {
    int value;
    unsigned weight;
} Pair;

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

static Pair apply_rule(const Rule *rule, int raw_value) {
    int clamped = raw_value;
    if (clamped < 0) {
        clamped = 0;
    } else if (clamped > rule->limit) {
        clamped = rule->limit;
    }

    {
        Pair out;
        out.value = clamped;
        out.weight = rule->weight;
        return out;
    }
}

static unsigned fold_pairs(const Pair *pairs, size_t count) {
    unsigned acc = 2166136261u;
    size_t i;
    for (i = 0; i < count; ++i) {
        unsigned lane = (unsigned)(pairs[i].value + 17) * (pairs[i].weight + (unsigned)(i + 3u));
        acc ^= lane;
        acc *= 16777619u;
        acc ^= (acc >> 13);
    }
    return acc;
}

int main(void) {
    static const Rule rules[] = {
        { "threads", 64, 3u },
        { "cache_kb", 8192, 5u },
        { "retry", 16, 7u },
        { "batch", 512, 11u },
    };
    static const char *lines[] = {
        "threads=8",
        "cache_kb=256",
        "retry=3",
        "BATCH=19",
        "cache_kb=99999",
        "retry=bad",
        "threads=17",
    };

    Pair pairs[8];
    size_t pair_count = 0;
    size_t i;
    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        const char *line = lines[i];
        const char *eq = line;
        char key_buf[32];
        size_t key_len = 0;
        int parsed_value = 0;
        size_t r;

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
                pairs[pair_count++] = apply_rule(&rules[r], parsed_value);
                break;
            }
        }
    }

    {
        unsigned folded = fold_pairs(pairs, pair_count);
        printf("%u %u\n", (unsigned)pair_count, folded);
    }
    return 0;
}
