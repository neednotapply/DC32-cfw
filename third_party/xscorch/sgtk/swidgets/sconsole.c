/*

   xscorch - sconsole.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Code for a text console


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
#define  __ALLOW_DEPRECATED_GDK__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sconsole.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



static ScDrawbufClass *parent_class;



#define  SC_CONSOLE_BORDER_WIDTH_CHARS       2
#define  SC_CONSOLE_BORDER_HEIGHT_CHARS      1



enum _ScConsoleSignals {
   PAINT_REGION_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_console_signals[LAST_SIGNAL] = { 0 };



/***  Low-level Support Functions  ***/



static inline char *_sc_console_get_text_buffer(ScConsole *cons) {
/* sc_console_get_text_buffer
   Returns the text buffer for this console.  */

   assert(IS_SC_CONSOLE(cons));
   return(cons->text.buffer);

}



static inline char *_sc_console_get_char_ptr(ScConsole *cons, int x, int y) {
/* sc_console_get_char_ptr
   Returns a pointer to the (x,y)'th character in the text buffer.  */

   char *p;

   assert(IS_SC_CONSOLE(cons));
   p = _sc_console_get_text_buffer(cons);
   if(p == NULL) return(NULL);
   return(p + y * sc_console_get_width(cons) + x);

}



static inline void _sc_console_valid_fonts(ScConsole *cons) {
/* sc_console_valid_fonts
   Make sure both fonts in this console are valid.  */

   assert(IS_SC_CONSOLE(cons));
   assert(cons->screen_font != NULL);
   assert(cons->screen_bold_font != NULL);

}



static inline void _sc_console_unref_fonts(ScConsole *cons) {
/* sc_console_unref_fonts
   Release the fonts on this console.  */

   assert(IS_SC_CONSOLE(cons));
   if(cons->screen_font != NULL) {
      gdk_font_unref(cons->screen_font);
      cons->screen_font = NULL;
   }
   if(cons->screen_bold_font != NULL) {
      gdk_font_unref(cons->screen_bold_font);
      cons->screen_bold_font = NULL;
   }

}



static inline void _sc_console_ref_fonts(ScConsole *cons) {
/* sc_console_ref_fonts
   Install new fonts for this console.  This function assumes the
   new GdkFont's have been assigned to screen_font and screen_bold_font,
   but have NOT yet been referenced.  This is not used when the font
   is loaded with gdk_font_load; it is ONLY used when we are passed a
   new font from the outside world, that we need to reference.  */

   assert(IS_SC_CONSOLE(cons));
   _sc_console_valid_fonts(cons);
   gdk_font_ref(cons->screen_font);
   gdk_font_ref(cons->screen_bold_font);

}



static inline void _sc_console_init_fonts(ScConsole *cons) {
/* sc_console_init_fonts
   Load a reasonable set of default fonts.  */

   assert(IS_SC_CONSOLE(cons));
   cons->screen_font = gdk_fontset_load("fixed");
   cons->screen_bold_font = gdk_fontset_load("fixed");
   _sc_console_valid_fonts(cons);

}



static inline void _sc_console_set_fonts(ScConsole *cons, GdkFont *font, GdkFont *boldfont) {
/* sc_console_set_fonts
   Release the current fonts on this console, and install the new fonts
   that are passed as arguments.  The reference count on the existing
   fonts is decremented, and the reference count on the new fonts will
   be incremented.  */

   _sc_console_unref_fonts(cons);
   cons->screen_font = font;
   cons->screen_bold_font = boldfont;
   _sc_console_ref_fonts(cons);

}



/***  Destructor  ***/



static void _sc_console_destroy_data(ScConsole *cons) {
/* sc_console_destroy_data
   Destroys the data buffer currently allocated to this console, and
   data allocated in the highlight structures.  This does not release
   interface widgets.  */

   GList *cur;

   assert(IS_SC_CONSOLE(cons));

   /* Delete the console text buffer */
   if(_sc_console_get_text_buffer(cons) != NULL) {
      free(_sc_console_get_text_buffer(cons));
      cons->text.buffer = NULL;
   }

   /* Delete the highlights list */
   cur = cons->highlights;
   while(cur != NULL) {
      if(cur->data != NULL) free(cur->data);
      cur->data = NULL;
      cur = cur->next;
   }
   g_list_free(cons->highlights);
   cons->highlights = NULL;

}



static void _sc_console_destroy(GtkObject *obj) {
/* sc_console_destroy
   Destroys the indicated console.  This includes all internal data
   structures, and all widgets associated with the console.  The
   current fonts will also be unreferenced.  */

   ScConsole *cons = SC_CONSOLE(obj);

   _sc_console_destroy_data(cons);
   _sc_console_unref_fonts(cons);

   /* Check for a parent signal handler */
   if(GTK_OBJECT_CLASS(parent_class)->destroy != NULL) {
      GTK_OBJECT_CLASS(parent_class)->destroy(obj);
   } /* Does parent have default? */

}



/***  Font Metrics, and Character<->Pixel Conversions  ***/



static inline gint _sc_console_char_width(ScConsole *cons) {
/* sc_console_char_width
   Return the width of one character, in pixels */

   int width1 = gdk_char_width(cons->screen_font, 'w');
   int width2 = gdk_char_width(cons->screen_bold_font, 'w');
   return(max(max(width1, width2), 1));

}



static inline gint _sc_console_char_height(ScConsole *cons) {
/* sc_console_char_height
   Return the height of one character, in pixels */

   int height1 = cons->screen_font->ascent + cons->screen_font->descent;
   int height2 = cons->screen_bold_font->ascent + cons->screen_bold_font->descent;
   return(max(max(height1, height2), 1));

}



static inline gint _sc_console_char_x(ScConsole *cons, gint x) {
/* sc_console_char_x
   Return the screen position of the X'th character.
   Does not compensate for scrolling, so char position is REAL */

   return(x * _sc_console_char_width(cons));

}



static inline gint _sc_console_char_y(ScConsole *cons, gint y) {
/* sc_console_char_y
   Return the screen position of the Y'th character
   Does not compensate for scrolling, so char position is REAL */

   return(y * _sc_console_char_height(cons));

}



gint sc_console_get_row_height(ScConsole *cons) {
/* sc_console_get_row_height
   Return one row height, in pixels */

   return(_sc_console_char_height(cons));

}



gint sc_console_get_col_width(ScConsole *cons) {
/* sc_console_get_col_width
   Return one column width, in pixels */

   return(_sc_console_char_width(cons));

}



inline gint sc_console_get_char_from_pixel_x(ScConsole *cons, gint x, gboolean view) {
/* sc_console_get_char_from_pixel_x
   Returns the character X position which is under pixel position x, where
   x is relative to the top-left of the widget.  This takes scrolling into
   account if view is TRUE, so you will get a valid position into the char
   buffer if view is set.  This always compensates for borders.  */

   x = x / sc_console_get_col_width(cons);
   if(view) x += sc_console_get_view_x(cons);
   if(cons->style != CONSOLE_BORDERLESS) x -= SC_CONSOLE_BORDER_WIDTH_CHARS;
   return(x);

}



