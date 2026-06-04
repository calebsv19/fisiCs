#include <float.h>
#include <stdio.h>

int main(void) {
    int radix_ok = FLT_RADIX >= 2;
    int mant_order = FLT_MANT_DIG <= DBL_MANT_DIG && DBL_MANT_DIG <= LDBL_MANT_DIG;
    int eps_order = FLT_EPSILON >= DBL_EPSILON && DBL_EPSILON >= (double)LDBL_EPSILON;
    int dec_order = FLT_DIG <= DBL_DIG && DBL_DIG <= DECIMAL_DIG;
    int eval_ok = FLT_EVAL_METHOD >= -1;
    int score = radix_ok + mant_order + eps_order + dec_order + eval_ok;

    printf("float-precision score=%d mant=%d/%d/%d dig=%d/%d/%d eval=%d\n",
           score,
           FLT_MANT_DIG,
           DBL_MANT_DIG,
           LDBL_MANT_DIG,
           FLT_DIG,
           DBL_DIG,
           DECIMAL_DIG,
           FLT_EVAL_METHOD);
    return score == 5 ? 0 : 1;
}
