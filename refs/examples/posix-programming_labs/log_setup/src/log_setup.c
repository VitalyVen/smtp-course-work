#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

pid_t (*out_pid)(void);
pid_t (*err_pid)(void);

void *setup_log(char *progname, char *log)
{
    // Pointers to logging library functions;
    const char *(*loglib_name)(void);
    int (*loglib_start)(char *progname, char *param);
    char *sep = strchr(log, ':');
    char *param = NULL;
    if (sep) {
        *sep = '\0';
        param = sep + 1;
    }
    void *log_lib = dlopen(log, RTLD_LAZY);
    if (!log_lib) {
        error("dlopen(%s): %s", log, dlerror());
        return NULL;
    }
    if (!(loglib_name = dlsym(log_lib, "name")))
        goto dl_error;
    if (!(out_pid =  dlsym(log_lib, "out_pid")))
        goto dl_error;
    if (!(err_pid =  dlsym(log_lib, "err_pid")))
        goto dl_error;
    if (!(loglib_start = dlsym(log_lib, "start")))
        goto dl_error;
    if (param)
        printf("Using the %s with \"%s\" parameter.\n", loglib_name(), param);
    else
        printf("Using the %s with no parameter.\n", loglib_name());
    int ret = loglib_start(progname, param);
    if (ret == 0)
        exit(0); // Child
    if (ret < 0) {
        error("There is a failure when starting plug-in.");
        goto dl_close;
    }
    return log_lib;
dl_error: // goto is a poor man 'catch'
    error("%s", dlerror());
dl_close: // 'goto' is a poor man 'finally'
    dlclose(log_lib);
    return NULL;
}

void close_log(void *log_lib)
{
    void (*loglib_stop)(void) = dlsym(log_lib, "stop");
    if (!loglib_stop)
        perror(dlerror());
    loglib_stop();
    if (dlclose(log_lib))
        perror(dlerror());
}
