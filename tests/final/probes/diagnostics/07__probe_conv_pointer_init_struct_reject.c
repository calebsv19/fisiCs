struct Payload {
    int value;
};

int main(void) {
    struct Payload payload = {7};
    int *p = payload;
    return p != 0;
}
