int wave86_vm_typedef_same_block(int n) {
    goto wave86_same_block_done;
    typedef int Wave86Row[n];
wave86_same_block_done:
    return 0;
}

int main(void) {
    return wave86_vm_typedef_same_block(3);
}
