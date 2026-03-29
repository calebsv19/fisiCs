typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

complex_pack complex_bridge_pack(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

_Complex double complex_bridge_mix(_Complex double a, _Complex double b) {
    return a + b - (_Complex double)2.0;
}
