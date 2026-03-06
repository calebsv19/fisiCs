#define OUTER 1

#if OUTER
#define INNER 1
# ifdef INNER
int nested_value = 9;
# else
int nested_value = 10;
# endif
#endif
