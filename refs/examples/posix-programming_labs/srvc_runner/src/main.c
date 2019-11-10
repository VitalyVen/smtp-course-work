#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errors.h"
#include "log_setup.h"
#include "pidfile.h"
#include "service.h"

int start(char *pidfile, char *log, char *servicev[])
{
    int r = -1;
    switch (open_pidfile(pidfile)) {
    case -1:
        goto exit;
    case 1:
        error("pidfile exists: %s", pidfile);
        goto exit;
    }
    if (daemonize())
        goto delete_pidfile;
    void *log_lib = setup_log(servicev[0], log);
    if (!log_lib)
        goto delete_pidfile;
    if (set_sighandler())
        goto close_log;
    if (close_pidfile())
        goto close_log;
    //
    printf("Service executed, service pid is %d\n", (int) getpid());
    char *program = servicev[0];
    if (execvp(program, servicev)) {
        error("execvp(%s): %s", program, strerror(errno));
        goto close_log;
    }
    error("execvp() returns");
    r = 0;
close_log:
    close_log(log_lib);
delete_pidfile:
    delete_pidfile(pidfile);
exit:
    return r;
}

void usage(void)
{
    printf("Usage: ./runner start <file.pid> <loglib.so>:<params> <service> <params...>\n"
           "   or: ./runner stop|status <file.pid>\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
        usage();
    char *cmd = argv[1];
    char *pidfile = argv[2];
    if (!strcmp(cmd, "start")) {
        if (argc < 5)
            usage();
        char *log = argv[3];
        if (start(pidfile, log, argv + 4) == -1)
            exit(EXIT_FAILURE);
    } else if (!strcmp(cmd, "stop")) {
        if (argc != 3)
            usage();
        if (stop_service(pidfile) == -1)
            exit(EXIT_FAILURE);
        delete_pidfile(pidfile);
    } else if (!strcmp(cmd, "status")) {
        if (argc != 3)
            usage();
        if (check_status(pidfile) == -1)
            exit(EXIT_FAILURE);
    } else {
        error("Unknown command: %s", cmd);
        usage();
    }
    exit(EXIT_SUCCESS);
}
