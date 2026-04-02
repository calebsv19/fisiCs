#define PICK(fmt, ...) (fmt , ##__VA_ARGS__)

int main(void) {
    return PICK(1);
}
