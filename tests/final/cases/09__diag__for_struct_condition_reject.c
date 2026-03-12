struct State {
    int code;
};

int main(void) {
    for (; (struct State){1};) {
        return 0;
    }
    return 1;
}
