char c_plain = 0;
signed char c_signed = -1;
unsigned char c_unsigned = 255;

int char_signedness_probe(void) {
    char local = (char)-1;
    return local < 0;
}
