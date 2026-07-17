// Regression: the standard null pointer constant `(void *)0` converts to a
// function-pointer parameter in a call, just as integer zero does.
typedef int (*DatalabCallback)(void *);

static int datalab_submit(DatalabCallback callback) {
    return callback == (DatalabCallback)0;
}

int datalab_null_callback_argument(void) {
    return datalab_submit((void *)0);
}
