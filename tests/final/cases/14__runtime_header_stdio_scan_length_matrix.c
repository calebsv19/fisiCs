#include <stdio.h>

int main(void) {
    short small = 0;
    unsigned char byte = 0;
    unsigned hex = 0;
    long long wide = 0;
    char word[8];
    int used = -1;
    int matched = sscanf("12,255,skip,0xff,-1234567890123,token!",
                         "%hd,%hhu,%*[^,],%x,%lld,%n%7[a-z]",
                         &small,
                         &byte,
                         &hex,
                         &wide,
                         &used,
                         word);
    long long checksum = (long long)small + (long long)byte + (long long)hex + wide + used + word[0];

    printf("stdio-scan matched=%d small=%d byte=%u hex=%u wide=%lld used=%d word=%s checksum=%lld\n",
           matched,
           (int)small,
           (unsigned)byte,
           hex,
           wide,
           used,
           word,
           checksum);

    return matched == 5 && small == 12 && byte == 255 && hex == 255U &&
                   wide == -1234567890123LL && used == 32 && word[0] == 't' &&
                   checksum == -1234567889453LL
               ? 0
               : 1;
}
