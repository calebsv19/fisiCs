union Payload {
    int scalar;
    int lane[2];
};

int main(void) {
    int i;
    for (i = 0 i < 1; ++i) {
        i += 2;
    }
    {
        union Payload p;
        p.lane[0] = 5;
        p.lane[1] = 8;
        switch (p.lane[0] - 5) {
            case 0:
                return p.lane[1];
            default:
                return p.scalar;
        }
    }
}
