#define HDR_CHECK(x) defined(x)

#if !HDR_CHECK(HDR_FLAG)
#error "HDR_FLAG should be visible at include check entry"
#endif

#define HDR_SEEN_BEFORE_UNDEF 1
#undef HDR_FLAG

#if HDR_CHECK(HDR_FLAG)
#error "HDR_FLAG should be undefined after include convergence"
#endif

#define HDR_SEEN_AFTER_UNDEF 2
#undef HDR_CHECK
