struct Payload {
    int x;
};

int main(void) {
    struct Payload p = {1};
    switch (p) {
        case 0: return 0;
        default: return 1;
    }
}
