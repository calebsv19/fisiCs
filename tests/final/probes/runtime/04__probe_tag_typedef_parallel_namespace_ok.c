struct AliasNamespace {
    int value;
};

typedef int AliasNamespace;

int main(void) {
    struct AliasNamespace node = {7};
    AliasNamespace number = 5;
    return (node.value - 7) + (number - 5);
}
