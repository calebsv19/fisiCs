#include <stddef.h>

typedef struct {
    float beats;
    const char* label;
} BeatLabel;

static int check_table(void) {
    static const BeatLabel kNotes[] = {
        {1.0f, "1/1"},
        {1.0f / 3.0f, "1/2T"},
        {1.0f / 6.0f, "1/4T"}
    };
    return (kNotes[0].beats > kNotes[1].beats) && (kNotes[1].beats > kNotes[2].beats);
}

int main(void) {
    return check_table() ? 0 : 1;
}
