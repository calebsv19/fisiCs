#include <ctype.h>
#include <locale.h>
#include <stdbool.h>

bool header_corpus_wave5_locale_alpha(unsigned char ch) {
    (void)setlocale(LC_ALL, "C");
    return isalpha(ch) && !isdigit(ch) && tolower(ch) == 'a';
}
