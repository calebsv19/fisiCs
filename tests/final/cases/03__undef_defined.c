#define X 1
#undef X
#if defined(X)
int v = 1;
#else
int v = 0;
#endif
int main(void) { return v; }
