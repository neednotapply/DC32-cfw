#include <string.h>
#include "defs.h"
#include "solution.h"

char *savedir;
int readonly = TRUE;

void initmovelist(actlist *list)
{
	if (list)
		memset(list, 0, sizeof(*list));
}

void addtomovelist(actlist *list, action move)
{
	(void)list;
	(void)move;
}

void copymovelist(actlist *to, actlist const *from)
{
	if (to)
		memset(to, 0, sizeof(*to));
	(void)from;
}

void destroymovelist(actlist *list)
{
	if (list)
		memset(list, 0, sizeof(*list));
}

int expandsolution(solutioninfo *solution, gamesetup const *game)
{
	(void)solution;
	(void)game;
	return FALSE;
}

int contractsolution(solutioninfo const *solution, gamesetup *game)
{
	(void)solution;
	(void)game;
	return FALSE;
}

int readsolutions(gameseries *series)
{
	(void)series;
	return TRUE;
}

int savesolutions(gameseries *series)
{
	(void)series;
	return TRUE;
}

void clearsolutions(gameseries *series)
{
	(void)series;
}

int loadsolutionsetname(char const *filename, char *buffer)
{
	(void)filename;
	(void)buffer;
	return 0;
}

int createsolutionfilelist(gameseries const *series, int morethanone,
	char const ***pfilelist, int *pcount, tablespec *table)
{
	(void)series;
	(void)morethanone;
	(void)pfilelist;
	(void)pcount;
	(void)table;
	return FALSE;
}

void freesolutionfilelist(char const **filelist, tablespec *table)
{
	(void)filelist;
	(void)table;
}