inline gint sc_console_get_char_from_pixel_y(ScConsole *cons, gint y, gboolean view) {
/* sc_console_get_char_from_pixel_y
   Returns the character Y position which is under pixel position y, where
   y is relative to the top-left of the widget.  This takes scrolling into
   account if view is set to TRUE, so you will get a valid position into
   the char buffer if view is set.  This always compensates for borders. */

   y = y / sc_console_get_row_height(cons);
   if(view) y += sc_console_get_view_y(cons);
   if(cons->style != CONSOLE_BORDERLESS) y -= SC_CONSOLE_BORDER_HEIGHT_CHARS;
   return(y);

}



inline void sc_console_get_char_from_pixel(ScConsole *cons, gint *x, gint *y, gboolean view) {
/* sc_console_get_char_from_pixel
   Interface that updates both X and Y */

   *x = sc_console_get_char_from_pixel_x(cons, *x, view);
   *y = sc_console_get_char_from_pixel_y(cons, *y, view);

}



void sc_console_get_char_from_pixel_rect(ScConsole *cons, GdkRectangle *r, gboolean view) {
/* sc_console_get_char_from_pixel_rect
   Fancy rectangle version */

   r->x = sc_console_get_char_from_pixel_x(cons, r->x, view);
   r->y = sc_console_get_char_from_pixel_y(cons, r->y, view);
   r->width  = r->width  * sc_console_get_col_width(cons);
   r->height = r->height * sc_console_get_row_height(cons);

}



inline gint sc_console_get_pixel_from_char_x(ScConsole *cons, gint x, gboolean view) {
/* sc_console_get_pixel_from_char_x
   Returns the pixel X position which corresponds to character col x, where
   x is relative to the top-left of the inner border.  This takes scrolling
   into account if view is TRUE.  This always compensates for borders; if
   view is false, (0,0) will be inside the border, never outside. */

   if(cons->style != CONSOLE_BORDERLESS) x += SC_CONSOLE_BORDER_WIDTH_CHARS;
   if(view) x -= sc_console_get_view_x(cons);
   x = _sc_console_char_x(cons, x);
   return(x);

}



inline gint sc_console_get_pixel_from_char_y(ScConsole *cons, gint y, gboolean view) {
/* sc_console_get_pixel_from_char_y
   Returns the pixel Y position which corresponds to character col y, where
   y is relative to the top-left of the inner border.  This takes scrolling
   into account if view is TRUE.  This always compensates for borders; if
   view is false, (0,0) will be inside the border, never outside. */

   if(cons->style != CONSOLE_BORDERLESS) y += SC_CONSOLE_BORDER_HEIGHT_CHARS;
   if(view) y -= sc_console_get_view_y(cons);
   y = _sc_console_char_y(cons, y);
   return(y);

}



inline void sc_console_get_pixel_from_char(ScConsole *cons, gint *x, gint *y, gboolean view) {
/* sc_console_get_pixel_from_char
   Interface that updates both X and Y */

   *x = sc_console_get_pixel_from_char_x(cons, *x, view);
   *y = sc_console_get_pixel_from_char_y(cons, *y, view);

}



void sc_console_get_pixel_from_char_rect(ScConsole *cons, GdkRectangle *r, gboolean view) {
/* sc_console_get_pixel_from_char_rect
   Fancy rectangle version */

   r->x = sc_console_get_pixel_from_char_x(cons, r->x, view);
   r->y = sc_console_get_pixel_from_char_y(cons, r->y, view);
   r->width  = r->width  * sc_console_get_col_width(cons);
   r->height = r->height * sc_console_get_row_height(cons);

}



/***  Drawing Routines  ***/



static inline gboolean _sc_console_in_bounds(gint cx, gint cy, gint x, gint y, gint width, gint height) {
/* sc_console_in_bounds
   Returns true if the indicated (cx,cy) coordinate is within the bounds
   of the rectangle with corner (x,y) and the indicated width and height. */

   assert(width >= 0);
   assert(height >= 0);
   return(cx >= x && cy >= y && cx < x + width && cy < y + height);

}



static void _sc_console_draw_char(ScConsole *cons, GdkGC *fg, GdkGC *bg, gboolean bold, gint x, gint y, char ch) {
/* sc_console_draw_char
   Draws a single character using the indicated fg/bg GCs, and at the
   CHARACTER position (x,y).  The coordinate will be converted to
   pixels automatically in this function.  The character to write is
   specified by this function; this function will select the bold or
   normal font based on the "bold" flag.  
   
   The coordinates are CHARACTER coordinates, but they are with respect
   to the VIEWPORT, not the character buffer.  */

   GdkFont *font;

   assert(IS_SC_CONSOLE(cons));
   assert(fg != NULL);
   assert(bg != NULL);

   /* Which font to select? */
   if(bold) {
      font = cons->screen_bold_font;
   } else {
      font = cons->screen_font;
   }

   /* Update X, Y to account for a window frame; also,
      set X, Y to the screen coordinates to write to. */
   sc_console_get_pixel_from_char(cons, &x, &y, FALSE);

   /* Write the text! */
   gdk_draw_rectangle(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                      bg,
                      TRUE,
                      x, y,
                      _sc_console_char_width(cons), _sc_console_char_height(cons));
   gdk_draw_text(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                 font, fg,
                 x, y + font->ascent,
                 &ch, 1);

}



