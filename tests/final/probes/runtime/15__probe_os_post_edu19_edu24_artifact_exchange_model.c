/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-24-bounded-result-artifact-exchange, commit
 * 06979f374cb823fe9911d57c97e130053c313b5d.
 *
 * EDU-24's authoritative artifact serialization and sector reads are
 * assembly-owned. This hardware-blind model freezes the observable request,
 * identity, cursor, chunk, and final-marker policy for a 512-byte result.
 */
typedef unsigned int edu24_u32;

enum {
    EDU24_STATE_COMPLETE = 3,
    EDU24_RESULT_BYTES = 512,
    EDU24_CHUNK_BYTES = 12,
    EDU24_CHUNK_COUNT = 43,
    EDU24_ARTIFACT_TYPE_RESULT = 1
};

int edu24_artifact_request_valid(
    edu24_u32 entry_state,
    edu24_u32 entry_generation,
    edu24_u32 entry_request_id,
    edu24_u32 wanted_generation,
    edu24_u32 wanted_request_id,
    edu24_u32 cursor,
    edu24_u32 entry_semantics_valid) {
    return entry_semantics_valid == 1U &&
           entry_state == EDU24_STATE_COMPLETE &&
           entry_generation != 0U &&
           entry_request_id != 0U &&
           wanted_generation == entry_generation &&
           wanted_request_id == entry_request_id &&
           cursor < EDU24_CHUNK_COUNT;
}

edu24_u32 edu24_artifact_chunk_offset(edu24_u32 cursor) {
    if (cursor >= EDU24_CHUNK_COUNT) return ~0U;
    return cursor * EDU24_CHUNK_BYTES;
}

edu24_u32 edu24_artifact_chunk_length(edu24_u32 cursor) {
    if (cursor >= EDU24_CHUNK_COUNT) return 0U;
    if (cursor + 1U == EDU24_CHUNK_COUNT) {
        return EDU24_RESULT_BYTES -
               (EDU24_CHUNK_COUNT - 1U) * EDU24_CHUNK_BYTES;
    }
    return EDU24_CHUNK_BYTES;
}

int edu24_artifact_chunk_final(edu24_u32 cursor) {
    return cursor == EDU24_CHUNK_COUNT - 1U;
}

int edu24_artifact_response_valid(
    edu24_u32 artifact_type,
    edu24_u32 generation,
    edu24_u32 cursor,
    edu24_u32 chunk_length,
    edu24_u32 final_marker) {
    return artifact_type == EDU24_ARTIFACT_TYPE_RESULT &&
           generation != 0U &&
           cursor < EDU24_CHUNK_COUNT &&
           chunk_length == edu24_artifact_chunk_length(cursor) &&
           final_marker == (edu24_u32)edu24_artifact_chunk_final(cursor);
}
