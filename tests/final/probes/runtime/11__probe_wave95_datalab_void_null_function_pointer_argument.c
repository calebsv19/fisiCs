typedef int (*DatalabCallback)(void *);

static int datalab_submit(DatalabCallback callback) {
    return callback == (DatalabCallback)0;
}

int main(void) {
    return datalab_submit((void *)0) ? 0 : 1;
}
