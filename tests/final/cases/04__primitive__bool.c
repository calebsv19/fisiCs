_Bool gb0 = 0;
_Bool gb1 = 7;

int bool_mix(_Bool in) {
    _Bool local = in;
    return (int)gb0 + (int)gb1 + (int)local;
}
