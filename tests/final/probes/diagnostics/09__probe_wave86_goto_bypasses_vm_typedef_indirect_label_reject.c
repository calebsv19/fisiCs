int wave86_vm_typedef_indirect_label(int n) {
    goto wave86_indirect_done;
    typedef int Wave86IndirectRow[n];
    if (n) wave86_indirect_done: return 0;
    return 1;
}

int main(void) {
    return wave86_vm_typedef_indirect_label(4);
}
