/*===================================================================*/
/*                                                                   */
/*  InfoNES_Mapper.cpp : InfoNES Mapper Function                     */
/*                                                                   */
/*  2000/05/16  InfoNES Project ( based on NesterJ and pNesX )       */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_Mapper.h"
#include "K6502.h"
#include <pico.h>

/*-------------------------------------------------------------------*/
/*  Mapper resources                                                 */
/*-------------------------------------------------------------------*/

BYTE *DRAM;   // [DRAM_SIZE];
BYTE *Map85_Chr_Ram;
BYTE *Map5_Wram;
BYTE *Map5_Ex_Ram;
BYTE *Map5_Ex_Vram;
BYTE *Map5_Ex_Nam;
BYTE Map5_Gfx_Mode;
BYTE Map5_Chr_Upper;

/*-------------------------------------------------------------------*/
/*  Table of Mapper initialize function                              */
/*-------------------------------------------------------------------*/

const struct MapperTable_tag MapperTable[] =
    {
        {0, Map0_Init},
        {1, Map1_Init},
        {2, Map2_Init},
        {3, Map3_Init},
        {4, Map4_Init},
        {7, Map7_Init},
        {10, Map10_Init},
        {-1, NULL}};

/*-------------------------------------------------------------------*/
/*  body of Mapper functions                                         */
/*-------------------------------------------------------------------*/

