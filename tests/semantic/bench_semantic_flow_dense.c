int classify(int x) {
entry:
    if (x < 0) {
        x = -x;
    }

    switch (x & 7) {
        case 0:
            x += 3;
            break;
        case 1:
        case 2:
            x += 5;
            break;
        case 3:
            x += 7;
            goto done;
        case 4:
            x += 11;
            break;
        default:
            x += 13;
            break;
    }

    if (x < 50) {
        x += 9;
        goto entry_tail;
    }

entry_tail:
    while (x > 10) {
        if ((x & 1) == 0) {
            x /= 2;
            continue;
        }
        x -= 3;
        if (x == 17) {
            break;
        }
    }

done:
    return x;
}

int main(void) {
    int i;
    int sum = 0;
    for (i = -16; i < 32; ++i) {
        sum += classify(i);
    }
    return sum == 0;
}
