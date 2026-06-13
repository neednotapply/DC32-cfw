#include "am_map.h"
#include "d_main.h"
#include "doomstat.h"
#include "f_finale.h"

cheatseq_t cheat_amap = CHEAT("iddt", 0);
boolean automapactive;

boolean AM_Responder(event_t *ev)
{
    (void) ev;
    return false;
}

void AM_Ticker(void)
{
}

void AM_Drawer(void)
{
}

void AM_Stop(void)
{
    automapactive = false;
}

finalestage_t finalestage;
const char *finaleflat;

boolean F_Responder(event_t *ev)
{
    (void) ev;
    return false;
}

void F_Ticker(void)
{
    gameaction = ga_worlddone;
}

void F_Drawer(void)
{
}

void F_StartFinale(void)
{
    gameaction = ga_worlddone;
}

const char *F_ArtScreenLumpName(void)
{
    return NULL;
}

#if DOOM_TINY
int F_BunnyScrollPos(void)
{
    return 0;
}

void F_BunnyDrawPatches(void)
{
}
#endif

int F_CastSprite(void)
{
    return 0;
}

void F_CastDrawer(void)
{
}
