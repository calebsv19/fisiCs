#line 8062 "virtual_stmt_label_redefinition_diag_probe.c"
static int probe(void) {
label:
    ;
label:
    return 0;
}

int main(void) {
    return probe();
}
