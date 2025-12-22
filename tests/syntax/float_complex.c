// Float/long double/_Complex parsing and conversions

_Complex double zd;
_Complex float zf;
long double ld = 1.0L;
double mix = (double)ld + 2.0;
float promote = 1.0f + 2.0f;
_Complex double lit_imag = 1.0 + 2.0i;
_Imaginary double just_imag = 3.0j;

int main(void) {
    _Complex double local = zd + (_Complex double)mix;
    _Complex double combo = lit_imag + just_imag;
    (void)local;
    (void)combo;
    return 0;
}
