#include <stddef.h>

typedef struct {
    const char *key;
    int value;
} KVPair;

static const KVPair k_default_pairs[] = {
    { "threads", 8 },
    { "cache_kb", 256 },
};

typedef struct ParserCtx {
    const char *input;
    size_t length;
} ;

int main(void) {
    return k_default_pairs[0].value;
}
