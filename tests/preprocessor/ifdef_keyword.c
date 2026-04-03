#ifndef __inline__
int keyword_ok;
#endif

#if __has_builtin(__is_target_arch)
int drop_has_builtin;
#else
int keep_has_builtin;
#endif

#if !defined(__has_extension) || !__has_extension(define_target_os_macros)
int keep_has_extension;
#else
int drop_has_extension;
#endif
