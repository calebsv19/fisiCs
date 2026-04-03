#ifndef BINARY_CORPUS_FRAGMENT_FEATURE_TOGGLE_H
#define BINARY_CORPUS_FRAGMENT_FEATURE_TOGGLE_H

#ifndef BINARY_CFG_FASTPATH
#define BINARY_CFG_FASTPATH 0
#endif

#ifndef BINARY_CFG_STRICT
#define BINARY_CFG_STRICT 0
#endif

#if BINARY_CFG_FASTPATH
#define INLINE_HINT static
#else
#define INLINE_HINT
#endif

INLINE_HINT unsigned fold_mode(unsigned seed) {
#if BINARY_CFG_STRICT
    return (seed * 3u) ^ 0x5au;
#else
    return (seed * 2u) + 7u;
#endif
}

#endif
