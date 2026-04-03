#define BINARY_CFG_FASTPATH 1
#define BINARY_CFG_STRICT 1
#include "binary__corpus_fragment_feature_toggle.h"

unsigned macro_include_toggle_score(unsigned x) {
    return fold_mode(x) + fold_mode(x + 1u);
}
