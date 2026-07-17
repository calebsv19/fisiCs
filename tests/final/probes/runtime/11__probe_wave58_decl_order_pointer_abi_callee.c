#include "11__probe_wave58_decl_order_pointer_abi_contract.h"

Wave58DeclOrderPayload *wave58_decl_order_pointer_shadow(
    Wave58DeclOrderPayload *out,
    const Wave58DeclOrderPayload *input,
    int bias) {
    {
        struct wave58_decl_order_payload {
            short delta;
            unsigned char mark;
            int fold;
        } local;

        local.delta = (short)(bias + 2);
        local.mark = 4;
        local.fold = bias * 3 + 1;

        out->lane = input->total + local.delta;
        out->total = input->lane * local.mark + local.fold;
        out->stamp = input->stamp + local.delta + local.mark;
        out->guard = input->guard + local.fold - local.mark;
    }

    return out;
}

Wave58DeclOrderPayload *wave58_decl_order_pointer_alpha(
    Wave58DeclOrderPayload *out,
    const Wave58DeclOrderPayload *input,
    int bias) {
    {
        struct wave58_decl_order_local_payload {
            short delta;
            unsigned char mark;
            int fold;
        } local;

        local.delta = (short)(bias + 2);
        local.mark = 4;
        local.fold = bias * 3 + 1;

        out->lane = input->total + local.delta;
        out->total = input->lane * local.mark + local.fold;
        out->stamp = input->stamp + local.delta + local.mark;
        out->guard = input->guard + local.fold - local.mark;
    }

    return out;
}
