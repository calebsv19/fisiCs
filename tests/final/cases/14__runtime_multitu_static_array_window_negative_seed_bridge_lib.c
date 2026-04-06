static int buf[8] = {2, 5, 8, 13, 21, 34, 55, 89};

void prime_window(int seed) {
    int s = seed % 97;
    if (s < 0) {
        s += 97;
    }

    buf[0] = (buf[0] + s) % 257;
    buf[1] = (buf[1] * 2 + s) % 257;
    buf[2] = (buf[2] * 3 + s) % 257;
    buf[3] = (buf[3] + s * 4) % 257;
    buf[4] = (buf[4] * 5 + s) % 257;
    buf[5] = (buf[5] + s * 6) % 257;
    buf[6] = (buf[6] * 7 + s) % 257;
    buf[7] = (buf[7] + s * 8) % 257;
}

int window_sum(int start, int width) {
    int sum = 0;

    if (width > 0) {
        sum += buf[start];
    }
    if (width > 1) {
        sum += buf[start + 1] * 2;
    }
    if (width > 2) {
        sum += buf[start + 2] * 3;
    }
    if (width > 3) {
        sum += buf[start + 3] * 4;
    }

    return sum;
}

int edge_mix(void) {
    return buf[0] * 5 + buf[7] * 9 + buf[3];
}
