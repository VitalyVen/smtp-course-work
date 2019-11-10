#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "errors.h"
#include "fd_utils.h"
#include "pidfile.h"

FILE *fpidfile;

int open_pidfile(const char *pidfile)
{
    int fd = open(pidfile, O_WRONLY | O_CREAT | O_EXCL, 0660);
    if (fd == -1) {
        if (errno == EEXIST)
            return 1;
        errnomsg("open(\"%s\")", pidfile);
        return -1;
    }
    if (!(fpidfile = fdopen(fd, "w"))) {
        errnomsg("fdopen(%d, \"w\")", fd);
        safe_close(fd);
        return -1;
    }
    return 0;
}

int close_pidfile()
{
    int r = -1;
    if (fprintf(fpidfile, "%d\n", getpid()) < 0) {
        errnomsg("fprintf(pidfile, ...)");
        goto exit;
    }
    if (fclose(fpidfile)) {
        errnomsg("fclose(pidfile)");
        goto exit;
    }
    r = 0;
exit:
    fpidfile = NULL;
    return r;
}

int delete_pidfile(const char *pidfile)
{
    if (fpidfile) {
        fclose(fpidfile);
        fpidfile = NULL;
    }
    int r =  unlink(pidfile);
    if (r) {
        errnomsg("unlink(\"%s\")", pidfile);
        return -1;
    }
    return 0;
}

int check_status(const char *pidfile)
{
    int r = -1;
    FILE *f = fopen(pidfile, "r");
    if (!f) {
        if (errno == ENOENT)
            warning("Pid file \"%s\" not found.", pidfile);
        else
            errnomsg("fopen(\"%s\")", pidfile);
        goto exit;
    }
    int pid;
    if (fscanf(f, "%d", &pid) != 1) {
        warning("Cannot read pid from \"%s\"", pidfile);
        printf("Service is just started probably\n");
        goto close;
    }
    if (!kill(pid, 0)) {
        printf("Service is still running (pid = %d)\n", pid);
        r = 0;
    } else
        printf("Service is terminated incorrectly (pid = %d)\n", pid);
close:
    fclose(f);
exit:
    return r;
}
