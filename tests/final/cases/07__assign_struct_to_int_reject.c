struct Payload {
    int value;
};

int main(void) {
    struct Payload payload = {1};
    int number = payload;
    return number;
}