static void _sc_console_draw_region(ScConsole *cons, gint x, gint y, gint width, gint height) {
/* sc_console_draw_region
   Draws a region of the console, specified by the rectangle using CHARACTER
   coordinates, with corner (x,y) and the indicated width and height.  This
   function uses information about active highlights and the text buffer to
   determine what to draw.  width/height may be negative, in which case the
   rectangle is flipped appropriately.  
   
   The coordinates are CHARACTER coordinates, but they are relative to the
   CHARACTER BUFFER, not the viewport.  */

   ScConsoleHighlight *high;  /* Active highlight for current position */
   GdkColor *oldfgcolor;      /* Previous foreground colour (to speed up GC ops) */
   GdkColor *oldbgcolor;      /* Previous background colour (to speed up GC ops) */
   GdkRectangle region;       /* Rectangle representing redraw region */
   GdkColor *fgcolor;         /* Current foreground colour */
   GdkColor *bgcolor;         /* Current background colour */
   GdkGC *fg_gc;              /* Foreground GC */
   GdkGC *bg_gc;              /* Background GC */
   GList *cur;                /* List to iterate the highlights */

   char *chptr;               /* Pointer to current character to draw */
   gint x1;                   /* Left X boundary */
   gint y1;                   /* Top Y boundary  */
   gint x2;                   /* Right X boundary */
   gint y2;                   /* Bottom Y boundary */
   gint cx;                   /* Current X iter */
   gint cy;                   /* Current Y iter */
   gboolean bold;             /* Draw this in bold? */

   assert(IS_SC_CONSOLE(cons));

   if(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)) == NULL) return;

   /* Set up the region variable. */
   region.x = x;
   region.y = y;
   region.width = width;
   region.height = height;

   x1 = x;
   y1 = y;
   x2 = x + width - 1;
   y2 = y + height - 1;

   /* Check to see if width/height were negative, and swap if needed */
   if(x1 > x2) {
      x1 = x1 + x2;
      x2 = x1 - x2;
      x1 = x1 - x2;
   }
   if(y1 > y2) {
      y1 = y1 + y2;
      y2 = y1 - y2;
      y1 = y1 - y2;
   }

   /* Clip the bottom/left coordinates based on extents of the text buffer */
   if(x1 < cons->text.viewx) x1 = cons->text.viewx;
   if(y1 < cons->text.viewy) y1 = cons->text.viewy;
   if(x1 >= cons->text.viewx + cons->text.vieww) return;
   if(y1 >= cons->text.viewy + cons->text.viewh) return;

   /* Clip the top/right coordinates based on extents of the text buffer */
   if(x2 < cons->text.viewx) return;
   if(y2 < cons->text.viewy) return;
   if(x2 >= cons->text.viewx + cons->text.vieww) x2 = cons->text.viewx + cons->text.vieww - 1;
   if(y2 >= cons->text.viewy + cons->text.viewh) y2 = cons->text.viewy + cons->text.viewh - 1;

   /* Request a GC */
   fg_gc = gdk_gc_new(((GtkWidget *)cons)->window);
   bg_gc = gdk_gc_new(((GtkWidget *)cons)->window);

   /* Setup default foreground, background colors */
   oldfgcolor = &cons->colors.foreground;
   oldbgcolor = &cons->colors.background;
   gdk_gc_set_foreground(fg_gc, oldfgcolor);
   gdk_gc_set_foreground(bg_gc, oldbgcolor);

   /* Start printing characters... */
   for(cy = y1; cy <= y2; ++cy) {

      /* Get the character to be drawn */
      chptr = _sc_console_get_char_ptr(cons, x1, cy);
      g_return_if_fail(chptr != NULL);
      for(cx = x1; cx <= x2; ++cx, ++chptr) {

         fgcolor = &cons->colors.foreground;
         bgcolor = &cons->colors.background;
         bold = cons->colors.bold;

         /* What highlight are we using? */
         cur = cons->highlights;
         while(cur != NULL) {
            /* Are we on this highlight? */
            high = cur->data;
            assert(high != NULL);
            cur = cur->next;
            if(_sc_console_in_bounds(cx, cy, high->x, high->y, high->width, high->height)) {
               /* We are on this highlight; set new fg/bg color */
               if(!high->colors.colors_alloc) {
                  high->colors.colors_alloc = TRUE;
                  gdk_colormap_alloc_color(gtk_widget_get_colormap((GtkWidget *)cons),
                                           &high->colors.foreground, FALSE, TRUE);
                  gdk_colormap_alloc_color(gtk_widget_get_colormap((GtkWidget *)cons),
                                           &high->colors.background, FALSE, TRUE);
               }
               fgcolor = &high->colors.foreground;
               bgcolor = &high->colors.background;
               bold = high->colors.bold;
            } /* Coordinates in bound? */
         } /* Iterating thru installed highlights ... */

         /* Are we drawing the new cursor? */
         if(_sc_console_in_bounds(cx, cy, cons->cursor.x, cons->cursor.y,
                                  cons->cursor.width, cons->cursor.height)) {
            /* Welp, we're on the cursor.. all that work, for nothing */
            if(gtk_widget_has_focus((GtkWidget *)cons)) {
               if(cons->cursor.highlighted) {
                  fgcolor = &cons->colors.forelight;
                  bgcolor = &cons->colors.backlight;
               } else {
                  fgcolor = &cons->colors.forecursor;
                  bgcolor = &cons->colors.backcursor;
               }
            } else {
               fgcolor = &cons->colors.foreshadow;
               bgcolor = &cons->colors.backshadow;
            }
         } /* We be a cursor? */

         /* Only update the GC if we absolutely have to */
         if(oldfgcolor != fgcolor) {
            oldfgcolor = fgcolor;
            gdk_gc_set_foreground(fg_gc, fgcolor);
         }
         if(oldbgcolor != bgcolor) {
            oldbgcolor = bgcolor;
            gdk_gc_set_foreground(bg_gc, bgcolor);
         }

         _sc_console_draw_char(cons, fg_gc, bg_gc, bold,
                               cx - cons->text.viewx, cy - cons->text.viewy, *chptr);
      } /* Iterating thru X */
   } /* Iterate thru Y */

   /* Update X, Y to account for a window frame; also,
      set X, Y to the screen coordinates to write to. */
   sc_console_get_pixel_from_char(cons, &x, &y, TRUE);

   /* Update width, height to screen coordinates */
   width = width * _sc_console_char_width(cons);
   height= height* _sc_console_char_height(cons);

   /* Make sure this update is queued for display to screen. */
   sc_drawbuf_queue_draw(SC_DRAWBUF(cons), x, y, width, height);

   /* Propagate the draw request to the active console. */
   g_signal_emit_by_name(GTK_OBJECT(cons), "paint_region", &region, NULL);

   /* Release the GC's */
   g_object_unref(fg_gc);
   g_object_unref(bg_gc);

}



static void _sc_console_draw_horiz_scroll(__libj_unused ScConsole *cons) {
/* sc_console_draw_horiz_scroll
   Currently not supported :(  */

}



static inline void _sc_console_vert_scroll_extents(ScConsole *cons,
                                                   gint *startx, gint *starty,
                                                   gint *width, gint *height,
                                                   gint *arrowh) {
/* sc_console_vert_scroll_extents
   Figure out the extents of the vertical scrollbar.  The arguments are
      startx, starty: bottomleft coordinate of scrollbar body.  The starty
                      is ABOVE the bottom arrow.
      width, height:  total dimensions of the scrollbar body.  The height
                      does NOT include the height of the 2 arrows.
      arrowh:         height of each arrow (the width of arrow is == width).
 */

   *width  = ((_sc_console_char_width(cons) + 2) & ~1);
   *height = GTK_WIDGET(cons)->allocation.height - 3 * _sc_console_char_height(cons);
   *startx = GTK_WIDGET(cons)->allocation.width - 2 * _sc_console_char_width(cons) + 1;
   *starty = _sc_console_char_height(cons) * 3 / 2;
   *arrowh = *width + 2;

}



static inline void _sc_console_vert_trough_extents(ScConsole *cons, gint starty, gint height,
                                                   gint *pos, gint *size) {
/* sc_console_vert_trough_extents
   Figure out the extents of the trough (the "box" in the scrollbar body
   that indicates the current position of the scrollbar and the size of
   the viewable area).  The function is given starty and height, and
   returns pos, which indicates the bottommost coordinate of the trough,
   and size, which indicates the height of the trough.  */

   *pos    = starty + height * cons->text.viewy / cons->text.bufferh;
   *size   = height * cons->text.viewh / cons->text.bufferh;

}



static inline gboolean _sc_console_can_scroll_up(ScConsole *cons) {
/* sc_console_can_scroll_up
   Returns true if it is possible to scroll up currently.  */

   return(sc_console_get_view_y(cons) > 0);

}



static inline gboolean _sc_console_can_scroll_down(ScConsole *cons) {
/* sc_console_can_scroll_down
   Returns true if it is possible to scroll down currently.  */

   return(sc_console_get_view_y(cons) + sc_console_get_view_height(cons) < sc_console_get_height(cons));

}



