#if (1 ? 5 : (1 / 0))
int pp_ternary_true = 1;
#else
int pp_ternary_true = 2;
#endif

#if (0 ? (1 / 0) : 6)
int pp_ternary_false = 3;
#else
int pp_ternary_false = 4;
#endif
