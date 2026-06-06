#include <stdio.h>
#include <wchar.h>

int main(void) {
    const wchar_t *signed_text = L"  -123xyz";
    const wchar_t *hex_text = L"0xfftail";
    const wchar_t *float_text = L"  6.25rest";
    const wchar_t *empty_text = L"word";
    wchar_t *end_signed = 0;
    wchar_t *end_hex = 0;
    wchar_t *end_float = 0;
    wchar_t *end_empty = 0;
    long signed_value = wcstol(signed_text, &end_signed, 10);
    unsigned long hex_value = wcstoul(hex_text, &end_hex, 0);
    double float_value = wcstod(float_text, &end_float);
    long empty_value = wcstol(empty_text, &end_empty, 10);
    long off_signed = (long)(end_signed - signed_text);
    long off_hex = (long)(end_hex - hex_text);
    long off_float = (long)(end_float - float_text);
    long off_empty = (long)(end_empty - empty_text);
    long scaled = (long)(float_value * 100.0);
    long summary = signed_value + (long)hex_value + scaled + off_signed +
                   off_hex + off_float + off_empty + empty_value;

    printf("wchar-numeric signed=%ld off=%ld hex=%lu off=%ld scaled=%ld off=%ld empty=%ld off=%ld summary=%ld\n",
           signed_value,
           off_signed,
           hex_value,
           off_hex,
           scaled,
           off_float,
           empty_value,
           off_empty,
           summary);

    return signed_value == -123L && off_signed == 6L && hex_value == 255UL &&
                   off_hex == 4L && scaled == 625L && off_float == 6L &&
                   empty_value == 0L && off_empty == 0L && summary == 773L
               ? 0
               : 1;
}
