union Payload {
    int scalar;
    int lane[2];
};

int main(void) {
    int state = 0;
    switch (state) {
        case 0
            state++;
            break;
        default:
            break;
    }
    {
        union Payload source;
        union Payload copy;
        source.lane[0] = 5;
        source.lane[1] = 8;
        copy = source;
        return copy.lane[0] + copy.lane[1];
    }
}
