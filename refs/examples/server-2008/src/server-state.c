#include <stdlib.h>

#include "server-state.h"

void init_state(struct server_state_t *state)
{
	state->state = SERVER_FSM_ST_INIT;
	state->user = NULL;
	state->password = NULL;
	state->attempts = 0;
}

void free_state(struct server_state_t *state)
{
	if (state->user)
		free(state->user);
	if (state->password)
		free(state->password);
	init_state(state);
}
