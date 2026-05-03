#ifndef PROBE_CORPUS_PINNED_NESTED_DECL_TAIL_REPLAY_REJECT_B_H
#define PROBE_CORPUS_PINNED_NESTED_DECL_TAIL_REPLAY_REJECT_B_H

typedef struct ReplayNode ReplayNode;
typedef ReplayNode *ReplayNodePtr;

struct ReplayNode {
    int lane;
    ReplayNodePtr next;
};

typedef struct ReplayWrap {
    ReplayNode head;
    struct ReplayWrap *next;
    int tag;
} ;

#endif
