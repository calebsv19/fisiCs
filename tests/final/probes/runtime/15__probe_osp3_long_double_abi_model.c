#include <float.h>
#include <stdio.h>

int main(void) {
    printf(
        "OSP3 long-double-abi mant=%d maxexp=%d size=%u eval=%d\n",
        LDBL_MANT_DIG,
        LDBL_MAX_EXP,
        (unsigned)sizeof(long double),
        FLT_EVAL_METHOD
    );
    return 0;
}
