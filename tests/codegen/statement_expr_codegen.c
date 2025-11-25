static int helper(int value) {
    return ({ int doubled = value * 2; doubled + 5; });
}

int main(void) {
    int result = helper(4);
    return result - 13;
}
