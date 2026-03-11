struct Payload {
    int value;
};

int main(void) {
    if ((struct Payload){1}) {
        return 0;
    }
    return 1;
}