static void _sc_console_draw_vert_scroll(ScConsole *cons) {
/* sc_console_draw_vert_scroll
   Draws the vertical scrollbar.  */

   GtkWidget *widget = (GtkWidget *)cons;
   GdkPoint points[3];
   GdkGC *foreground;
   gint arrowh;
   gint startx;
   gint starty;
   gint height;
   gint width;
   gint size;
   gint pos;

   assert(IS_SC_CONSOLE(cons));

   /* Can we even draw yet? */
   if(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)) == NULL) return;
   if(cons->style == CONSOLE_BORDERLESS) return;

   /* Request a GC */
   foreground = gdk_gc_new(widget->window);

   /* Determine vertical scrollbar extents */
   _sc_console_vert_scroll_extents(cons, &startx, &starty, &width, &height, &arrowh);
   _sc_console_vert_trough_extents(cons, starty, height, &pos, &size);

   /* erase any original bars */
   gdk_gc_set_foreground(foreground, &cons->colors.backscroll);
   gdk_draw_rectangle(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                      foreground,
                      TRUE,
                      startx, starty - arrowh,
                      width, height + 2 * arrowh);

   /* setup gc colors for bar */
   gdk_gc_set_foreground(foreground, &cons->colors.forescroll);

   /* Draw vertical slider */
   gdk_draw_rectangle(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                      foreground,
                      TRUE,
                      startx, pos,
                      width, size);

   /* Determine if up-arrow is required */
   if(_sc_console_can_scroll_up(cons)) {
      points[0].x = startx + width / 2;
      points[0].y = starty - arrowh;
      points[1].x = points[0].x - width / 2;
      points[1].y = starty - 2;
      points[2].x = points[0].x + width / 2;
      points[2].y = starty - 2;
      gdk_draw_polygon(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                       foreground,
                       TRUE,
                       points, 3);
   } /* Up arrow? */

   /* Determine if down-arrow is required */
   if(_sc_console_can_scroll_down(cons)) {
      points[0].x = startx + width / 2;
      points[0].y = starty + height + arrowh;
      points[1].x = points[0].x - width / 2;
      points[1].y = starty + height + 2;
      points[2].x = points[0].x + width / 2;
      points[2].y = starty + height + 2;
      gdk_draw_polygon(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                       foreground,
                       TRUE,
                       points, 3);
   } /* Down arrow? */

   /* Release the GC's */
   g_object_unref(foreground);

   /* Make sure everything is queued for draw */
   sc_drawbuf_queue_draw(SC_DRAWBUF(widget), startx, starty - arrowh,
                         width, height + 2 * arrowh);

}



static void _sc_console_draw_frame(ScConsole *cons) {
/* sc_console_draw_frame
   Draws the window border for this console, if applicable, and draws
   the scrollbars for this console if they need to be displayed.  */

   GtkWidget *widget = (GtkWidget *)cons;
   GdkGC *foreground;
   GdkGC *background;

   assert(IS_SC_CONSOLE(cons));

   /* Can we even draw yet? */
   if(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)) == NULL) return;

   /* Request a GC */
   foreground = gdk_gc_new(widget->window);
   background = gdk_gc_new(widget->window);

   /* setup gc colors */
   gdk_gc_set_foreground(foreground, &cons->colors.foreground);
   gdk_gc_set_foreground(background, &cons->colors.background);

   /* Clear the screen; draw window border if appropriate */
   gdk_draw_rectangle(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                      background,
                      TRUE,
                      0, 0,
                      widget->allocation.width, widget->allocation.height);
   if(cons->style != CONSOLE_BORDERLESS) {
      /* Draw a border as well... */
      gdk_draw_rectangle(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)),
                         foreground,
                         FALSE,
                         _sc_console_char_width(cons) - 4, _sc_console_char_height(cons) / 2 - 2,
                         widget->allocation.width - 2 * _sc_console_char_width(cons) + 8,
                         widget->allocation.height - _sc_console_char_height(cons) + 4);

      /* Did we need scrollers? */
      if(cons->text.scrollx) {
         _sc_console_draw_horiz_scroll(cons);
      } /* horizontal scrollbar */
      if(cons->text.scrolly) {
         _sc_console_draw_vert_scroll(cons);
      } /* vertical scrollbar */
   } /* Draw the window border? */

   /* Release the GC's */
   g_object_unref(foreground);
   g_object_unref(background);

}



static void _sc_console_draw_all(ScConsole *cons) {
/* sc_console_draw_all
   Redraws the entire console, including all displayable text, all borders,
   and the scrollbars if they are applicable.  */

   GtkWidget *widget = (GtkWidget *)cons;

   assert(IS_SC_CONSOLE(cons));

   /* Can we even draw yet? */
   if(sc_drawbuf_get_buffer(SC_DRAWBUF(cons)) == NULL) return;

   /* Setup default foreground, background colors */
   /* (make sure to allocate them if not done already) */
   if(!cons->colors.colors_alloc) {
      cons->colors.colors_alloc = TRUE;
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.foreground, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.background, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.forecursor, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backcursor, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.foreshadow, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backshadow, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.forescroll, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backscroll, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.forelight, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backlight, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.foredisabled, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backdisabled, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.forestandard, FALSE, TRUE);
      gdk_colormap_alloc_color(gtk_widget_get_colormap(widget), &cons->colors.backstandard, FALSE, TRUE);
   }

   /* Redraw the window frame */
   _sc_console_draw_frame(cons);

   /* Redraw each line of text */
   _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));

   /* Make sure everything is queued for draw */
   sc_drawbuf_queue_draw(SC_DRAWBUF(widget), 0, 0, widget->allocation.width, widget->allocation.height);

}



/***  Standard Events  ***/



static gint _sc_console_configure(GtkWidget *widget, GdkEventConfigure *event) {
/* sc_console_configure
   This is the configure event when a console is resized.  Takes
   care of redrawing the entire console.  */

   /* Check for a parent signal handler */
   if(GTK_WIDGET_CLASS(parent_class)->configure_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->configure_event(widget, event)) {
         /* Oops; we must halt */
         return(TRUE);
      } /* Can we continue? */
   } /* Does parent have default? */

   /* Draw the console */
   _sc_console_draw_all(SC_CONSOLE(widget));

   /* Let other events run as well... */
   return(FALSE);

}



static gint _sc_console_draw_focus(GtkWidget *widget, GdkEventFocus *event) {
/* sc_console_draw_focus
   Draws the console's cursor as if it were focused.  This sets the GTK
   flags to indicate the console currently has the focus.  */

   ScConsole *cons = SC_CONSOLE(widget);

   gtk_widget_grab_focus(widget);
   _sc_console_draw_region(cons, cons->cursor.x, cons->cursor.y, cons->cursor.width, cons->cursor.height);

   /* Check for a parent signal handler */
   if(GTK_WIDGET_CLASS(parent_class)->focus_in_event != NULL) {
      return(GTK_WIDGET_CLASS(parent_class)->focus_in_event(widget, event));
   } /* Does parent have default? */

   /* Allow other events */
   return(FALSE);

}



static gint _sc_console_undraw_focus(GtkWidget *widget, GdkEventFocus *event) {
/* sc_console_undraw_focus
   Draws the console's cursor as if it were unfocused.  */

   ScConsole *cons = SC_CONSOLE(widget);

   _sc_console_draw_region(cons, cons->cursor.x, cons->cursor.y, cons->cursor.width, cons->cursor.height);

   /* Check for a parent signal handler */
   if(GTK_WIDGET_CLASS(parent_class)->focus_out_event != NULL) {
      return(GTK_WIDGET_CLASS(parent_class)->focus_out_event(widget, event));
   } /* Does parent have default? */

   /* Allow other events */
   return(FALSE);

}



