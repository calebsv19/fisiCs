int id(int x) {
    return x;
}

int apply(int fn(int)) {
    return fn(2);
}

int main(void) {
    return apply(id) - 2;
}
