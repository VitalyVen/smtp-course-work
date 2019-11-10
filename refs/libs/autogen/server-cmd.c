#include <string.h>
#include <stdio.h>

#include "server-cmd.h"
#include "server-parse.h"

int server_cmd_user(const char *cmd, struct server_state_t *state)
{
	state->user = get_param(SERVER_FSM_EV_CMD_USER, cmd);
	fprintf(stderr, "user: [%s]\n", state->user);
	return 0;
}

int server_cmd_password(const char *cmd, struct server_state_t *state)
{
	state->password = get_param(SERVER_FSM_EV_CMD_PASSW, cmd);
	fprintf(stderr, "password is [%s]\n", state->password);
	return 0;
}

int server_authorize(struct server_state_t *state)
{
	/* Скажи "пароль"... */
	if (state->user && state->password)
		if ((!strcmp(state->user, "user")) && (!strcmp(state->password, "password"))) {
			fprintf(stderr, "user accepted: %s\n", state->user);
			return 0;
		}
	state->attempts++;
	fprintf(stderr, "invalid username or password: %s\n", state->user);
	return -1;
}
