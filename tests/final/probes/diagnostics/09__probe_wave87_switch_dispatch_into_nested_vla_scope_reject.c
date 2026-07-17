int wave87_nested_vla_case(int n, int selector) {
    switch (selector) {
        case 0: {
            int row[n];
            case 1:
                return (int)sizeof row;
        }
        default:
            return 0;
    }
}

int main(void) {
    return wave87_nested_vla_case(3, 1);
}
