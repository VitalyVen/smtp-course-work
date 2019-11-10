#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errors.h"
#include "pidfile.h"
#include "service.h"

void usage(void)
{
    // TODO: [<user>]:[<group>]
    printf("Usage: ./sample start <file.pid> <service> <params...>\n"
           "   or: ./sample stop|status <file.pid>\n");
    exit(1);
}

int start(char *pidfile, char *servicev[])
{
    int r = 1;
    switch (open_pidfile(pidfile)) {
    case -1:
        goto exit;
    case 1:
        error("pidfile exists: %s", pidfile);
        goto exit;
    }
    if (daemonize())
        goto delete_pidfile;
    if (set_sighandler())
        goto delete_pidfile;
    if (close_pidfile())
        goto delete_pidfile;
    pid_t service_pid = fork_service(servicev);
    if (service_pid == -1)
        goto delete_pidfile;
    set_sighandler_pid(service_pid);
    if (wait_service(service_pid))
        goto delete_pidfile;
    r = 0;
delete_pidfile:
    delete_pidfile(pidfile);
exit:
    return r;
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
        return start(pidfile, argv + 3);
    } else if (!strcmp(cmd, "stop")) {
        if (argc != 3)
            usage();
        stop_service(pidfile);
    } else if (!strcmp(cmd, "status")) {
        if (argc != 3)
            usage();
        check_status(pidfile);
    } else {
        error("Unknown command: %s", cmd);
        return 1;
    }
    return 0;
}
