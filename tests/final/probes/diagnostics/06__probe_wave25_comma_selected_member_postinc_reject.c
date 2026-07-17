struct Wave25CommaCell {
    int value;
};

static int wave25_touch(int *stamp) {
    return ++*stamp;
}

int probe_wave25_comma_selected_member_postinc(int pick) {
    struct Wave25CommaCell cells[2] = {{3}, {7}};
    int stamp = 0;
    struct Wave25CommaCell *selected = pick ? &cells[0] : &cells[1];
#line 12301 "virtual_lv_wave25_comma_selected_member_postinc.c"
    (wave25_touch(&stamp), selected->value)++;
    return selected->value + stamp;
}
