int main(void) {
    int value = ({ int inner = 3; inner + 1; });
    return value;
}
