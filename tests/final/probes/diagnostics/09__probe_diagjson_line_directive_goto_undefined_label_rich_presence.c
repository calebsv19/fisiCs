#line 8002 "virtual_stmt_goto_undefined_diagjson_probe.c"
static void probe(void) {
    goto missing_label;
}

int main(void) {
    probe();
    return 0;
}
