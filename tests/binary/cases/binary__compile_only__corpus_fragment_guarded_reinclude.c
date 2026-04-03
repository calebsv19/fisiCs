#include "binary__corpus_fragment_reinclude_guard.h"
#include "binary__corpus_fragment_reinclude_guard.h"
#include "binary__corpus_fragment_reinclude_guard.h"

unsigned guarded_reinclude_score(unsigned base) {
    FrameMark a = FRAME_MARK_INIT(base, 1u);
    FrameMark b = FRAME_MARK_INIT(base + 1u, 2u);
    return a.id + a.depth + b.id + b.depth;
}
