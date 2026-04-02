#ifndef CORPUS_EXTERNAL_GUARD_MISMATCH_EXPECTED_H
#define CORPUS_EXTERNAL_GUARD_MISMATCH_ACTUAL_H

static int header_seed(void) {
    return 17;
}

#include "15__torture__corpus_external_include_guard_mismatch_reject.h"

#endif
