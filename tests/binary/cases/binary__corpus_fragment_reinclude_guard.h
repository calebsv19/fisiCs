#ifndef BINARY_CORPUS_FRAGMENT_REINCLUDE_GUARD_H
#define BINARY_CORPUS_FRAGMENT_REINCLUDE_GUARD_H

typedef struct {
    unsigned id;
    unsigned depth;
} FrameMark;

#define FRAME_MARK_INIT(i, d) ((FrameMark){(i), (d)})

#endif
