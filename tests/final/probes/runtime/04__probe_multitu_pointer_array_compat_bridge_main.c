extern int printf(const char*, ...);
extern int (*wave14_ptr_plane_fetch(void))[3];

int main(void) {
    int (*wave14_ptr_plane)[3] = wave14_ptr_plane_fetch();
    printf("%d %d %d\n",
           wave14_ptr_plane[0][0],
           wave14_ptr_plane[0][1],
           wave14_ptr_plane[1][2]);
    return 0;
}
