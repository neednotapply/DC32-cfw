/* $Header: /fridge/cvs/xscorch/sgtk/sgtk.h,v 1.25 2009-04-26 17:39:48 jacob Exp $ */
/*

   xscorch - sgtk.h           Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Generic GTK header


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License ONLY.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#ifndef __sgtk_h_included
#define __sgtk_h_included


/* This file implies xscorch.h. */
#include <xscorch.h>


#ifndef __ALLOW_DEPRECATED_GDK__
   /* For reasons beyond my comprehension, they deprecated this in 2.0 */
   #define gdk_color_black(cmap, color) \
      do {                              \
         (color)->red   = 0;            \
         (color)->green = 0;            \
         (color)->blue  = 0;            \
         gdk_colormap_alloc_color((cmap), (color), FALSE, TRUE); \
      } while(0)
#endif /* __ALLOW_DEPRECATED_GDK__ undefined? */


/* Debugging constants */
#define  SC_GTK_DEBUG_GTK              0
#define  SC_GTK_DEBUG_PAINT            0


/* Disable deprecated GTK 2.0 interfaces, if applicable */
#if !defined(__ALLOW_DEPRECATED_GTK__)
   #define  GTK_DISABLE_DEPRECATED
#endif
#if !defined(__ALLOW_DEPRECATED_GDK__)
   #define  GDK_DISABLE_DEPRECATED
#endif


#endif /* __sgtk_h_included */
