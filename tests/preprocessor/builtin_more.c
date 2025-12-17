#line 100 "foo.c"
#define GETFILE() __FILE__

int file_len = sizeof(__FILE__) - 1;
int file_len_macro = sizeof(GETFILE()) - 1;
int line_value = __LINE__; // should be 100 here
int date_len = sizeof(__DATE__) - 1; // __DATE__ is always 11 chars
int time_len = sizeof(__TIME__) - 1; // __TIME__ is always 8 chars

int main(void) {
    return file_len + file_len_macro + line_value + date_len + time_len;
}
