#ifndef __inline__
int keyword_ok;
#endif

#if __has_builtin(__is_target_arch)
int drop_has_builtin;
#else
int keep_has_builtin;
#endif
