enum E {
    EA = -1,
    EB = 2
};

int main(void) {
    enum E e = EB;
    switch (e) {
        case EA:
            return 1;
        case EB:
            return 2;
        default:
            return 3;
    }
}
