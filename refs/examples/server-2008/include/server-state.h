#ifndef _SERVER_STATE_H_
#define _SERVER_STATE_H_

/** \file server-state.h
 *  В этом файле описана структура с состоянием и фунции по её инициализации и освобождению.
 */

#include "server-fsm.h"

/** 
 *  \brief Структура, хранящее дополниетельную информацию о состоянии.
 */
struct server_state_t {
	te_server_fsm_state state;
	char *user, *password;
	int attempts;
};

void init_state(struct server_state_t *state);

void free_state(struct server_state_t *state);


#endif
