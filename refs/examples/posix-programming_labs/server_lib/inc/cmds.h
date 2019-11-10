#ifndef __CMDS_H__
#define __CMDS_H__

#include <sys/types.h>

#include "async_arg.h"
#include "db_virtual.h"

// Processes a protocol command.
// Return pointer to an answer or NULL in case of invalid request.
char *process_cmd(char *request, struct vdb *db,
                  struct async_arg *arg, int *to_send);

#endif
