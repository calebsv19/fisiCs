#include "10__probe_runtime_wave66_include_extern_static_shadow.h"

int bucket10_wave66_include_lib_step(int delta) {
    bucket10_wave66_shared_counter += delta;
    return bucket10_wave66_shared_counter + bucket10_wave66_header_shadow_step(1);
}
