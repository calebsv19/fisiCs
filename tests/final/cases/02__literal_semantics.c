typedef char assert_hex[(0x1FULL == 31ULL) ? 1 : -1];
typedef char assert_oct[(017ULL == 15ULL) ? 1 : -1];
typedef char assert_hex_big[(0x7fffffffffffffffULL == 9223372036854775807ULL) ? 1 : -1];
typedef char assert_multi[('ab' == 0x6162) ? 1 : -1];
typedef char assert_splice[(1\
23 == 123) ? 1 : -1];
typedef char assert_u8[(sizeof(u8"hi") == 3) ? 1 : -1];
typedef char assert_L[(sizeof(L"hi") == 3 * sizeof(wchar_t)) ? 1 : -1];
typedef char assert_u[(sizeof(u"hi") == 3 * sizeof(wchar_t)) ? 1 : -1];
typedef char assert_U[(sizeof(U"hi") == 3 * sizeof(wchar_t)) ? 1 : -1];

int main(void) { return 0; }
