/* $Header: /fridge/cvs/xscorch/sgame/slscapetools.h,v 1.4 2009-04-26 17:39:41 jacob Exp $ */
/*--------------------------------------------------------------------
 *
 *  slscapetools.h
 *  Copyright (C) 2000 Matti Hänninen
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to 
 *
 *    The Free Software Foundation, Inc.
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 * 
 */

#ifndef SLSCAPETOOLS_H
#define SLSCAPETOOLS_H

#include <xscorch.h>

/* #define SC_TOOLS_DEBUG */

/*--------------------------------------------------------------------
 *  F u n c t i o n   d e c l a r a t i o n s
 */

/* Module initialization */
void sc_tools_init(void);

/* Continuous noise functions */
double sc_tools_pnoise(long int offset, double x);
double sc_tools_pnoise_periodic(long int offset, long int period, double x);
double sc_tools_rnoise(long int offset, double x);
double sc_tools_rnoise_periodic(long int offset, long int period, double x);

/* Very handy functions */
double sc_tools_clamp(double a, double b, double x);
double sc_tools_mod(double a, double x);
double sc_tools_step(double a, double x);
double sc_tools_smoothstep(double a, double b, double x);

#endif /* SLSCAPETOOLS_H */

/* slscapetools.h ends */
