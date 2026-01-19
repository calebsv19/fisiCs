# Failing or Suspicious Behavior (make run, include/test.txt)

- `sizeof` on a dereferenced pointer-to-VLA yields pointer size instead of runtime VLA size (e.g., `int n; int a[n]; int (*p)[n] = &a; sizeof *p`).
