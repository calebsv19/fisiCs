#if ((1u << 31) > 0u) && ((0xffffffffu >> 31) == 1u)
int pp_unsigned_shift_boundary = 12;
#else
int pp_unsigned_shift_boundary = 13;
#endif
