#ifndef __FD_UTILS_H__
#define __FD_UTILS_H__

#include <sys/types.h>

// Returns binded to socket_name local socket or 0 in case of any error.
int create_binded_socket(char *socket_name);

ssize_t send_all(int sockfd, const void *buf, size_t len);

ssize_t safe_send(int sockfd, const void *buf, size_t len);

ssize_t safe_recv(int sockfd, char *buf, size_t len);

ssize_t read_all(int fd, void *buf, size_t len);

ssize_t write_all(int fd, const void *buf, size_t len);

int safe_close(int fd);

int set_nonblock(int fd);

#endif
