#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "service.h"
#include "errors.h"

pid_t daemonize(void)
{
    if (getppid() == 1) {
        error("init is already our parent");
        return -1;
    }
    if (signal(SIGTTOU, SIG_IGN))
        error("signal(SIGTTOU, SIG_IGN)");
    if (signal(SIGTTIN, SIG_IGN))
        error("signal(SIGTTIN, SIG_IGN)");
    if (signal(SIGTSTP, SIG_IGN))
        error("signal(SIGTSTP, SIG_IGN)");
    pid_t pid = fork();
    if (pid == -1) {
        errnomsg("fork()");
        return -1;
    }
    if (pid > 0)
        exit(0); // parent does not return
    umask(0);
    if (setsid() == -1) {
        errnomsg("setssid()");
        return -1;
    }
    if (chdir("/"))
        errnomsg("chdir(\"/\")");
    return 0;
}

pid_t sighandler_pid = -1;

void set_sighandler_pid(pid_t pid)
{
    sighandler_pid = pid;
    //printf("sighandler_pid set to %d\n", (int) sighandler_pid);
}

void sig_handler(int signal)
{
    printf("Signal caught: %d\n", signal);
    if (sighandler_pid > 0) {
        printf("Signal %d was sent to process %d\n", signal, (int) sighandler_pid);
        kill(sighandler_pid, signal);
    }
}

int set_sighandler()
{
    struct sigaction sa = {
        .sa_flags = 0,
        .sa_handler = sig_handler
    };
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction()");
        return -1;
    }
    return 0;
}

pid_t fork_service(char *argv[])
{
    pid_t pid = fork();
    if (pid == -1) {
        errnomsg("fork()");
        return -1;
    }
    if (pid == 0) {
        char *program = argv[0];
        if (execvp(program, argv))
            error("execvp(%s): %s", program, strerror(errno));
        exit(101);
    }
    return pid;
}

int wait_service(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) == -1)
        CHECK_EINTR(continue, return -1, "waitpid(%d)", (int) pid);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 101)
        return -1;
    return 0;
}

pid_t stop_service(const char *pidfile)
{
    pid_t r = -1;
    FILE *f = fopen(pidfile, "r");
    if (!f) {
        error("fopen(\"%s\"): %s)", pidfile, strerror(errno));
        goto exit;
    }
    pid_t pid;
    if (fscanf(f, "%d", &pid) != 1) {
        error("Cannot read pid from \"%s\"", pidfile);
        goto close;
    }
    if (kill(pid, 0)) {
        warning("Service is not running (pid = %d)", pid);
        r = 0;
    }
    else if (kill(pid, SIGTERM)) {
        error("kill(%d, SIGTERM)", pid);
        goto close;
    } else
        r = pid;
close:
    fclose(f);
exit:
    return r;
}
