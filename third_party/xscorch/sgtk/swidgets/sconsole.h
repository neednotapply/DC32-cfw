/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sconsole.h,v 1.18 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - sconsole.h       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Display "console"


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
#ifndef __sconsole_h_included
#define __sconsole_h_included


#include <gtk/gtk.h>
#include <sgtk.h>
#include <sdrawbuf.h>


/* Console typecasts */
#define  SC_CONSOLE(obj)         G_TYPE_CHECK_INSTANCE_CAST(obj, sc_console_get_type(), ScConsole)
#define  SC_CONSOLE_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, sc_console_get_type(), ScConsoleClass)
#define  IS_SC_CONSOLE(obj)      G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_console_get_type())


/* Console style information */
typedef enum _ScConsoleStyle {
   CONSOLE_NORMAL,               /* Normal borders */
   CONSOLE_BORDERLESS            /* No borders or padding */
} ScConsoleStyle;


/* Console colors */
typedef struct _ScConsoleColors {
   GdkColor foreground;          /* Normal foreground */
   GdkColor background;          /* Normal background */
   GdkColor forecursor;          /* Cursor foreground */
   GdkColor backcursor;          /* Cursor background */
   GdkColor foreshadow;          /* Shadow foreground */
   GdkColor backshadow;          /* Shadow background */
   GdkColor forescroll;          /* Scrollbar foreground */
   GdkColor backscroll;          /* Scrollbar background */
   GdkColor forelight;           /* Lighted foreground */
   GdkColor backlight;           /* Lighted background */
   GdkColor foredisabled;        /* Disabled foreground */
   GdkColor backdisabled;        /* Disabled background */
   GdkColor forestandard;        /* Standard foreground */
   GdkColor backstandard;        /* Standard background */
   gboolean colors_alloc;        /* True of FG/BG allocated */
   gboolean bold;                /* True if font should bold */
} ScConsoleColors;


/* Console color values */
typedef enum _ScConsoleColorId {
   SC_CONSOLE_FOREGROUND,
   SC_CONSOLE_BACKGROUND,
   SC_CONSOLE_FORECURSOR,
   SC_CONSOLE_BACKCURSOR,
   SC_CONSOLE_FORESHADOW,
   SC_CONSOLE_BACKSHADOW,
   SC_CONSOLE_FORESCROLL,
   SC_CONSOLE_BACKSCROLL,
   SC_CONSOLE_FORELIGHT,
   SC_CONSOLE_BACKLIGHT,
   SC_CONSOLE_FOREDISABLED,
   SC_CONSOLE_BACKDISABLED,
   SC_CONSOLE_FORESTANDARD,
   SC_CONSOLE_BACKSTANDARD
} ScConsoleColorId;


/* Console text */
typedef struct _ScConsoleText {
   char *buffer;                 /* Actual character data */
   gint bufferw;                 /* Width of char window  */
   gint bufferh;                 /* Height of char window */
   gint dispx;                   /* Actual display topleft X */
   gint dispy;                   /* Actual display topleft Y */
   gint viewx;                   /* Viewport topleft X */
   gint viewy;                   /* Viewport topleft Y */
   gint vieww;                   /* Viewport width */
   gint viewh;                   /* Viewport height */
   gboolean scrollx;             /* Scrollbar along X? */
   gboolean scrolly;             /* Scrollbar along Y? */
} ScConsoleText;


/* Console cursor */
typedef struct _ScConsoleCursor {
   gboolean highlighted;         /* Cursor is "highlighted" */
   gint x;                       /* Cursor X position */
   gint y;                       /* Cursor Y position */
   gint width;                   /* Width of cursor  */
   gint height;                  /* Height of cursor */
} ScConsoleCursor;


/* Console highlight */
typedef struct _ScConsoleHighlight {
   ScConsoleColors colors;       /* Highlight colors */
   gint x;                       /* X coordinate of light */
   gint y;                       /* Y coordinate of light */
   gint width;                   /* Width of the light  */
   gint height;                  /* Height of the light */
} ScConsoleHighlight;


/* The ScConsole structure */
typedef struct _ScConsole {
   ScDrawbuf draw_buffer;        /* Parent is a drawing buf */
   ScConsoleColors colors;       /* Colors for this console */
   GdkFont *screen_font;         /* Font used in this console */
   GdkFont *screen_bold_font;    /* Bold font that is used */
   GtkAllocation req_alloc;      /* Req. pos/size allocation */

   ScConsoleText text;           /* Data for the console text */
   ScConsoleCursor cursor;       /* Info associated with cursor */
   ScConsoleStyle style;         /* Windowing style for console */
   GList *highlights;            /* List of color highlights */
   gboolean ignorerelease;       /* Ignore next release if true */
} ScConsole;


