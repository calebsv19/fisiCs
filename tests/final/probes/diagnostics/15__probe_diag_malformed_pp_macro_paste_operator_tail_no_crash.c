#define BAD_JOIN(a) a ## +

int main(void) {
    return BAD_JOIN(1);
}
