#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "errors.h"
#include "redir.h"
#include "logger.h"
#include "loglib.h"
#include "misc_macros.h"

const char *name()
{
    return "Syslog Logging Plugin";
}

static int log_stderr = LOG_ERR;
static int log_stdout = LOG_NOTICE;

static void process_stderr(char *message)
{
    syslog(log_stderr, "%s", message);
    fprintf(stderr, "%s",message);
}

static void process_stdout(char *message)
{
    syslog(log_stdout, "%s", message);
    fprintf(stdout, "%s", message);
}

static struct logger logger;

struct {
    int level;
    char *name;
} levels[] = {
    {LOG_DEBUG, "debug"},
    {LOG_INFO, "info"},
    {LOG_NOTICE, "notice"},
    {LOG_WARNING, "warning"},
    {LOG_ERR, "error"},
    {LOG_CRIT, "critical"},
    {LOG_ALERT, "alert"},
    {LOG_EMERG, "emergency"},
};

static int set_level(char *level_name, int *level)
{
    for (size_t i = 0; i < lengthof(levels); i++)
        if (!strcmp(level_name, levels[i].name)) {
            *level = levels[i].level;
            return 0;
        }
    error("Invalid log priority level: \"%s\"\n", level_name);
    return -1;
}


// Param is a "stdout_log_level,stderror_log_level" string;
int start(char *progname, char *param)
{
    assert(param != NULL);
    char *sep = strchr(param, ',');
    if (!sep) {
        error("Invalid plugin parameters: \"%s\"", param);
        return -1;
    }
    *sep = '\0';
    if (set_level(sep + 1, &log_stderr) || set_level(param, &log_stderr))
        return -1;
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog(progname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
    logger_init(&logger, process_stdout, process_stderr);
    int ret = logger_start(&logger);
    if (ret < 0)
        closelog();
    else
        set_sig_handlers(&logger);
    return ret ; // success
}

void stop(void)
{
    clear_sig_handlers();
    logger_stop(&logger);
}

pid_t out_pid(void)
{
    return logger.r[R_OUT].pid;
}

pid_t err_pid(void)
{
    return logger.r[R_ERR].pid;
}
