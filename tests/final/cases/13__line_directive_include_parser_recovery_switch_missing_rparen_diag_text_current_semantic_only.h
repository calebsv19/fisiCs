#line 132351 "virtual_ir_include_parser_recovery_switch_missing_rparen_diag_text_current_semantic_only.h"
int main(void) {
    int s = 0;
    switch (s {
        case 0:
            s = 1;
            break;
        default:
            s = 2;
            break;
    }
    return s;
}
