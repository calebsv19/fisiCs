#line 8022 "virtual_stmt_goto_cross_init_diagjson_probe.c"
static void probe(void) {
    goto after_init;
    int value = 1;
after_init:
    (void)value;
}

int main(void) {
    probe();
    return 0;
}
