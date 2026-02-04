int x = 1;
int main(void) {
    int x = 2;
    { int x = 3; }
    return x;
}
