#ifndef _SERVER_RUN_H_
#define _SERVER_RUN_H_

/** \file server-run.h
 *  \brief Основные функции.
 *
 *   В этом файле описана главная функция программы.
 */


#include <stdio.h>
#include <string.h>

#include "checkoptn.h"
#include "server-parse.h"
#include "server-state.h"

int run(const char *filename);

#endif
