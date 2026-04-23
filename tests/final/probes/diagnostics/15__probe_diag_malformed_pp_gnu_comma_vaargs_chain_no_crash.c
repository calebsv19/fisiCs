#define L0(x) x
#define L1(fmt, ...) L0(fmt , ##__VA_ARGS__)
#define L2(fmt, ...) L1(fmt , ##__VA_ARGS__)

int main(void) {
    return L2(1);
}
