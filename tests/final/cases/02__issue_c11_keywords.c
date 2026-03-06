/*
 * Issue repro: these C11/C17 spellings are not consistently recognized as
 * proper keywords yet. Keep this case out of the active suite until behavior is
 * either implemented correctly or rejected with an intentional stable policy.
 */
_Static_assert(1, "ok");
_Thread_local int tls;
_Atomic int atom;
int x = _Alignof(int);
