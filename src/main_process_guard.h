// SPDX-License-Identifier: Apache-2.0
#ifndef FISICS_MAIN_PROCESS_GUARD_H
#define FISICS_MAIN_PROCESS_GUARD_H

#include <stdbool.h>

/* CLI-only process admission, descriptor cleanup and watchdog lifecycle. */
bool fisics_proc_guard_enter(void);
void fisics_proc_guard_cleanup(void);
void fisics_proc_guard_arm_timeout(void);
void fisics_proc_guard_disarm_timeout(void);

#endif
