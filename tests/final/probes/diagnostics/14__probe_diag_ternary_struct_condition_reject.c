struct Payload {
    int x;
};

int main(void) {
    struct Payload p = {1};
    return p ? 7 : 3;
}
