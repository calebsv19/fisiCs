struct TokenNamespace {
    int value;
};

int TokenNamespace = 9;

int main(void) {
    struct TokenNamespace node = {4};
    return (TokenNamespace - 9) + (node.value - 4);
}
