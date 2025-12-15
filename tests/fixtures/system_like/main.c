#include <sys/sys_stub.h>
#include <missing_sys.h>
#include "local.h"

int main(void) {
    return sys_do() + LOCAL_VAL;
}
