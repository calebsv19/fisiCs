struct State {
    int code;
};

int main(void) {
    do {
        return 0;
    } while ((struct State){1});
    return 1;
}
