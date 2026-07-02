/*===================================================================*/
/*                                                                   */
/*  InfoNES_System.h : The function which depends on a system        */
/*                                                                   */
/*  2000/05/29  InfoNES Project ( based on pNesX )                   */
/*                                                                   */
/*===================================================================*/

#ifndef InfoNES_SYSTEM_H_INCLUDED
#define InfoNES_SYSTEM_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include "InfoNES_Types.h"

/*-------------------------------------------------------------------*/
/*  Palette data                                                     */
/*-------------------------------------------------------------------*/
extern const WORD NesPalette[];

/*-------------------------------------------------------------------*/
/*  Function prototypes                                              */
/*-------------------------------------------------------------------*/

/* Menu screen */
int InfoNES_Menu();

/* Read ROM image file */
int InfoNES_ReadRom(const char *pszFileName);

/* Release a memory for ROM */
void InfoNES_ReleaseRom();

/* Transfer the contents of work frame on the screen */
int InfoNES_LoadFrame();

/* Pace every emulated frame independently from the rendered frame rate. */
void InfoNES_FrameBoundary();

#ifdef NES_PERF_PROFILE
enum InfoNES_PerfSection {
    INFONES_PERF_CPU,
    INFONES_PERF_PPU,
};
void InfoNES_PerfStart(enum InfoNES_PerfSection section);
void InfoNES_PerfStop(enum InfoNES_PerfSection section);
#else
#define InfoNES_PerfStart(section) ((void)0)
#define InfoNES_PerfStop(section) ((void)0)
#endif

/* Get a joypad state */
void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem);

/* memcpy */
inline void *InfoNES_MemoryCopy(void *dest, const void *src, int count)
{
    __builtin_memcpy(dest, src, count);
    return dest;
}

/* memset */
inline void *InfoNES_MemorySet(void *dest, int c, int count)
{
    __builtin_memset(dest, c, count);
    return dest;
}

/* Print debug message */
void InfoNES_DebugPrint(const char *pszMsg);

/* Wait */
inline void InfoNES_Wait() {}

/* Print system message */
void InfoNES_MessageBox(const char *pszMsg, ...);

void InfoNES_Error(const char *pszMsg, ...);
void InfoNES_PreDrawLine(int line);
void InfoNES_PostDrawLine(int line);

#endif /* !InfoNES_SYSTEM_H_INCLUDED */
