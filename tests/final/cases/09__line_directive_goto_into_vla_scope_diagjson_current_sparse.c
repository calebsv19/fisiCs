#line 6402 "virtual_stmt_goto_vla_current.c"
int bucket09_goto_vla(int n) {
    goto after_vla;
    int vla[n];
after_vla:
    return 0;
}

int main(void) {
    return bucket09_goto_vla(3);
}
