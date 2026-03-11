int g = 5;

int main(void) {
    extern int g;
    int x = g;
    {
        int g = 2;
        x += g;
    }
    return x;
}
