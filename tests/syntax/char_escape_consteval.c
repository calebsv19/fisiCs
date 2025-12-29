// Valid and invalid character constant folding.
int a = '\n';
int b = '\x7f';
wchar_t good = L'\u00A9';
wchar_t bad = L'\U00110000'; // out of range for wchar_t on typical targets
