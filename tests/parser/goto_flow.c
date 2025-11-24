int main(void) {
    int result = 0;

start:
    result += 1;
    if (result == 1) {
        goto mid;
    }

mid:
    result += 2;
    if (result < 5) {
        goto end;
    }

    result += 4;
    goto exit;

end:
    result += 8;
    goto exit;

exit:
    return result;
}
