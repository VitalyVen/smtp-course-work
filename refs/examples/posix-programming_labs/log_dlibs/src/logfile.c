#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "errors.h"
#include "logger.h"
#include "loglib.h"
#include "redir.h"

static void process_stderr(char *message);
static void process_stdout(char *message);

const char *name()
{
    return "File Logging Plugin (two logging processes)";
}

static pid_t parent_pid;
static FILE *log_file;
static struct logger logger;

int start(char *progname, char *log_filename)
{
    if (log_file) {
        error("Log has already started.");
        return -1;
    }
    parent_pid = getpid();
    while (!(log_file = fopen(log_filename, "a")))
        CHECK_EINTR(, return -1, 
                    "Cannot open log file \"%s\"", log_filename);
    logger_init(&logger, process_stdout, process_stderr);
    int ret = logger_start(&logger);
    if (ret <= 0) {
        fclose(log_file);
        log_file = NULL;
    } else
        set_sig_handlers(&logger);
    return ret;
}

void stop(void)
{
    clear_sig_handlers();
    logger_stop(&logger);
    while (fclose(log_file))
        CHECK_EINTR(, break, "fclose(log)");
    log_file = NULL;
}

static void process_stderr(char *message)
{
    fprintf(log_file, "[STDERR][%d] %s", parent_pid, message);
    fflush(log_file);
    fprintf(stderr, "%s", message);
}

static void process_stdout(char *message)
{
    fprintf(log_file, "[STDOUT][%d] %s", parent_pid, message);
    fflush(log_file);
    fprintf(stdout, "%s", message);
}

pid_t out_pid(void)
{
    return logger.r[R_OUT].pid;
}

pid_t err_pid(void)
{
    return logger.r[R_ERR].pid;
}
