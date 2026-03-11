int align_char = _Alignof(char);
int align_int = _Alignof(int);
int align_double = _Alignof(double);

int main(void) {
    return align_char + align_int + align_double;
}
