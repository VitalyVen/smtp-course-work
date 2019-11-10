#ifndef _SERVER_CMD_H_
#define _SERVER_CMD_H_

/** \file server-cmd.h
 *  \brief Функции обработки команд и изменения состояния сервера.
 *
 *   В этом файле описано состояние сервера, а также вызываемые
 *   из конечного автомата функции обработки команд.
 */

#include "server-state.h"

#define MAXCMDLEN 1024


/** \fn int server_cmd_user(const char *cmd, struct server_state_t *state)
 *  \brief Функция, обрабатывающая команду USER.
 *  \param cmd -- текст команды;
 *  \param state -- состояние сервера.
 *  \return 0 -- в случае успеха, или код ошибки.
 */
int server_cmd_user(const char *cmd, struct server_state_t *state);

/** \fn int server_cmd_password(const char *cmd, struct server_state_t *state)
 *  \brief Функция, обрабатывающая команду PASSWORD.
 *  \param cmd -- текст команды;
 *  \param state -- состояние сервера.
 *  \return 0 -- в случае успеха, или код ошибки.
 */
int server_cmd_password(const char *cmd, struct server_state_t *state);


/** \fn int server_cmd_user(struct server_state_t *state)
 *  \brief Функция, авторизирующая пользователя.
 *  \param state -- состояние сервера.
 *  \return 0 -- в случае успеха, или код ошибки.
 */
int server_authorize(struct server_state_t *state);

#endif
