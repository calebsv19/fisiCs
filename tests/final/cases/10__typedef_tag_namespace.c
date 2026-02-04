typedef int S;
struct S { int v; };
int main(void) {
    S a = 1;
    struct S b = {2};
    return a + b.v;
}
