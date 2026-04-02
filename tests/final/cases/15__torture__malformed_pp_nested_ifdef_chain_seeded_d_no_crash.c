#if defined(OUTER_A)
#if defined(INNER_A)
int inner_a(void) { return 1; }
#elif defined(INNER_B)
int inner_b(void) { return 2; }
#else
int inner_fallback(void) { return 3; }
#endif
#elif defined(OUTER_B)
int outer_b(void) { return 4; }
