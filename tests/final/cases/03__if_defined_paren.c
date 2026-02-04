#define FOO 1
#if defined(FOO) || defined BAR
int ok = 1;
#else
int ok = 0;
#endif
int main(void) { return ok; }
