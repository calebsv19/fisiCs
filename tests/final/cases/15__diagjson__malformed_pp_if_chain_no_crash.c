#if 1
#if 0
int x = 1;
#else
int y = 2;
#endif

// Intentionally dangling preprocessor conditional to stress end-of-file closure.
#if 0
static int z = 3;
