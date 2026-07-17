#line 7601 "virtual_stmt_switch_continue_not_loop_probe.c"
int main(void) {
    int selector = 1;

    switch (selector) {
        case 0:
            return 0;
        case 1:
            continue;
        default:
            break;
    }

    return selector;
}