static gint _sc_console_button_press(GtkWidget *widget, GdkEventButton *event) {
/* sc_console_button_press
   Someone clicked the mouse.  Update the cursor/active highlight if
   needed, and check for scrollbar events.  */

   ScConsole *cons = SC_CONSOLE(widget);
   gboolean needredraw = FALSE;
   gint arrowh;
   gint height;
   gint width;
   gint size;
   gint pos;
   gint x;
   gint y;

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->button_press_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   /* Make sure this is a SINGLE click event */
   if(event->type != GDK_BUTTON_PRESS) return(FALSE);

   /* See if this was a click onto a scrollbar */
   if(cons->text.scrolly) {
      /* Check to see if we're clicking in vertical scrollbar region */
      _sc_console_vert_scroll_extents(cons, &x, &y, &width, &height, &arrowh);
      _sc_console_vert_trough_extents(cons, y, height, &pos, &size);
      if(event->x >= x && event->x < x + width &&
        event->y >= y - arrowh && event->y < y + height + arrowh) {
         /* Click was onto the scrollbar itself */
         if(event->y >= y - arrowh && event->y < y) {
            /* Clicked up-arrow */
            if(_sc_console_can_scroll_up(cons)) {
               --cons->text.viewy;
               needredraw = TRUE;
            } /* Can we scroll down? */
            needredraw = TRUE;
         } else if(event->y >= y + height && event->y < y + height + arrowh) {
            if(_sc_console_can_scroll_down(cons)) {
               ++cons->text.viewy;
               needredraw = TRUE;
            } /* Can we scroll down? */
         } else if(event->y < pos) {
            /* Page up */
            if(_sc_console_can_scroll_up(cons)) {
               cons->text.viewy -= cons->text.viewh;
               if(cons->text.viewy < 0) cons->text.viewy = 0;
               needredraw = TRUE;
            } /* Can we page up? */
         } else if(event->y >= pos + size) {
            /* Page down */
            if(_sc_console_can_scroll_down(cons)) {
               cons->text.viewy += cons->text.viewh;
               if(cons->text.viewy > sc_console_get_height(cons) - cons->text.viewh) {
                  cons->text.viewy = sc_console_get_height(cons) - cons->text.viewh;
               }
               needredraw = TRUE;
            } /* Can we page up? */
         } /* Checking Y coordinate */

         /* Redraw components? */
         if(needredraw) {
            _sc_console_draw_vert_scroll(cons);
            _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));
         }

         /* Click was in scrollbar, abort */
         cons->ignorerelease = TRUE;
         return(TRUE);
      } /* Checking X, Y coordinate */
   } /* Checking vertical scrollbar */

   /* Nothing interesting... */
   return(FALSE);

}



static gint _sc_console_button_release(GtkWidget *widget, GdkEventButton *event) {
/* sc_console_button_release
   Someone released the mouse button.  Update the cursor/active highlight if
   needed, and check for scrollbar events.  */

   ScConsole *cons = SC_CONSOLE(widget);

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->button_release_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->button_release_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   /* See if this was a click onto a scrollbar */
   if(cons->ignorerelease) {
      cons->ignorerelease = FALSE;
      return(TRUE);
   } /* Clicked a scrollbar */

   /* Nothing interesting... */
   return(FALSE);

}



static gint _sc_console_key_press(GtkWidget *widget, GdkEventKey *event) {
/* sc_console_key_press
   Process a key that was pressed.  */

   ScConsole *cons = SC_CONSOLE(widget);

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->key_press_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   switch(event->keyval) {
      case GDK_Page_Up:
      case GDK_KP_Page_Up:
         if(_sc_console_can_scroll_up(cons)) {
            cons->text.viewy -= cons->text.viewh;
            if(cons->text.viewy < 0) cons->text.viewy = 0;
            _sc_console_draw_vert_scroll(cons);
            _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));
         }
         return(TRUE);

      case GDK_Page_Down:
      case GDK_KP_Page_Down:
         if(_sc_console_can_scroll_down(cons)) {
            cons->text.viewy += cons->text.viewh;
            if(cons->text.viewy > sc_console_get_height(cons) - cons->text.viewh) {
               cons->text.viewy = sc_console_get_height(cons) - cons->text.viewh;
            }
            _sc_console_draw_vert_scroll(cons);
            _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));
         }
         return(TRUE);
   } /* Search for special keys */

   /* Fallthrough */
   return(FALSE);

}



static gint _sc_console_key_release(GtkWidget *widget, GdkEventKey *event) {
/* sc_console_key_release
   Process a key that was released.  */

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->key_release_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->key_release_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   switch(event->keyval) {
      case GDK_Page_Up:
      case GDK_KP_Page_Up:
      case GDK_Page_Down:
      case GDK_KP_Page_Down:
         return(TRUE);
   } /* Search for special keys */

   /* Fallthrough */
   return(FALSE);

}



/***  Console Class Initializers  ***/



static void _sc_console_class_init(ScConsoleClass *klass) {
/* sc_console_class_init
   Initialise the console class.  */

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   /* Determine parent class */
   parent_class = g_type_class_peek(sc_drawbuf_get_type());

   /* Construct new signals */
   _sc_console_signals[PAINT_REGION_SIGNAL] =
      g_signal_new("paint_region",                 /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScConsoleClass, paint_region),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__POINTER,
                                                   /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first parametre to marshaller */
                  );

   /* Attach default signal handlers */
   klass->paint_region                          = NULL;
   GTK_OBJECT_CLASS(klass)->destroy             = _sc_console_destroy;
   GTK_WIDGET_CLASS(klass)->configure_event     = _sc_console_configure;
   GTK_WIDGET_CLASS(klass)->focus_in_event      = _sc_console_draw_focus;
   GTK_WIDGET_CLASS(klass)->focus_out_event     = _sc_console_undraw_focus;
   GTK_WIDGET_CLASS(klass)->button_press_event  = _sc_console_button_press;
   GTK_WIDGET_CLASS(klass)->button_release_event= _sc_console_button_release;
   GTK_WIDGET_CLASS(klass)->key_press_event     = _sc_console_key_press;
   GTK_WIDGET_CLASS(klass)->key_release_event   = _sc_console_key_release;

}



static void _sc_console_init_obj(ScConsole *cons) {
/* sc_console_init_obj
   Initialize a new instance of a console.  */

   /* Clear the text buffer */
   cons->text.scrollx = 0;
   cons->text.scrolly = 0;
   cons->text.buffer  = NULL;
   _sc_console_init_fonts(cons);
   cons->cursor.highlighted = FALSE;
   cons->highlights = NULL;
   cons->ignorerelease = FALSE;

   /* Reset the cursor */
   cons->cursor.x = 0;
   cons->cursor.y = 0;
   cons->cursor.width = 0;
   cons->cursor.height= 0;

   /* Initialize console colours */
   gdk_color_parse("#000000", &cons->colors.background);
   gdk_color_parse("#c0c0c0", &cons->colors.foreground);
   gdk_color_parse("#701010", &cons->colors.backcursor);
   gdk_color_parse("#ffffff", &cons->colors.forecursor);
   gdk_color_parse("#202020", &cons->colors.backshadow);
   gdk_color_parse("#e0e0e0", &cons->colors.foreshadow);
   gdk_color_parse("#101060", &cons->colors.backscroll);
   gdk_color_parse("#7090a0", &cons->colors.forescroll);
   gdk_color_parse("#a02020", &cons->colors.backlight);
   gdk_color_parse("#ffffff", &cons->colors.forelight);
   gdk_color_parse("#000000", &cons->colors.backdisabled);
   gdk_color_parse("#606060", &cons->colors.foredisabled);
   gdk_color_parse("#000000", &cons->colors.backstandard);
   gdk_color_parse("#c0c0c0", &cons->colors.forestandard);
   cons->colors.colors_alloc = FALSE;
   cons->colors.bold = FALSE;

   /* Make sure the console is focusable. */
   gtk_widget_set_can_focus(GTK_WIDGET(cons), TRUE);

}



