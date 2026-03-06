#if 1 || (1 / 0)
int short_or = 1;
#else
int short_or = 2;
#endif

#if 0 && (1 / 0)
int short_and = 3;
#else
int short_and = 4;
#endif