/* ScConsoleClass structure */
typedef struct _ScConsoleClass {
   ScDrawbufClass parent_class;
   void     (*paint_region)(ScConsole *cons, GdkRectangle *bounds);
} ScConsoleClass;


/* Console initialisation */
GType sc_console_get_type(void);
GtkWidget *sc_console_new(gint x, gint y, gint width, gint height, ScConsoleStyle style,
                          GdkFont *font, GdkFont *boldfont);


/* Console modification */
void sc_console_init(ScConsole *cons, gint x, gint y, gint width, gint height, ScConsoleStyle style,
                     GdkFont *font, GdkFont *boldfont);
void sc_console_buffer_size(ScConsole *cons, gint width, gint height);


/* Writing strings and characters */
void sc_console_write_char(ScConsole *cons, gint x, gint y, char ch);
void sc_console_write_line(ScConsole *cons, gint x, gint y, const char *line);
void sc_console_write_line_wrap(ScConsole *cons, gint x, gint y, const char *line);
void sc_console_clear_line(ScConsole *cons, gint y);
void sc_console_clear(ScConsole *cons);


/* Setup colors and highlights */
#define sc_console_get_foreground(cons)      (&(cons)->colors.foreground)
#define sc_console_get_background(cons)      (&(cons)->colors.background)
void sc_console_set_fonts(ScConsole *cons, GdkFont *font, GdkFont *boldfont);
void sc_console_set_colors(ScConsole *cons, GdkColor *fg, GdkColor *bg);
#define sc_console_set_foreground(cons, fg)  sc_console_set_colors(cons, fg, NULL)
#define sc_console_set_background(cons, bg)  sc_console_set_colors(cons, NULL, bg)
GdkColor *sc_console_get_color(ScConsole *cons, ScConsoleColorId rqst);

void sc_console_highlight_attach(ScConsole *cons, GdkColor *fg, GdkColor *bg, gboolean bold, gint x, gint y, gint width, gint height);
void sc_console_highlight_attach_disabled(ScConsole *cons, gint x, gint y, gint width, gint height);
gboolean sc_console_highlight_detach(ScConsole *cons);
void sc_console_highlight_detach_all(ScConsole *cons);


/* Setup cursor attributes */
void sc_console_set_cursor(ScConsole *cons, gint x, gint y, gint width, gint height);
void sc_console_set_cursor_pos(ScConsole *cons, gint x, gint y);
void sc_console_set_cursor_highlighted(ScConsole *cons, gboolean highlighted);


/* General information functions */
#define sc_console_get_cursor_x(cons)        (cons->cursor.x)
#define sc_console_get_cursor_y(cons)        (cons->cursor.y)
#define sc_console_get_cursor_width(cons)    (cons->cursor.width)
#define sc_console_get_cursor_height(cons)   (cons->cursor.height)


/* These functions return the number of pixels required
   for a character, both a width and height. */
gint sc_console_get_col_width(ScConsole *cons);
gint sc_console_get_row_height(ScConsole *cons);


/* These functions convert a REAL pixel coordinate (that
   is relative to top-left of widget) to a character value,
   where character (0,0) is the top-left character on the
   screen.  These functions ADJUST to take scrolling (the
   viewport) into account if view is set to TRUE; for most
   applications, you should set view to TRUE.  */
gint sc_console_get_char_from_pixel_x(ScConsole *cons, gint x, gboolean view);
gint sc_console_get_char_from_pixel_y(ScConsole *cons, gint y, gboolean view);
void sc_console_get_char_from_pixel(ScConsole *cons, gint *x, gint *y, gboolean view);
gint sc_console_get_pixel_from_char_x(ScConsole *cons, gint x, gboolean view);
gint sc_console_get_pixel_from_char_y(ScConsole *cons, gint y, gboolean view);
void sc_console_get_pixel_from_char(ScConsole *cons, gint *x, gint *y, gboolean view);

/* Fancier versions of the same; the rectangle widths and
   heights will also be updated in these versions. */
void sc_console_get_char_from_pixel_rect(ScConsole *cons, GdkRectangle *r, gboolean view);
void sc_console_get_pixel_from_char_rect(ScConsole *cons, GdkRectangle *r, gboolean view);


/* These functions return characteristics of the text buffer,
   all return units in terms of characters.  The first two
   return the total size of the buffer, and the remaining
   functions return characteristics of the viewport (used when
   scrollbars are active). */
#define sc_console_get_width(cons)           (cons->text.bufferw)
#define sc_console_get_height(cons)          (cons->text.bufferh)
#define sc_console_get_view_x(cons)          (cons->text.viewx)
#define sc_console_get_view_y(cons)          (cons->text.viewy)
#define sc_console_get_view_width(cons)      (cons->text.vieww)
#define sc_console_get_view_height(cons)     (cons->text.viewh)


#endif /* __sconsole_h_included */
