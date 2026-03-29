struct Payload {
    int x;
};

int f(void) {
    struct Payload p = {1};
    return p;
}

int main(void) {
    return f();
}
