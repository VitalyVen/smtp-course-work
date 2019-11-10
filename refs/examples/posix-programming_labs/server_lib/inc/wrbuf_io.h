#ifndef __WRBUF_IO_H__
#define __WRBUF_IO_H__

#include <sys/types.h>

#include "wrbuffer.h"

ssize_t send_wrbuffer(struct wrbuffer *buf, int socket);

#endif