GType sc_console_get_type(void) {
/* sc_console_get_type
   Return the console type.  */

   static GType sc_console_type = 0;

   if(sc_console_type == 0) {
      static const GTypeInfo sc_console_info = {
         sizeof(ScConsoleClass),          /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_console_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScConsole),               /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_console_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_console_type = g_type_register_static(sc_drawbuf_get_type(), "ScConsole",
                                               &sc_console_info, 0);
   }

   return(sc_console_type);

}



void sc_console_buffer_size(ScConsole *cons, gint width, gint height) {
/* sc_console_buffer_size
   This resets the console size.  The buffer text is cleared.  This does not
   automatically redraw the console; it is generally used as an initialization
   event only.  This does NOT update the viewport dimensions, although it will
   reset the viewport position to the top-left of the buffer.  */

   if(width <= 0 || height <= 0) {
      printf("Let me get this straight. You want to create an empty buffer?\n");
      printf("... oh, you are SO going to regret this operation.  The console\n");
      printf("... library will make you SUFFER HORRIBLY for this transgression.\n");
      printf("(this is Jacob's honorary bug!)\n");
   }
   g_return_if_fail(width > 0 && height > 0);

   if(width < cons->text.vieww) width = cons->text.vieww;
   if(height< cons->text.viewh) height= cons->text.viewh;
   cons->text.buffer = (char *)realloc(cons->text.buffer, width * height);
   if(cons->text.buffer != NULL) {
      memset(cons->text.buffer, ' ', width * height);
   }

   cons->text.viewx   = 0;
   cons->text.viewy   = 0;
   cons->text.bufferw = width;
   cons->text.bufferh = height;
   cons->text.scrollx = (cons->text.vieww < width);
   cons->text.scrolly = (cons->text.viewh < height);

}



static inline void _sc_console_init_dimensions(ScConsole *cons) {
/* sc_console_init_dimensions
   Initialize console dimensions based on the current font metrics, and set
   the console's usize request.  */

   int x;
   int y;
   int width;
   int height;

   assert(IS_SC_CONSOLE(cons));

   x      = cons->text.dispx;
   y      = cons->text.dispy;
   width  = cons->text.vieww;
   height = cons->text.viewh;

   if(cons->style != CONSOLE_BORDERLESS) {
      x -= SC_CONSOLE_BORDER_WIDTH_CHARS;
      y -= SC_CONSOLE_BORDER_HEIGHT_CHARS;
      width += SC_CONSOLE_BORDER_WIDTH_CHARS * 2;
      height+= SC_CONSOLE_BORDER_HEIGHT_CHARS * 2;
   }

   cons->req_alloc.x = _sc_console_char_x(cons, x);
   cons->req_alloc.y = _sc_console_char_y(cons, y);
   cons->req_alloc.width = width * _sc_console_char_width(cons);
   cons->req_alloc.height= height* _sc_console_char_height(cons);

   gtk_widget_set_size_request(GTK_WIDGET(cons), cons->req_alloc.width, cons->req_alloc.height);

}




void sc_console_init(ScConsole *cons, gint x, gint y, gint width, gint height, ScConsoleStyle style,
                     GdkFont *font, GdkFont *boldfont) {
/* sc_console_init
   Initialize a console.  */

   g_return_if_fail(IS_SC_CONSOLE(cons));
   g_return_if_fail(x >= 0 && y >= 0);
   g_return_if_fail(width > 0 && height > 0);
   g_return_if_fail(font != NULL && boldfont != NULL);

   _sc_console_destroy_data(cons);
   _sc_console_set_fonts(cons, font, boldfont);

   cons->text.vieww   = width;
   cons->text.viewh   = height;
   cons->text.dispx   = x;
   cons->text.dispy   = y;
   sc_console_buffer_size(cons, width, height);

   cons->style = style;

   _sc_console_init_dimensions(cons);

   gtk_widget_set_sensitive(GTK_WIDGET(cons), FALSE);

   sc_console_clear(cons);

}



void sc_console_clear(ScConsole *cons) {
/* sc_console_clear
   Erase console text and release all highlights.  */

   g_return_if_fail(IS_SC_CONSOLE(cons));

   if(cons->text.buffer != NULL) {
      memset(cons->text.buffer, ' ', sc_console_get_width(cons) * sc_console_get_height(cons));
   }
   sc_console_highlight_detach_all(cons);
   _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));

}



GtkWidget *sc_console_new(gint x, gint y, gint width, gint height, ScConsoleStyle style,
                          GdkFont *font, GdkFont *boldfont) {
/* sc_console_new
   Create a new console.  */

   ScConsole *cons;

   g_return_val_if_fail(x >= 0 && y >= 0, NULL);
   g_return_val_if_fail(width > 0 && height > 0, NULL);
   g_return_val_if_fail(font != NULL && boldfont != NULL, NULL);
   
   cons = g_object_new(sc_console_get_type(), NULL);
   g_return_val_if_fail(cons != NULL, NULL);

   sc_console_init(cons, x, y, width, height, style, font, boldfont);
   return(GTK_WIDGET(cons));

}



/***  Console Write Functions  ***/



void sc_console_write_char(ScConsole *cons, gint x, gint y, char ch) {
/* sc_console_write_char
   Writes a single character at (x,y).  */

   char *p;

   g_return_if_fail(IS_SC_CONSOLE(cons));

   if(x < 0 || x >= sc_console_get_width(cons)) return;
   if(y < 0 || y >= sc_console_get_height(cons)) return;

   if(ch < 0x20) ch = ' ';
   p = _sc_console_get_char_ptr(cons, x, y);
   g_return_if_fail(p != NULL);
   *p = ch;
   _sc_console_draw_region(cons, x, y, 1, 1);

}



void sc_console_write_line(ScConsole *cons, gint x, gint y, const char *line) {
/* sc_console_write_line
   Writes a line of text starting at character position (x,y).  If the
   line overflows the width of the text buffer, then the line is
   truncated.  If x < 0, then the first (-x) characters of line will
   be trimmed, and printing will begin in column 0.  */

   gint width;
   const char *start;
   const char *end;
   char *p;

   g_return_if_fail(IS_SC_CONSOLE(cons));
   g_return_if_fail(line != NULL);

   width = sc_console_get_width(cons);
   if(y < 0 || y >= sc_console_get_height(cons)) return;

   for(start = line; *start != '\0' && (line - start) + x < 0; ++start) /* Just loop */;
   for(end = start; *end != '\0' && (end - start) + x < width; ++end) /* Just loop */;
   if(end == start) return;

   p = _sc_console_get_char_ptr(cons, x, y);
   g_return_if_fail(p != NULL);
   memcpy(p, start, end - start);
   _sc_console_draw_region(cons, x, y, end - start, 1);

}



