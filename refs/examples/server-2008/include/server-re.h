#ifndef _SERVER_RE_H_
#define _SERVER_RE_H_

#include "server-state.h"

/** \file server-re.h
 *  В этом файле описаны макросы регулярных выражений.
 */

#define RE_CMD_QUIT "^\\s*QUIT\\s*\n$"
#define RE_CMD_USER "^\\s*USER:\\s*(\\w+)\\s*\n$"
#define RE_CMD_PASSWORD "^\\s*PASSWORD:\\s*(\\w+)\\s*\n$"

#endif
