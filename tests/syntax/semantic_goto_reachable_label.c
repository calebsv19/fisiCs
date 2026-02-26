int goto_cleanup_path(int x) {
    if (x > 0) {
        goto cleanup;
    }
    return 1;

cleanup:
    x += 2;
    return x;
}
