struct S {
    int x;
};

int main(void) {
    int v = (int)(struct S){1};
    return v;
}
