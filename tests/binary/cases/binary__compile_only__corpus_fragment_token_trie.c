#include <stddef.h>

typedef struct TrieNode TrieNode;
struct TrieNode {
    unsigned char ch;
    unsigned terminal;
    TrieNode *sibling;
    TrieNode *child;
};

static TrieNode *find_child(TrieNode *node, unsigned char ch) {
    TrieNode *it = node ? node->child : (TrieNode *)0;
    while (it) {
        if (it->ch == ch) return it;
        it = it->sibling;
    }
    return (TrieNode *)0;
}

size_t trie_match_prefix(TrieNode *root, const char *s) {
    size_t best = 0;
    size_t i = 0;
    TrieNode *cur = root;
    while (cur && s[i]) {
        unsigned char ch = (unsigned char)s[i];
        cur = find_child(cur, ch);
        if (!cur) break;
        ++i;
        if (cur->terminal) best = i;
    }
    return best;
}
