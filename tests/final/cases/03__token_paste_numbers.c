#define CAT(a,b) a ## b
int main(void) {
    int x = CAT(12,34);
    return x;
}