void sc_console_clear_line(ScConsole *cons, gint y) {
/* sc_console_clear_line
   Clears a single line of text on row y.  */

   char *p;

   g_return_if_fail(IS_SC_CONSOLE(cons));
   if(y < 0 || y >= sc_console_get_height(cons)) return;

   p = _sc_console_get_char_ptr(cons, 0, y);
   g_return_if_fail(p != NULL);
   memset(p, ' ', sc_console_get_width(cons));
   _sc_console_draw_region(cons, 0, y, sc_console_get_width(cons), 1);

}



void sc_console_write_line_wrap(ScConsole *cons, gint x, gint y, const char *line) {
/* sc_console_write_line_wrap
   Writes a line of text starting at character position (x,y).  If the
   line overflows the width of the text buffer, then it is continued
   on the next line starting in column 0.  If the text overflows the
   height of the text buffer in this manner, then it will be truncated.
   
   It is considered an error if x < 0 or y < 0.  If x is beyond the
   width of the line, then printing will start on the next line.  */

   gint width;
   gint height;
   gint minx;
   char *p;
   
   g_return_if_fail(IS_SC_CONSOLE(cons));
   g_return_if_fail(line != NULL);
   g_return_if_fail(x >= 0 && y >= 0);

   width = sc_console_get_width(cons);
   height= sc_console_get_height(cons);
   if(x >= width) {
      x = 0;
      ++y;
   }
   if(y >= height) return;

   minx = x;
   p = _sc_console_get_char_ptr(cons, x, y);
   g_return_if_fail(p != NULL);
   while(y < height && *line != '\0') {
      *p = *line;
      ++line;
      ++p;
      ++x;

      if(x >= width) {
         _sc_console_draw_region(cons, minx, y, x - minx, 1);
         minx = 0;
         x = 0;
         ++y;
      }
   }

   if(x > minx) {
      _sc_console_draw_region(cons, minx, y, x - minx, 1);
   }

}



/***  Console Cursors  ***/



static void _sc_console_lock_view_to_cursor(ScConsole *cons, ScConsoleCursor *cursor) {
/* sc_console_lock_view_to_cursor
   Forces the cursor to appear within the viewport by altering the viewport
   if necessary.  */

   gboolean needrewrite = FALSE;
   
   assert(IS_SC_CONSOLE(cons));
   assert(cursor != NULL);
   
   if(cursor->x < cons->text.viewx) {
      cons->text.viewx = cursor->x;
      _sc_console_draw_horiz_scroll(cons);
      needrewrite = TRUE;
   } else if(cursor->x + cursor->width > cons->text.viewx + cons->text.vieww) {
      cons->text.viewx = cursor->y - (cons->text.vieww - cursor->width);
      _sc_console_draw_horiz_scroll(cons);
      needrewrite = TRUE;
   }

   if(cursor->y < cons->text.viewy) {
      cons->text.viewy = cursor->y;
      _sc_console_draw_vert_scroll(cons);
      needrewrite = TRUE;
   } else if(cursor->y + cursor->height > cons->text.viewy + cons->text.viewh) {
      cons->text.viewy = cursor->y - (cons->text.viewh - cursor->height);
      _sc_console_draw_vert_scroll(cons);
      needrewrite = TRUE;
   }

   if(needrewrite) {
      _sc_console_draw_region(cons, 0, 0, sc_console_get_width(cons), sc_console_get_height(cons));
   }

}



void sc_console_set_cursor(ScConsole *cons, gint x, gint y, gint width, gint height) {
/* sc_console_set_cursor
   Updates all attributes of the cursor.  The cursor's position is given in
   (x,y) which are CHARACTER coordinates relative to the TEXT BUFFER (not
   the viewport).  The cursor's dimensions are also specified.  To make the
   cursor invisible, use width = height = 0.  The viewport may be altered to
   ensure that the cursor is visible within the viewport.  */

   ScConsoleCursor *cursor;   /* Pointer to the cursor structure */
   gint oldx;                 /* Cursor's original position X */
   gint oldy;                 /* Cursor's original position Y */
   gint oldw;                 /* Cursor's original width */
   gint oldh;                 /* Cursor's original height */

   g_return_if_fail(IS_SC_CONSOLE(cons));
   g_return_if_fail(width >= 0 && height >= 0);

   /* Get the cursor */
   cursor = &cons->cursor;

   /* Update the cursor attributes */
   oldx = cursor->x;
   oldy = cursor->y;
   oldw = cursor->width;
   oldh = cursor->height;
   cursor->x = x;
   cursor->y = y;
   cursor->width = width;
   cursor->height= height;

   /* Update the display.  Make sure the cursor is on the screen somewhere. */
   _sc_console_lock_view_to_cursor(cons, cursor);
   _sc_console_draw_region(cons, oldx, oldy, oldw, oldh);
   _sc_console_draw_region(cons, cursor->x, cursor->y, cursor->width, cursor->height);

}



void sc_console_set_cursor_pos(ScConsole *cons, gint x, gint y) {
/* sc_console_set_cursor_pos
   Modifies the cursor position to the CHARACTER coordinates (x,y) (which are
   relative to the TEXT BUFFER, not the viewport).  The cursor's width and
   height are left unaltered.  The viewport may be altered to ensure that the
   cursor is visible within the viewport.  */

   ScConsoleCursor *cursor;   /* Pointer to the cursor structure */
   gint oldx;                 /* Original cursor position X */
   gint oldy;                 /* Original cursor position Y */

   g_return_if_fail(IS_SC_CONSOLE(cons));

   /* Get the cursor */
   cursor = &cons->cursor;

   /* Update the position */
   oldx = cursor->x;
   oldy = cursor->y;
   cursor->x = x;
   cursor->y = y;

   /* Update the display.  Make sure the cursor is on the screen somewhere. */
   _sc_console_lock_view_to_cursor(cons, cursor);
   _sc_console_draw_region(cons, oldx, oldy, cursor->width, cursor->height);
   _sc_console_draw_region(cons, cursor->x, cursor->y, cursor->width, cursor->height);

}



void sc_console_set_cursor_highlighted(ScConsole *cons, gboolean highlighted) {
/* sc_console_set_cursor_highlighted
   Sets a flag indicating whether the cursor should appear "highlighted" or
   not.  The cursor is usually positioned on an active spot.  The highlighted
   flag determines whether the cursor is very bright, or just its normal
   colours.  */

   g_return_if_fail(IS_SC_CONSOLE(cons));

   if((highlighted && !cons->cursor.highlighted) || (!highlighted && cons->cursor.highlighted)) {
      cons->cursor.highlighted = highlighted;
      _sc_console_draw_region(cons,
                              sc_console_get_cursor_x(cons),
                              sc_console_get_cursor_y(cons),
                              sc_console_get_cursor_width(cons),
                              sc_console_get_cursor_height(cons));
   }

}



/***  Console Colours and Fonts  ***/



