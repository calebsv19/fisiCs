static int helper(void) {
    return 0;
}

extern int helper(void);

int main(void) {
    return helper();
}
