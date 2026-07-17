static int wave41_apply(int (*callback)(int));

static int wave41_apply(callback)
int callback(int);
{
    return callback(41);
}

static int wave41_increment_callback(int value) {
    return value + 1;
}

int main(void) {
    return wave41_apply(wave41_increment_callback) == 42 ? 0 : 1;
}
