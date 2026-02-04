#define STR(x) #x
#define CAT(a, b) a ## b

int main(void) {
    int hello_world = 3;
    const char* s = STR(hello);
    return CAT(hello, _world) + (int)s[0];
}
