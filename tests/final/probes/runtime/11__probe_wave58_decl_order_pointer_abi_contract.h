#ifndef FISICS_PROBE_WAVE58_DECL_ORDER_POINTER_ABI_CONTRACT_H
#define FISICS_PROBE_WAVE58_DECL_ORDER_POINTER_ABI_CONTRACT_H

typedef struct wave58_decl_order_payload {
    long long lane;
    long long total;
    int stamp;
    int guard;
} Wave58DeclOrderPayload;

Wave58DeclOrderPayload *wave58_decl_order_pointer_shadow(
    Wave58DeclOrderPayload *out,
    const Wave58DeclOrderPayload *input,
    int bias);

Wave58DeclOrderPayload *wave58_decl_order_pointer_alpha(
    Wave58DeclOrderPayload *out,
    const Wave58DeclOrderPayload *input,
    int bias);

#endif
