// Wide/UTF literal escape handling and range checks

#include <wchar.h>

wchar_t good[] = L"A\u0042\x43";
wchar_t overflow_wide[] = L"\U00110000";
char narrow_overflow[] = "\u0100";
char embedded_nul[] = "a\0b";
wchar_t wide_char_ok = L'\u0041';
wchar_t wide_char_bad = L'\U00110000';

int main(void) {
    return good[0] == L'A';
}
