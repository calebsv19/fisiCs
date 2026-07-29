typedef unsigned int u32;

extern int edu24_artifact_request_valid(
    u32 entry_state, u32 entry_generation, u32 entry_request_id,
    u32 wanted_generation, u32 wanted_request_id, u32 cursor,
    u32 entry_semantics_valid);
extern u32 edu24_artifact_chunk_offset(u32 cursor);
extern u32 edu24_artifact_chunk_length(u32 cursor);
extern int edu24_artifact_chunk_final(u32 cursor);
extern int edu24_artifact_response_valid(
    u32 artifact_type, u32 generation, u32 cursor, u32 chunk_length,
    u32 final_marker);
extern int printf(const char *format, ...);

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u32 actual, u32 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ actual) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u32)(ACTUAL), (u32)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, 0U, 1U), 1U);
    RUN(2U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, 42U, 1U), 1U);
    RUN(3U, edu24_artifact_request_valid(
        2U, 7U, 9U, 7U, 9U, 0U, 1U), 0U);
    RUN(4U, edu24_artifact_request_valid(
        4U, 7U, 9U, 7U, 9U, 0U, 1U), 0U);
    RUN(5U, edu24_artifact_request_valid(
        3U, 0U, 9U, 0U, 9U, 0U, 1U), 0U);
    RUN(6U, edu24_artifact_request_valid(
        3U, 7U, 0U, 7U, 0U, 0U, 1U), 0U);
    RUN(7U, edu24_artifact_request_valid(
        3U, 7U, 9U, 8U, 9U, 0U, 1U), 0U);
    RUN(8U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 10U, 0U, 1U), 0U);
    RUN(9U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, 43U, 1U), 0U);
    RUN(10U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, ~0U, 1U), 0U);
    RUN(11U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, 0U, 0U), 0U);
    RUN(12U, edu24_artifact_request_valid(
        3U, 7U, 9U, 7U, 9U, 0U, 2U), 0U);

    RUN(13U, edu24_artifact_chunk_offset(0U), 0U);
    RUN(14U, edu24_artifact_chunk_offset(1U), 12U);
    RUN(15U, edu24_artifact_chunk_offset(41U), 492U);
    RUN(16U, edu24_artifact_chunk_offset(42U), 504U);
    RUN(17U, edu24_artifact_chunk_offset(43U), ~0U);
    RUN(18U, edu24_artifact_chunk_offset(~0U), ~0U);

    RUN(19U, edu24_artifact_chunk_length(0U), 12U);
    RUN(20U, edu24_artifact_chunk_length(41U), 12U);
    RUN(21U, edu24_artifact_chunk_length(42U), 8U);
    RUN(22U, edu24_artifact_chunk_length(43U), 0U);
    RUN(23U, edu24_artifact_chunk_length(~0U), 0U);

    RUN(24U, edu24_artifact_chunk_final(0U), 0U);
    RUN(25U, edu24_artifact_chunk_final(41U), 0U);
    RUN(26U, edu24_artifact_chunk_final(42U), 1U);
    RUN(27U, edu24_artifact_chunk_final(43U), 0U);
    RUN(28U, edu24_artifact_chunk_final(~0U), 0U);

    RUN(29U, edu24_artifact_response_valid(
        1U, 7U, 0U, 12U, 0U), 1U);
    RUN(30U, edu24_artifact_response_valid(
        1U, 7U, 42U, 8U, 1U), 1U);
    RUN(31U, edu24_artifact_response_valid(
        2U, 7U, 0U, 12U, 0U), 0U);
    RUN(32U, edu24_artifact_response_valid(
        1U, 0U, 0U, 12U, 0U), 0U);
    RUN(33U, edu24_artifact_response_valid(
        1U, 7U, 43U, 0U, 0U), 0U);
    RUN(34U, edu24_artifact_response_valid(
        1U, 7U, 0U, 11U, 0U), 0U);
    RUN(35U, edu24_artifact_response_valid(
        1U, 7U, 0U, 12U, 1U), 0U);
    RUN(36U, edu24_artifact_response_valid(
        1U, 7U, 42U, 12U, 1U), 0U);
    RUN(37U, edu24_artifact_response_valid(
        1U, 7U, 42U, 8U, 0U), 0U);
    RUN(38U, edu24_artifact_response_valid(
        1U, ~0U, 42U, 8U, 1U), 1U);

    printf(
        "OS-POST-EDU19 edu24-artifact snapshot=06979f3 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
