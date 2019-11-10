/** \file server-run.c
 *  \brief Основные функции.
 *
 *   В этом файле описана главная функция программы.
 */


#include <stdio.h>
#include <string.h>

#include "checkoptn.h"
#include "server-parse.h"
#include "server-state.h"
#include "server-cmd.h"

static int read_cmd(FILE *file, te_server_fsm_event *event, char *cmd) 
{	
	if (!strchr(fgets(cmd, MAXCMDLEN, file), '\n'))
		return 1;
	*event = parse_cmd(cmd);
	return 0;
}

int run(const char *filename)
{
	int result = 0;
	te_server_fsm_event event, gen_ev;
	te_server_fsm_state new_state;
	struct server_state_t state;

	char cmd[MAXCMDLEN];
	FILE *file = fopen(filename, "r");
	if (!file) {
		perror(filename);	
		return -1;
	}

	init_state(&state);
	while (!feof(file)) {
		if (read_cmd(file, &event, cmd)) {
			result = 1;
			goto error;
		}
	
		fprintf(stderr, "event = %d, cmd = %s\n", event, cmd);
		
		new_state = server_fsm_step(state.state, event, cmd, &state);
		if (new_state == SERVER_FSM_ST_INVALID) 
			continue;
		state.state = new_state;
		/*
		 * Желание явно указать на схеме состояние
		 * непринятия пароля привело к извращениям.
		 * Недостаток (?) autofsm.
		 */
		switch (state.state) {
		case SERVER_FSM_ST_AUTHORIZATION:
			switch (server_authorize(&state)) {
			case 0: gen_ev = SERVER_FSM_EV_PASSWORD_ACCEPTED; break;
			case -2: gen_ev = SERVER_FSM_EV_TOO_MANY_ATTEMPTS; break;
			default: 
				gen_ev = SERVER_FSM_EV_PASSWORD_INVALID;
			}
			state.state = server_fsm_step(state.state, gen_ev, NULL, &state);
			break;
		default: break;
		}
		if (state.state == SERVER_FSM_ST_DONE)
			break;
	}
error:
	free_state(&state);
	fclose(file);
	return result;
}
