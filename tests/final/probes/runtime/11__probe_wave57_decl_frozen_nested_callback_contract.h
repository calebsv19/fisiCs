#ifndef FISICS_PROBE_WAVE57_DECL_FROZEN_NESTED_CALLBACK_CONTRACT_H
#define FISICS_PROBE_WAVE57_DECL_FROZEN_NESTED_CALLBACK_CONTRACT_H

typedef struct wave57_decl_frozen_payload {
    long long x;
    long long y;
    int stamp;
    int guard;
} Wave57DeclFrozenPayload;

typedef Wave57DeclFrozenPayload (*Wave57DeclFrozenCallback)(
    Wave57DeclFrozenPayload value,
    int salt);

typedef Wave57DeclFrozenCallback (*Wave57DeclFrozenChooser)(int route);

Wave57DeclFrozenPayload wave57_decl_frozen_nested_callback_route(
    Wave57DeclFrozenPayload value,
    Wave57DeclFrozenChooser chooser,
    int route,
    int salt);

#endif
