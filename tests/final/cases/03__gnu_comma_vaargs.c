#define PICK(fmt, ...) (fmt , ##__VA_ARGS__)

int a = PICK(1);
int b = PICK(1, 2);
