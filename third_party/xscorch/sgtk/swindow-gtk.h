/* $Header: /fridge/cvs/xscorch/sgtk/swindow-gtk.h,v 1.16 2009-04-26 17:39:51 jacob Exp $ */
/*
   
   xscorch - swindow-gtk.h    Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK interface to xscorch
    

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
#ifndef __swindow_gtk_h_included
#define __swindow_gtk_h_included


/* The order of includes does matter here! */
#include <sgtk.h>
#include <sdialog.h>
#include <sconsole.h>
#include <sgame/swindow.h>
#include <gtk/gtk.h>


/* Forward structure declarations */
struct _sc_expl_cache_gtk;
struct _sc_colormap_gtk;
struct _sc_config;


/* Miscellaneous constants */
#define  SC_GTK_STRING_BUFFER    0x4000   /* String buffer size */
#define  SC_GTK_TOOLTIP_DELAY    1000     /* Delay (in ms) of tooltips */


/* Main window structure */
typedef struct _sc_window_gtk {
   struct _sc_config *c;                  /* Config structure */
int buffer_a[1024];
   struct _sc_color_gtk *colormap;        /* GTK allocated colors */
   struct _sc_expl_cache_gtk *explcache;  /* List of drawn explosions */
   guint state;                           /* ? */
   guint timer_id;                        /* Installed timer signal ID */
   guint windarrowsize;                   /* Size of the drawn wind arrow */
   gboolean ready;                        /* True when ready to play */
   gboolean exposed;                      /* True once win exposed */
   gboolean statenabled;                  /* True if status pane is enabled */
   GdkFont *fixed_font;                   /* Normal fixed-width font */
   GdkFont *italic_fixed_font;            /* Italic fixed-width font */
   GdkFont *bold_fixed_font;              /* Boldface fixed-width font */
   GtkWidget *app;                        /* GtkWindow main window */
   GtkWidget *mainmenu;                   /* Main menu bar */
   GtkWidget *status;                     /* Status widget */
   GtkWidget *screen;                     /* Drawing screen */
   GdkPixmap *landbuffer;                 /* Offscreen pix for land */
   GdkPixmap *logo;                       /* Offscreen game logo */
   GdkBitmap *logo_m;                     /* Offscreen logo mask */
   GdkPixmap *icon;                       /* Offscreen game icon */
   GdkBitmap *icon_m;                     /* Offscreen icon mask */
   GtkWidget *border;                     /* Border of playing field */
   GtkWidget *chatbox;                    /* Chatbox for network mode */
   GtkWidget *net_server;
   GtkWidget *net_client;
   GtkWidget *net_disconnect;
   GtkWidget *net_chat;
   GtkWidget *net_status;
} sc_window_gtk;


GtkWidget *sc_window_console_new(sc_window_gtk *w, int x, int y, int width, int height, ScConsoleStyle style);
GtkWidget *sc_window_active_console_new(sc_window_gtk *w, int x, int y, int width, int height, ScConsoleStyle style);

void sc_window_console_set_fonts(sc_window_gtk *w, ScConsole *cons);

void sc_window_load_fonts(sc_window_gtk *w);
void sc_window_unload_fonts(sc_window_gtk *w);
void sc_window_reload_fonts(sc_window_gtk *w);


#endif /* __swindow_gtk_h_included */
