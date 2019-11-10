#ifndef __TELNET_IO_H__
#define __TELNET_IO_H__

#include <sys/types.h>

#include "buffer.h"

ssize_t recv_until(int sockfd, struct buffer *buf, char *sep);

ssize_t send_wsuffix(int sockfd, char *line, char *suffix);

int strip_telnet_cmd(char *cmd);

#endif
