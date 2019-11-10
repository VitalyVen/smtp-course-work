#include <assert.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "log_setup.h"

void do_some_output()
{
    printf("stdout logger pid: %d\n", out_pid());
    printf("stderr logger pid: %d\n", err_pid());
    fprintf(stderr, "This is a stderr message.\n");
    if (kill(1, SIGKILL))
        perror("just some perror & stderr test");
    if (err_pid() > 0 && err_pid() != out_pid()) {
        printf("Killing stderr logger (pid: %d)...\n", err_pid());
        sleep(1);
        kill(err_pid(), SIGKILL);
        sleep(1);
        fprintf(stderr, "This string causes SIGPIPE.\n");
        printf("New stderr logger pid: %d\n", err_pid());
        fprintf(stderr, "Hopefully, stderr has been restored.\n");
        printf("Killing stdout logger (pid: %d)...\n", out_pid());
        sleep(1);
        kill(out_pid(), SIGKILL);
        sleep(1);
        fprintf(stdout, "This string causes SIGPIPE.\n");
        printf("New stdout logger pid: %d\n", out_pid());
    } else {
        printf("Pids aren't looking good, skipping..\n");
    }
}

void usage(void)
{
    printf("Usage: ./sample <path to log library>:<log library parameters>\n");
    printf("Example: ./sample log_file.so:/tmp/mylog\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();
    void *log_lib = setup_log("logging sample", argv[1]);
    if (!log_lib)
        exit(EXIT_FAILURE);
    do_some_output();
    close_log(log_lib);
    exit(EXIT_SUCCESS);
}
