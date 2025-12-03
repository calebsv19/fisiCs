#define FLAG 1

#if defined(FLAG)
int keep_one = 1;
#else
int drop_one = -1;
#endif

#if 0
int drop_two = -2;
#elif 1
int keep_two = 2;
#endif

int main(void) {
    return keep_one + keep_two;
}
