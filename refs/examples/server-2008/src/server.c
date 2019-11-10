/** \file server.c
 *  \brief Основные функции.
 *
 *   В этом файле описана главная функция программы.
 */


#include <stdio.h>
#include <string.h>

#include "checkoptn.h"
#include "server-run.h"


/** \fn int main(int argc, char *argv[])
 *  \brief Главная функция программы.
 *  \param argc -- число аргументов при вызове программы;
 *  \param argv -- аргументы вызова программы.
 *  \return код завершения программы.
 */
int main(int argc, char *argv[])
{
	int optct = optionProcess(&test_serverOptions, argc, argv);
	
	argc -= optct;
	argv += optct;

 	return run(OPT_ARG(FILE));
}

