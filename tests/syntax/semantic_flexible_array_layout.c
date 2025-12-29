// Flexible array layout: size equals prefix only.
struct Flex {
    int n;
    int data[];
};

int check_size[sizeof(struct Flex) == sizeof(int) ? 1 : -1];
int check_align[_Alignof(struct Flex) == _Alignof(int) ? 1 : -1];

struct Flex f = { .n = 1 };

int main(void) {
    return f.n + check_size[0] + check_align[0];
}
