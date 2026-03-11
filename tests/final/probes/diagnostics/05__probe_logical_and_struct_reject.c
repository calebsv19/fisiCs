struct Payload {
    int value;
};

int main(void) {
    return ((struct Payload){1}) && 1;
}
