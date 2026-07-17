int wave86_vm_typedef_alias_descendant(int n) {
    typedef int Wave86Row[n];
    typedef Wave86Row Wave86Alias;
    goto wave86_descendant_done;
    {
        Wave86Alias wave86_values;
wave86_descendant_done:
        return 0;
    }
}

int main(void) {
    return wave86_vm_typedef_alias_descendant(4);
}
