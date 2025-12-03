#define NAME(x) x
#define STR(x) #x
#define CONCAT(a, b) a##b

int CONCAT(my, func)(void) {
    return 3;
}

int main(void) {
    // Forces stringification and token pasting to run before parsing.
    return CONCAT(my, func)() + sizeof(STR(NAME(token)));
}