void sc_console_set_fonts(ScConsole *cons, GdkFont *font, GdkFont *boldfont) {
/* sc_console_set_fonts
   Update the fonts installed for this console.  Neither font pointer
   should be NULL.  The currently-installed fonts will be unreferenced,
   the new fonts will have their reference counts incremented by 1, and
   the entire console will be resized, repositioned, and redrawn.  */

   #if SC_GTK_DEBUG_GTK
      printf("sc_console_set_fonts:  installing new console fonts for %p:  %p %p\n", 
             (void *)cons, (void *)font, (void *)boldfont);
   #endif /* SC_GTK_DEBUG_GTK */

   g_return_if_fail(IS_SC_CONSOLE(cons));
   g_return_if_fail(font != NULL && boldfont != NULL);

   _sc_console_set_fonts(cons, font, boldfont);
   _sc_console_init_dimensions(cons);

   #if SC_GTK_DEBUG_GTK
      printf("sc_console_set_fonts:  finished installing new console fonts for %p\n", 
             (void *)cons);
   #endif /* SC_GTK_DEBUG_GTK */

}



void sc_console_set_colors(ScConsole *cons, GdkColor *fg, GdkColor *bg) {
/* sc_console_set_colors
   Modify the normal foreground/background colours for this console.
   This does not alter the colour attributes for highlights or the
   cursor.  If either colour is NULL, then the console's current colour
   is preserved.  */

   g_return_if_fail(IS_SC_CONSOLE(cons));

   if(fg != NULL) {
      cons->colors.foreground.red   = fg->red;
      cons->colors.foreground.green = fg->green;
      cons->colors.foreground.blue  = fg->blue;
   }
   if(bg != NULL) {
      cons->colors.background.red   = bg->red;
      cons->colors.background.green = bg->green;
      cons->colors.background.blue  = bg->blue;
   }
   cons->colors.colors_alloc = FALSE;
   cons->colors.bold = FALSE;
   _sc_console_draw_all(cons);

}



GdkColor *sc_console_get_color(ScConsole *cons, ScConsoleColorId rqst) {
/* sc_console_get_color
   Returns the GdkColor for the specified console attribute.  */

   g_return_val_if_fail(IS_SC_CONSOLE(cons), NULL);

   switch(rqst) {
   case SC_CONSOLE_FOREGROUND:
      return(&cons->colors.foreground);
   case SC_CONSOLE_BACKGROUND:
      return(&cons->colors.background);
   case SC_CONSOLE_FORECURSOR:
      return(&cons->colors.forecursor);
   case SC_CONSOLE_BACKCURSOR:
      return(&cons->colors.backcursor);
   case SC_CONSOLE_FORESHADOW:
      return(&cons->colors.foreshadow);
   case SC_CONSOLE_BACKSHADOW:
      return(&cons->colors.backshadow);
   case SC_CONSOLE_FORESCROLL:
      return(&cons->colors.forescroll);
   case SC_CONSOLE_BACKSCROLL:
      return(&cons->colors.backscroll);
   case SC_CONSOLE_FORELIGHT:
      return(&cons->colors.forelight);
   case SC_CONSOLE_BACKLIGHT:
      return(&cons->colors.backlight);
   case SC_CONSOLE_FOREDISABLED:
      return(&cons->colors.foredisabled);
   case SC_CONSOLE_BACKDISABLED:
      return(&cons->colors.backdisabled);
   case SC_CONSOLE_FORESTANDARD:
      return(&cons->colors.forestandard);
   case SC_CONSOLE_BACKSTANDARD:
      return(&cons->colors.backstandard);
   }
   return(NULL);

}



/***  Console Highlights  ***/



void sc_console_highlight_attach(ScConsole *cons, GdkColor *fg, GdkColor *bg, gboolean bold,
                                 gint x, gint y, gint width, gint height) {
/* sc_console_highlight_attach
   Attach a new highlight to the console, with the indicated colours and
   font weight, and indicated bounding box.  If either colour is NULL,
   then the default console text colours will be used.  Highlights are
   used to change the appearance of text in a particular region; they do
   not alter the text itself, however.
   
   The coordinates are relative to the TEXT BUFFER, not the viewport.  */

   ScConsoleHighlight *high;
   
   g_return_if_fail(IS_SC_CONSOLE(cons));

   high = (ScConsoleHighlight *)malloc(sizeof(ScConsoleHighlight));
   g_return_if_fail(high != NULL);

   if(fg != NULL) {
      high->colors.foreground.red   = fg->red;
      high->colors.foreground.green = fg->green;
      high->colors.foreground.blue  = fg->blue;
   } else {
      high->colors.foreground.red   = cons->colors.foreground.red;
      high->colors.foreground.green = cons->colors.foreground.green;
      high->colors.foreground.blue  = cons->colors.foreground.blue;
   }
   if(bg != NULL) {
      high->colors.background.red   = bg->red;
      high->colors.background.green = bg->green;
      high->colors.background.blue  = bg->blue;
   } else {
      high->colors.background.red   = cons->colors.background.red;
      high->colors.background.green = cons->colors.background.green;
      high->colors.background.blue  = cons->colors.background.blue;
   }
   high->colors.colors_alloc = FALSE;
   high->colors.bold = bold;

   high->x = x;
   high->y = y;
   high->width = width;
   high->height= height;

   cons->highlights = g_list_append(cons->highlights, high);
   _sc_console_draw_region(cons, high->x, high->y, high->width, high->height);

}



void sc_console_highlight_attach_disabled(ScConsole *cons, gint x, gint y, gint width, gint height) {
/* sc_console_highlight_attach_disabled
   Attach a "disabled" highlight to the console; one that follows
   the built-in disabled colour scheme, using a normal font (not
   bold).  The coordinates are CHARACTER coordinates, and are relative
   to the TEXT BUFFER, not the viewport.  */

   sc_console_highlight_attach(cons, &cons->colors.foredisabled, &cons->colors.backdisabled,
                               FALSE, x, y, width, height);

}



gboolean sc_console_highlight_detach(ScConsole *cons) {
/* sc_console_highlight_detach
   Remove the most recently attached highlight on this console.  
   Returns FALSE if there were no consoles available for deletion.  */

   ScConsoleHighlight *high;  /* Highlight to delete */
   GList *cur;                /* Linked list iterator */
   gint x;                    /* X position of highlight */
   gint y;                    /* Y position of highlight */
   gint width;                /* Width of highlight */
   gint height;               /* Height of highlight */
   
   g_return_val_if_fail(IS_SC_CONSOLE(cons), FALSE);

   /* Is there anything to delete? */
   cur = cons->highlights;
   if(cur == NULL) return(FALSE);

   /* Delete the last highlight */
   while(cur->next != NULL) cur = cur->next;
   high = cur->data;
   g_return_val_if_fail(high != NULL, FALSE);
   cur->data = NULL;

   /* Find out the extents of the highlight */
   x = high->x;
   y = high->y;
   width = high->width;
   height= high->height;

   /* Release the highlight */
   free(high);
   cons->highlights = g_list_remove_link(cons->highlights, cur);
   g_list_free(cur);

   /* Update display */
   _sc_console_draw_region(cons, x, y, width, height);
   return(TRUE);

}



void sc_console_highlight_detach_all(ScConsole *cons) {
/* sc_console_highlight_detach_all
   Remove all highlights from this console.  */

   assert(IS_SC_CONSOLE(cons));
   while(sc_console_highlight_detach(cons)) /* Just loop */;

}
