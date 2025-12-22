// Wide character and string literals

wchar_t wc = L'a';
wchar_t* ws = L"hi";
const wchar_t arr[] = L"ok";

int main(void) {
    return ws[0] != L'h';
}
