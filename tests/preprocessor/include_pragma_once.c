#include "pragma_once_header.h"
#include "pragma_once_header.h"

#ifdef INCLUDED_TWICE
int duplicated_declaration(void);
#endif

int main() {
    return INCLUDED_ONCE;
}
