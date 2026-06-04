#include <stdio.h>
#include <string.h>

int main(void) {
    const char *text = "alpha:beta-gamma";
    const char *colon = strchr(text, ':');
    const char *gamma = strstr(text, "gamma");
    size_t alpha_span = strcspn(text, ":");
    size_t beta_span = strspn(text + 6, "abcdefghijklmnopqrstuvwxyz");
    long colon_pos = colon ? (long)(colon - text) : -1L;
    long gamma_pos = gamma ? (long)(gamma - text) : -1L;

    printf("string-search colon=%ld gamma=%ld spans=%lu/%lu\n",
           colon_pos,
           gamma_pos,
           (unsigned long)alpha_span,
           (unsigned long)beta_span);
    return colon_pos == 5 && gamma_pos == 11 && alpha_span == 5 && beta_span == 4 ? 0 : 1;
}
