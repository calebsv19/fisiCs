typedef const unsigned long ulong_t;
static volatile int counter;
extern int shared;
restrict int *rp;
ulong_t value;

int use_register(void) {
    register int reg = 1;
    return reg;
}
