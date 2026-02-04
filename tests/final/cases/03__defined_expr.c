#define YES 1
#if defined(YES) && !defined(NO)
int ok = 1;
#else
int ok = 2;
#endif

int main(void) { return ok; }
