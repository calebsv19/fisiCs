struct Inner { int x; int y; };
struct Outer { int a; struct Inner b; int c; };

struct Outer o = { 1, { 2, 3 }, 4 };
struct Outer p = { 5, 6, 7, 8 };
