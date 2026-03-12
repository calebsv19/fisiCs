struct S {
    int a;
};

int main(void) {
    int x = (int)(struct S){1};
    return x;
}
