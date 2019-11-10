#ifndef __CNCT_IO_H__
#define __CNCT_IO_H__

#include "cnct.h"
#include "db_virtual.h"

// Reads a client command from fd and sends an answer to fd.
// Returns -1 if it is better to close a client cnct.
int process_cnct(struct cnct *conn, struct vdb *db);

#endif
