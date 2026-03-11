int main(void) {
    unsigned value = 0u;
    unsigned flipped = ~value;
    return flipped == ~0u ? 0 : 1;
}
