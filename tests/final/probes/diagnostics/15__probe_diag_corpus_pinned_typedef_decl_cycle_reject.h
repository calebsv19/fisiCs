#ifndef PROBE_CORPUS_PINNED_TYPEDEF_DECL_CYCLE_REJECT_H
#define PROBE_CORPUS_PINNED_TYPEDEF_DECL_CYCLE_REJECT_H

typedef struct CycleDecl CycleDecl;
typedef CycleDecl *CycleDeclPtr;

struct CycleDecl {
    int value;
    CycleDeclPtr next;
};

typedef struct CycleAlias {
    CycleDecl node;
    struct CycleAlias *next;
} ;

#endif
