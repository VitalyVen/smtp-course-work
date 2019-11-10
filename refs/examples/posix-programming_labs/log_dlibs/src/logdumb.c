#include <stdio.h>
#include <string.h>

#include "errors.h"
#include "redir.h"
#include "loglib.h"

const char *name()
{
    return "Dumb Logging Plugin";
}

int start(char *progname, char *param)
{
    if (param && strlen(param) > 0) {
        error("This plugin uses no paramaters.");
        return -1;
    }
    return 1;
}

void stop(void)
{
    printf("Dump log has stopped.\n");
}

pid_t out_pid(void)
{
    return -1;
}

pid_t err_pid(void)
{
    return -1;
}
