#ifndef FISICS_TEST_WAVE66_INCLUDE_EXTERN_STATIC_SHADOW_H
#define FISICS_TEST_WAVE66_INCLUDE_EXTERN_STATIC_SHADOW_H

extern int bucket10_wave66_shared_counter;

static int bucket10_wave66_header_shadow_step(int delta) {
    static int bucket10_wave66_shared_counter = 20;
    bucket10_wave66_shared_counter += delta;
    return bucket10_wave66_shared_counter;
}

int bucket10_wave66_include_lib_step(int delta);

#endif
