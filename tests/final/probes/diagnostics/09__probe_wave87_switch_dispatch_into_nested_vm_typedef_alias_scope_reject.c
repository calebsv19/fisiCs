int wave87_nested_vm_typedef_case(int n, int selector) {
    switch (selector) {
        case 0: {
            typedef int Wave87Row[n];
            typedef Wave87Row Wave87Alias;
            case 1:
                return (int)sizeof(Wave87Alias);
        }
        default:
            return 0;
    }
}

int main(void) {
    return wave87_nested_vm_typedef_case(3, 1);
}
