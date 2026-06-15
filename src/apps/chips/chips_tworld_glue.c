#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "defs.h"
#include "err.h"
#include "oshw.h"
#include "res.h"

char *resdir;
char const *_err_cfile;
unsigned long _err_lineno;
static int mTworldTick;

static void chipsTworldClearErrorSite(void)
{
	_err_cfile = NULL;
	_err_lineno = 0;
}

void chipsTworldResetTimer(void)
{
	mTworldTick = 0;
}

void chipsTworldAdvanceTick(void)
{
	if (mTworldTick < MAXIMUM_TICK_COUNT)
		mTworldTick++;
}

time_t chipsTworldTime(time_t *out)
{
	time_t value = 12345;

	if (out)
		*out = value;
	return value;
}

int oshwinitialize(int silence, int soundbufsize, int showhistogram, int fullscreen)
{
	(void)silence;
	(void)soundbufsize;
	(void)showhistogram;
	(void)fullscreen;
	return TRUE;
}

void settimer(int action)
{
	if (action < 0)
		mTworldTick = 0;
}

void settimersecond(int ms)
{
	(void)ms;
}

int gettickcount(void)
{
	return mTworldTick;
}

int waitfortick(void)
{
	chipsTworldAdvanceTick();
	return TRUE;
}

int advancetick(void)
{
	chipsTworldAdvanceTick();
	return TRUE;
}

int setkeyboardrepeat(int enable)
{
	(void)enable;
	return TRUE;
}

int setkeyboardarrowsrepeat(int enable)
{
	(void)enable;
	return TRUE;
}

int setkeyboardinputmode(int enable)
{
	(void)enable;
	return TRUE;
}

int input(int wait)
{
	(void)wait;
	return CmdNone;
}

int anykey(void)
{
	return TRUE;
}

tablespec const *keyboardhelp(int context)
{
	(void)context;
	return NULL;
}

int loadfontfromfile(char const *filename, int complain)
{
	(void)filename;
	(void)complain;
	return TRUE;
}

void freefont(void)
{
}

int loadtileset(char const *filename, int complain)
{
	(void)filename;
	(void)complain;
	return TRUE;
}

void freetileset(void)
{
}

int creategamedisplay(void)
{
	return TRUE;
}

void setcolors(long bkgnd, long text, long bold, long dim)
{
	(void)bkgnd;
	(void)text;
	(void)bold;
	(void)dim;
}

void cleardisplay(void)
{
}

int displaygame(void const *state, int timeleft, int besttime)
{
	(void)state;
	(void)timeleft;
	(void)besttime;
	return TRUE;
}

int displayendmessage(int basescore, int timescore, long totalscore, int completed)
{
	(void)basescore;
	(void)timescore;
	(void)totalscore;
	(void)completed;
	return TRUE;
}

int setdisplaymsg(char const *msg, int msecs, int bold)
{
	(void)msg;
	(void)msecs;
	(void)bold;
	return TRUE;
}

int displaylist(char const *title, void const *table, int *index, int (*inputcallback)(int*))
{
	(void)title;
	(void)table;
	(void)index;
	(void)inputcallback;
	return FALSE;
}

int displayinputprompt(char const *prompt, char *inputbuf, int maxlen, int (*inputcallback)(void))
{
	(void)prompt;
	(void)inputbuf;
	(void)maxlen;
	(void)inputcallback;
	return FALSE;
}

int setaudiosystem(int active)
{
	(void)active;
	return FALSE;
}

int loadsfxfromfile(int index, char const *filename)
{
	(void)index;
	(void)filename;
	return TRUE;
}

void playsoundeffects(unsigned long sfx)
{
	(void)sfx;
}

void setsoundeffects(int action)
{
	(void)action;
}

int setvolume(int volume, int display)
{
	(void)volume;
	(void)display;
	return TRUE;
}

int changevolume(int delta, int display)
{
	(void)delta;
	(void)display;
	return TRUE;
}

void freesfx(int index)
{
	(void)index;
}

void ding(void)
{
}

void setsubtitle(char const *subtitle)
{
	(void)subtitle;
}

void usermessage(int action, char const *prefix, char const *cfile,
	unsigned long lineno, char const *fmt, va_list args)
{
	(void)action;
	(void)prefix;
	(void)cfile;
	(void)lineno;
	(void)fmt;
	(void)args;
}

void _warn(char const *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	usermessage(0, "warning", _err_cfile, _err_lineno, fmt, args);
	va_end(args);
	chipsTworldClearErrorSite();
}

void _errmsg(char const *prefix, char const *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	usermessage(0, prefix, _err_cfile, _err_lineno, fmt, args);
	va_end(args);
	chipsTworldClearErrorSite();
}

void _die(char const *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	usermessage(0, "error", _err_cfile, _err_lineno, fmt, args);
	va_end(args);
	chipsTworldClearErrorSite();
}

int displaytiletable(char const *title, tiletablerow const *rows, int count, int completed)
{
	(void)title;
	(void)rows;
	(void)count;
	(void)completed;
	return TRUE;
}

int displaytable(char const *title, tablespec const *table, int completed)
{
	(void)title;
	(void)table;
	(void)completed;
	return TRUE;
}

int initresources(void)
{
	return TRUE;
}

int loadgameresources(int ruleset)
{
	(void)ruleset;
	return TRUE;
}

void freeallresources(void)
{
}
