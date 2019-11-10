/** \file server-parse.c
 *  \brief Функции и переменные для разбора команд.
 *
 *  В этом файле описаны функции по разбору команд и их параметров
 *  с помощью регулярных выражений.
 *  В файле существует функция инициализации, компилирующая регулярные выражения.
 *  Для всех регулярных выражений объявлены статические 
 *  глобальные переменные, содержащие скомпилированые выражения.
 */

#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "server-parse.h"
#include "server-re.h"

/** \var static regex_t re_cmd_quit
 *  \brief Скомпилированное регулярное выражение  для команды QUIT.
 */
static regex_t re_cmd_quit;

/** \var static regex_t re_cmd_user
 *  \brief Скомпилированное регулярное выражение для команды USER.
 */
static regex_t re_cmd_user;

/** \var static regex_t re_cmd_password
 *  \brief Скомпилированное регулярное выражение для команды PASSWORD.
 */
static regex_t re_cmd_password;

struct {
	te_server_fsm_event event;
	regex_t *re;
} re_events[] = {	
	{SERVER_FSM_EV_CMD_QUIT, &re_cmd_quit},
	{SERVER_FSM_EV_CMD_USER, &re_cmd_user},
	{SERVER_FSM_EV_CMD_PASSW, &re_cmd_password}
};

#define N_RE_EVENTS sizeof(re_events)/sizeof(re_events[0])

te_server_fsm_event parse_cmd(const char *cmd)
{
	int i;
	for (i = 0; i < N_RE_EVENTS;  i++)
		if (!regexec(re_events[i].re, cmd, 0, NULL, 0)) {
// 			fprintf(stderr, "re matched\n");
			return re_events[i].event;
		}
	return SERVER_FSM_EV_INVALID;
}

char *get_param(te_server_fsm_event event, const char *cmd)
{
	int i;
	regmatch_t match[2];
	for (i = 0; i < N_RE_EVENTS;  i++)
		if (re_events[i].event == event) {
			if(!regexec(re_events[i].re, cmd, 2, match, 0)) {
				int l = match[1].rm_eo - match[1].rm_so;
				printf("so = %d, eo = %d, l = %d\n", match[1].rm_so, match[1].rm_eo, l);
				char *r = malloc(sizeof(char) * (l + 1));
				if (r) {
					strncpy(r, &cmd[match[1].rm_so], l);
					r[l] = 0;
				}
				return r;
			}
			return NULL;
		}
	return NULL;
}

/** \fn void init_re()
 *  \brief Функция иниализации, компилирующая регулярные выражения.
 *  \return Ничего.
 *  На самом деле функция просто вызывается из void __attribute__((constructor)) initre(),
 *  но из-за косяк doxygen, не знающего про фукнции инициализации, приходится завести две функции. 
 */
void init_re()
{
	fprintf(stderr, "compiling re...\n");
	assert (!regcomp (&re_cmd_quit, RE_CMD_QUIT, REG_EXTENDED));
	assert (!regcomp (&re_cmd_user, RE_CMD_USER, REG_EXTENDED));
	assert (!regcomp (&re_cmd_password, RE_CMD_PASSWORD, REG_EXTENDED));
}

void free_re()
{
	regfree(&re_cmd_quit);
	regfree(&re_cmd_user);
	regfree(&re_cmd_password);
}

/* Косяк */
void __attribute__((constructor)) initre()
{
    init_re();
}

void __attribute__((destructor)) freere()
{
    free_re();
}

