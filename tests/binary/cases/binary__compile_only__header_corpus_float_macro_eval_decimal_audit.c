#include <float.h>

int main(void) {
    int eval_method = FLT_EVAL_METHOD;
    int decimal_digits = DECIMAL_DIG;

    return (eval_method >= -1 && decimal_digits > 0) ? 0 : 1;
}
