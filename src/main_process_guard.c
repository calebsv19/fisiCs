// SPDX-License-Identifier: Apache-2.0

#include "main_process_guard.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int g_proc_guard_dir_fd = -1;
static char g_proc_guard_entry_name[64] = {0};
static pid_t g_proc_guard_group_pid = 0;
static int g_proc_guard_timeout_sec = 0;


void fisics_proc_guard_cleanup(void) {
    if (g_proc_guard_dir_fd >= 0) {
        if (g_proc_guard_entry_name[0] != '\0') {
            (void)unlinkat(g_proc_guard_dir_fd, g_proc_guard_entry_name, 0);
            g_proc_guard_entry_name[0] = '\0';
        }
        (void)close(g_proc_guard_dir_fd);
        g_proc_guard_dir_fd = -1;
    }
}

static int parse_nonnegative_int_env_with_default(const char* key, int defaultValue) {
    const char* raw = getenv(key);
    if (!raw || !raw[0]) return defaultValue;
    char* end = NULL;
    long v = strtol(raw, &end, 10);
    if (!end || *end != '\0' || v < 0 || v > 100000) {
        return defaultValue;
    }
    return (int)v;
}

static bool proc_guard_directory_is_private(int fd, uid_t owner) {
    struct stat st;
    return fstat(fd, &st) == 0 &&
           S_ISDIR(st.st_mode) &&
           st.st_uid == owner &&
           (st.st_mode & 0777) == 0700;
}

/* TMPDIR belongs to a compilation task. It must never select the process
 * accounting namespace: independent tasks still share the per-user limit.
 * The explicit override is for intentionally shared sandbox/runtime scopes. */
static int proc_guard_open_directory(uid_t owner) {
    char path[PATH_MAX];
    const char* requested = getenv("FISICS_PROC_GUARD_DIR");
    int length;
    if (requested && requested[0]) {
        if (requested[0] != '/') return -1;
        length = snprintf(path, sizeof(path), "%s", requested);
    } else {
        length = snprintf(path, sizeof(path), "/tmp/fisics_proc_guard-%lu",
                          (unsigned long)owner);
    }
    if (length < 0 || length >= (int)sizeof(path)) return -1;
    if (mkdir(path, 0700) != 0 && errno != EEXIST) return -1;

    int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return -1;
    if (!proc_guard_directory_is_private(fd, owner)) {
        close(fd);
        return -1;
    }
    return fd;
}

static bool proc_guard_regular_file_is_private(const struct stat* st,
                                               uid_t owner) {
    return st &&
           S_ISREG(st->st_mode) &&
           st->st_uid == owner &&
           (st->st_mode & (S_IRWXG | S_IRWXO)) == 0;
}

static int proc_guard_lock_directory(int dirFd, uid_t owner) {
    int fd = -1;
    /* First-use contenders may observe ENOENT during a non-exclusive create.
     * Open established state first; create absent state exclusively and retry
     * only missing-file/create races or interrupted syscalls. */
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        fd = openat(dirFd, ".lock", O_RDWR | O_NOFOLLOW | O_CLOEXEC);
        if (fd >= 0) break;
        if (errno == EINTR) continue;
        if (errno != ENOENT) return -1;

        fd = openat(dirFd, ".lock",
                    O_CREAT | O_EXCL | O_RDWR | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd >= 0) break;
        if (errno != EEXIST && errno != ENOENT && errno != EINTR) return -1;
    }
    if (fd < 0) return -1;

    struct stat st;
    if (fstat(fd, &st) != 0 ||
        !proc_guard_regular_file_is_private(&st, owner)) {
        close(fd);
        return -1;
    }

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    while (fcntl(fd, F_SETLKW, &lock) != 0) {
        if (errno == EINTR) continue;
        close(fd);
        return -1;
    }
    return fd;
}

static bool proc_guard_parse_entry_pid(const char* name, pid_t* outPid) {
    if (!name || strncmp(name, "pid-", 4) != 0) return false;
    const char* pidPart = name + 4;
    if (!pidPart[0]) return false;

    errno = 0;
    char* end = NULL;
    long parsed = strtol(pidPart, &end, 10);
    if (errno == ERANGE || !end || *end != '\0' || parsed <= 0) return false;

    pid_t pid = (pid_t)parsed;
    if ((long)pid != parsed) return false;

    char canonical[64];
    int canonicalLen = snprintf(canonical,
                                sizeof(canonical),
                                "pid-%ld",
                                parsed);
    if (canonicalLen < 0 ||
        canonicalLen >= (int)sizeof(canonical) ||
        strcmp(name, canonical) != 0) {
        return false;
    }
    if (outPid) *outPid = pid;
    return true;
}

static bool proc_guard_entry_is_private(int dirFd,
                                        const char* name,
                                        uid_t owner,
                                        pid_t* outPid) {
    pid_t pid = 0;
    if (!proc_guard_parse_entry_pid(name, &pid)) return false;

    struct stat st;
    if (fstatat(dirFd, name, &st, AT_SYMLINK_NOFOLLOW) != 0 ||
        !proc_guard_regular_file_is_private(&st, owner)) {
        return false;
    }
    if (outPid) *outPid = pid;
    return true;
}