#include "mapper/InfoNES_Mapper_000.cpp"
#include "mapper/InfoNES_Mapper_001.cpp"
#include "mapper/InfoNES_Mapper_002.cpp"
#include "mapper/InfoNES_Mapper_003.cpp"
#include "mapper/InfoNES_Mapper_004.cpp"
#include "mapper/InfoNES_Mapper_007.cpp"
#include "mapper/InfoNES_Mapper_010.cpp"
#if 0
#include "mapper/InfoNES_Mapper_008.cpp"
#include "mapper/InfoNES_Mapper_009.cpp"
#include "mapper/InfoNES_Mapper_011.cpp"
#include "mapper/InfoNES_Mapper_013.cpp"
#include "mapper/InfoNES_Mapper_015.cpp"
#include "mapper/InfoNES_Mapper_016.cpp"
#include "mapper/InfoNES_Mapper_017.cpp"
#include "mapper/InfoNES_Mapper_018.cpp"
#include "mapper/InfoNES_Mapper_019.cpp"
#if PICO_RP2350
#include "mapper/InfoNES_Mapper_020.cpp"
#endif
#include "mapper/InfoNES_Mapper_021.cpp"
#include "mapper/InfoNES_Mapper_022.cpp"
#include "mapper/InfoNES_Mapper_023.cpp"
#include "mapper/InfoNES_Mapper_024.cpp"
#include "mapper/InfoNES_Mapper_025.cpp"
#include "mapper/InfoNES_Mapper_026.cpp"
#include "mapper/InfoNES_Mapper_030.cpp"
#include "mapper/InfoNES_Mapper_032.cpp"
#include "mapper/InfoNES_Mapper_033.cpp"
#include "mapper/InfoNES_Mapper_034.cpp"
#include "mapper/InfoNES_Mapper_040.cpp"
#include "mapper/InfoNES_Mapper_041.cpp"
#include "mapper/InfoNES_Mapper_042.cpp"
#include "mapper/InfoNES_Mapper_043.cpp"
#include "mapper/InfoNES_Mapper_044.cpp"
#include "mapper/InfoNES_Mapper_045.cpp"
#include "mapper/InfoNES_Mapper_046.cpp"
#include "mapper/InfoNES_Mapper_047.cpp"
#include "mapper/InfoNES_Mapper_048.cpp"
#include "mapper/InfoNES_Mapper_049.cpp"
#include "mapper/InfoNES_Mapper_050.cpp"
#include "mapper/InfoNES_Mapper_051.cpp"
#include "mapper/InfoNES_Mapper_057.cpp"
#include "mapper/InfoNES_Mapper_058.cpp"
#include "mapper/InfoNES_Mapper_060.cpp"
#include "mapper/InfoNES_Mapper_061.cpp"
#include "mapper/InfoNES_Mapper_062.cpp"
#include "mapper/InfoNES_Mapper_064.cpp"
#include "mapper/InfoNES_Mapper_065.cpp"
#include "mapper/InfoNES_Mapper_066.cpp"
#include "mapper/InfoNES_Mapper_067.cpp"
#include "mapper/InfoNES_Mapper_068.cpp"
#include "mapper/InfoNES_Mapper_069.cpp"
#include "mapper/InfoNES_Mapper_070.cpp"
#include "mapper/InfoNES_Mapper_071.cpp"
#include "mapper/InfoNES_Mapper_072.cpp"
#include "mapper/InfoNES_Mapper_073.cpp"
#include "mapper/InfoNES_Mapper_074.cpp"
#include "mapper/InfoNES_Mapper_075.cpp"
#include "mapper/InfoNES_Mapper_076.cpp"
#include "mapper/InfoNES_Mapper_077.cpp"
#include "mapper/InfoNES_Mapper_078.cpp"
#include "mapper/InfoNES_Mapper_079.cpp"
#include "mapper/InfoNES_Mapper_080.cpp"
#include "mapper/InfoNES_Mapper_082.cpp"
#include "mapper/InfoNES_Mapper_083.cpp"
#include "mapper/InfoNES_Mapper_085.cpp"
#include "mapper/InfoNES_Mapper_086.cpp"
#include "mapper/InfoNES_Mapper_087.cpp"
#include "mapper/InfoNES_Mapper_088.cpp"
#include "mapper/InfoNES_Mapper_089.cpp"
#include "mapper/InfoNES_Mapper_090.cpp"
#include "mapper/InfoNES_Mapper_091.cpp"
#include "mapper/InfoNES_Mapper_092.cpp"
#include "mapper/InfoNES_Mapper_093.cpp"
#include "mapper/InfoNES_Mapper_094.cpp"
#include "mapper/InfoNES_Mapper_095.cpp"
#include "mapper/InfoNES_Mapper_096.cpp"
#include "mapper/InfoNES_Mapper_097.cpp"
#include "mapper/InfoNES_Mapper_100.cpp"
#include "mapper/InfoNES_Mapper_101.cpp"
#include "mapper/InfoNES_Mapper_105.cpp"
#include "mapper/InfoNES_Mapper_107.cpp"
#include "mapper/InfoNES_Mapper_108.cpp"
#include "mapper/InfoNES_Mapper_109.cpp"
#include "mapper/InfoNES_Mapper_110.cpp"
#include "mapper/InfoNES_Mapper_112.cpp"
#include "mapper/InfoNES_Mapper_113.cpp"
#include "mapper/InfoNES_Mapper_114.cpp"
#include "mapper/InfoNES_Mapper_115.cpp"
#include "mapper/InfoNES_Mapper_116.cpp"
#include "mapper/InfoNES_Mapper_117.cpp"
#include "mapper/InfoNES_Mapper_118.cpp"
#include "mapper/InfoNES_Mapper_119.cpp"
#include "mapper/InfoNES_Mapper_122.cpp"
#include "mapper/InfoNES_Mapper_133.cpp"
#include "mapper/InfoNES_Mapper_134.cpp"
#include "mapper/InfoNES_Mapper_135.cpp"
#include "mapper/InfoNES_Mapper_140.cpp"
#include "mapper/InfoNES_Mapper_151.cpp"
#include "mapper/InfoNES_Mapper_160.cpp"
#include "mapper/InfoNES_Mapper_180.cpp"
#include "mapper/InfoNES_Mapper_181.cpp"
#include "mapper/InfoNES_Mapper_182.cpp"
#include "mapper/InfoNES_Mapper_183.cpp"
#include "mapper/InfoNES_Mapper_184.cpp"
#include "mapper/InfoNES_Mapper_185.cpp"
#include "mapper/InfoNES_Mapper_187.cpp"
#include "mapper/InfoNES_Mapper_188.cpp"
#include "mapper/InfoNES_Mapper_189.cpp"
#include "mapper/InfoNES_Mapper_191.cpp"
#include "mapper/InfoNES_Mapper_193.cpp"
#include "mapper/InfoNES_Mapper_194.cpp"
#include "mapper/InfoNES_Mapper_200.cpp"
#include "mapper/InfoNES_Mapper_201.cpp"
#include "mapper/InfoNES_Mapper_202.cpp"
#include "mapper/InfoNES_Mapper_206.cpp"
#include "mapper/InfoNES_Mapper_222.cpp"
#include "mapper/InfoNES_Mapper_225.cpp"
#include "mapper/InfoNES_Mapper_226.cpp"
#include "mapper/InfoNES_Mapper_227.cpp"
#include "mapper/InfoNES_Mapper_228.cpp"
#include "mapper/InfoNES_Mapper_229.cpp"
#include "mapper/InfoNES_Mapper_230.cpp"
#include "mapper/InfoNES_Mapper_231.cpp"
#include "mapper/InfoNES_Mapper_232.cpp"
#include "mapper/InfoNES_Mapper_233.cpp"
#include "mapper/InfoNES_Mapper_234.cpp"
#include "mapper/InfoNES_Mapper_235.cpp"
#include "mapper/InfoNES_Mapper_236.cpp"
#include "mapper/InfoNES_Mapper_240.cpp"
#include "mapper/InfoNES_Mapper_241.cpp"
#include "mapper/InfoNES_Mapper_242.cpp"
#include "mapper/InfoNES_Mapper_243.cpp"
#include "mapper/InfoNES_Mapper_244.cpp"
#include "mapper/InfoNES_Mapper_245.cpp"
#include "mapper/InfoNES_Mapper_246.cpp"
#include "mapper/InfoNES_Mapper_248.cpp"
#include "mapper/InfoNES_Mapper_249.cpp"
#include "mapper/InfoNES_Mapper_251.cpp"
#include "mapper/InfoNES_Mapper_252.cpp"
#include "mapper/InfoNES_Mapper_255.cpp"
#endif

/* End of InfoNES_Mapper.cpp */
