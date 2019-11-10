#ifndef _SERVER_PARSE_H_
#define _SERVER_PARSE_H_

/** \file server-parse.h
 *  \brief Функции разбора команд.
 *
 *   В этом файле описаны функции, разбирающие команды клиента.
 */

#include "server-state.h"

/** 
 * \brief Возвращает событие, связанное с полученной командой. 
 * \param cmd -- команда.
 * \return SERVER_FSM_EV_INVALID или событие, связаное с командой. 
 */
te_server_fsm_event parse_cmd(const char *cmd);

/**
 * \brief Создаёт и возвращает строку с новым аргументом. Её затем следует освободить.	
 * \param event -- событие;
 * \param cmd -- команда.
 * \return Параметр команды или NULL.
 */
char *get_param(te_server_fsm_event event, const char *cmd);

#endif