static int proc_guard_cleanup_and_count(int dirFd,
                                        uid_t owner,
                                        pid_t currentPid) {
    int scanFd = dup(dirFd);
    if (scanFd < 0) return -1;
    DIR* dir = fdopendir(scanFd);
    if (!dir) {
        close(scanFd);
        return -1;
    }

    int count = 0;
    for (;;) {
        errno = 0;
        struct dirent* ent = readdir(dir);
        if (!ent) {
            if (errno != 0) count = -1;
            break;
        }

        pid_t pid = 0;
        if (!proc_guard_entry_is_private(dirFd, ent->d_name, owner, &pid)) {
            continue;
        }

        if (pid == currentPid) {
            (void)unlinkat(dirFd, ent->d_name, 0);
            continue;
        }

        if (kill(pid, 0) == 0) {
            if (count < INT_MAX) count++;
            continue;
        }

        int probeError = errno;
        if (probeError == ESRCH || probeError == EPERM) {
            (void)unlinkat(dirFd, ent->d_name, 0);
        } else if (count < INT_MAX) {
            count++;
        }
    }
    closedir(dir);
    return count;
}

static void fisics_timeout_handler(int signo) {
    (void)signo;
    fisics_proc_guard_cleanup();
    const char msg[] =
        "Error: fisics compile watchdog timed out; aborting hung compiler process.\n";
    (void)write(STDERR_FILENO, msg, sizeof(msg) - 1u);
    if (g_proc_guard_group_pid > 0) {
        (void)kill(-g_proc_guard_group_pid, SIGKILL);
    }
    _exit(124);
}

void fisics_proc_guard_disarm_timeout(void) {
    if (g_proc_guard_timeout_sec > 0) {
        alarm(0);
    }
}

void fisics_proc_guard_arm_timeout(void) {
    g_proc_guard_timeout_sec =
        parse_nonnegative_int_env_with_default("FISICS_TIMEOUT_SEC", 180);
    if (g_proc_guard_timeout_sec <= 0) {
        return;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fisics_timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        fprintf(stderr,
                "Warning: failed to install fisics watchdog handler; continuing without timeout.\n");
        g_proc_guard_timeout_sec = 0;
        return;
    }

    if (setpgid(0, 0) == 0 || (errno == EACCES || errno == EPERM)) {
        pid_t groupPid = getpgrp();
        if (groupPid == getpid()) {
            g_proc_guard_group_pid = groupPid;
        }
    }

    alarm((unsigned int)g_proc_guard_timeout_sec);
}

bool fisics_proc_guard_enter(void) {
    int maxProcs = parse_nonnegative_int_env_with_default("FISICS_MAX_PROCS", 1);
    if (maxProcs <= 0) return true;

    uid_t owner = geteuid();
    int dirFd = proc_guard_open_directory(owner);
    if (dirFd < 0) {
        fprintf(stderr,
                "Warning: no secure per-user process-guard directory available; continuing without process guard.\n");
        return true;
    }

    int lockFd = proc_guard_lock_directory(dirFd, owner);
    if (lockFd < 0) {
        close(dirFd);
        fprintf(stderr,
                "Warning: failed to lock secure process-guard directory; continuing without process guard.\n");
        return true;
    }

    pid_t pid = getpid();
    int running = proc_guard_cleanup_and_count(dirFd, owner, pid);
    if (running < 0) {
        close(lockFd);
        close(dirFd);
        fprintf(stderr,
                "Warning: failed to inspect secure process-guard directory; continuing without process guard.\n");
        return true;
    }
    if (running >= maxProcs) {
        fprintf(stderr,
                "Error: refusing to start fisics (FISICS_MAX_PROCS=%d, currently active=%d)\n",
                maxProcs,
                running);
        fprintf(stderr,
                "Hint: set FISICS_MAX_PROCS higher, or set it to 0 to disable this guard.\n");
        close(lockFd);
        close(dirFd);
        return false;
    }

    char entryName[sizeof(g_proc_guard_entry_name)];
    int entryNameLen = snprintf(entryName,
                                sizeof(entryName),
                                "pid-%ld",
                                (long)pid);
    if (entryNameLen < 0 || entryNameLen >= (int)sizeof(entryName)) {
        close(lockFd);
        close(dirFd);
        return true;
    }

    int fd = openat(dirFd,
                    entryName,
                    O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC,
                    0600);
    if (fd < 0) {
        // Best effort: don't hard-fail if guard file cannot be created.
        close(lockFd);
        close(dirFd);
        return true;
    }
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "%ld\n", (long)pid);
    if (len > 0) (void)write(fd, msg, (size_t)len);
    close(fd);

    g_proc_guard_dir_fd = dirFd;
    memcpy(g_proc_guard_entry_name, entryName, (size_t)entryNameLen + 1u);
    if (atexit(fisics_proc_guard_cleanup) != 0) {
        fisics_proc_guard_cleanup();
    }
    close(lockFd);
    return true;
}
