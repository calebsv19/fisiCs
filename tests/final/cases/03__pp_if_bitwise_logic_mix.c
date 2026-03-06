#if ((3 & 1) && (4 ^ 4)) || ((8 >> 3) == 1)
int ok = 7;
#else
int bad = 0;
#endif
