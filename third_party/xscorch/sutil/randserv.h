/* $Header: /fridge/cvs/xscorch/sutil/randserv.h,v 1.3 2009-04-26 17:40:01 jacob Exp $ */
/*--------------------------------------------------------------------
 *
 *  randserv.h -- Random deviate generator
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

#ifndef RANDSERV_H
#define RANDSERV_H

double rand_normal(void);
double rand_uniform(void);
long   rand_discrete(long min, long max);

#endif /* RANDSERV_H */

/* randserv.h ends */
