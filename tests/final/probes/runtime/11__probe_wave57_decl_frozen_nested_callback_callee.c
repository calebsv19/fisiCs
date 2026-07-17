#include "11__probe_wave57_decl_frozen_nested_callback_contract.h"

Wave57DeclFrozenPayload wave57_decl_frozen_nested_callback_route(
    Wave57DeclFrozenPayload value,
    Wave57DeclFrozenChooser chooser,
    int route,
    int salt) {
    Wave57DeclFrozenPayload out = value;

    {
        struct wave57_decl_frozen_payload {
            short delta;
            unsigned char mark;
            int fold;
        };
        typedef struct wave57_decl_frozen_payload Wave57DeclFrozenPayload;
        Wave57DeclFrozenPayload local;
        Wave57DeclFrozenCallback selected;

        local.delta = (short)(route + 2);
        local.mark = 5;
        local.fold = salt * 3 + 1;

        selected = chooser(route);
        out = selected(out, salt + local.delta);
        out.x += local.fold;
        out.y += (long long)local.mark * 7;
        out.stamp += local.delta;
        out.guard += local.mark;
    }

    return out;
}
