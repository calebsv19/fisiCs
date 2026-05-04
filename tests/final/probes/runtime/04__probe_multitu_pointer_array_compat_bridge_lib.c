static int wave14_plane_storage[2][3] = {
    {4, 7, 9},
    {1, 5, 11},
};

int (*wave14_ptr_plane_fetch(void))[3] {
    return wave14_plane_storage;
}
