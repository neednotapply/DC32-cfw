/* $Header: /fridge/cvs/xscorch/sgtk/swindow-gtk.c,v 1.34 2011-07-31 19:05:20 jacob Exp $ */
/*

   xscorch - swindow-gtk.c    Copyright(c) 2000-2004 Justin David Smith
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
#define  __ALLOW_DEPRECATED_GDK__

/* Start with system includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Pick up xscorch GTK *first*, then pick up custom widgets */
#include <sgtk.h>
#include <sactiveconsole.h>
#include <sdialog.h>
#include <sdisplay.h>

/* Next, grab files for the xscorch interface components */
#include <scolor-gtk.h>
#include <sexplosion-gtk.h>
#include <simage-gtk.h>
#include <smenu-gtk.h>
#include <sstatus-gtk.h>
#include <stank-gtk.h>
#include <swindow-gtk.h>

/* Pick up xscorch game engine headers */
#include <sgame/sgame.h>
#include <sgame/splayer.h>
#include <sgame/sland.h>
#include <sgame/sstate.h>
#include <snet/snet.h>
#include <ssound/ssound.h>

/* Miscellaneous headers */
#include <gdk/gdkkeysyms.h>



static gint _sc_window_timeout_gtk(gpointer data) {

   sc_window_gtk *w = data;

   /* Execute next state in the state machine */
   sc_state_run(w->c, w->c->game);

   /* Run sound updates */
   #if USE_SOUND
      sc_sound_update(w->c->sound);
   #endif /* Sound? */

   /* Leave the timeout in place */
   return(TRUE);

}



void sc_window_timer_enable(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;

   w->timer_id = g_timeout_add(SC_SLEEP_TIME, _sc_window_timeout_gtk, w);

}



void sc_window_timer_disable(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;

   g_source_remove(w->timer_id);

}



static void _sc_screen_expose_gtk(__libj_unused GtkWidget *widget, __libj_unused GdkEvent *event, gpointer data) {

   sc_window_gtk *w = data;

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER_();
   #endif /* debug */

   /* Start game if this is our first drawing */
   if(!w->exposed && w->ready) {
      sc_window_timer_enable((sc_window *)w);
      w->exposed = TRUE;
   }

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_EXIT_();
   #endif /* debug */

}



static gint _sc_delete_event_gtk(__libj_unused GtkWidget *app, __libj_unused GdkEventAny *event, __libj_unused gpointer data) {

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER_();
   #endif /* debug */

   gtk_main_quit();

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_EXIT_();
   #endif /* debug */

   return(TRUE);

}



static gboolean _sc_window_keypress_gtk(GtkWidget *widget, GdkEventKey *key, gpointer data) {

   sc_window_gtk *w = data;
   sc_player *curplayer = w->c->plorder[w->c->game->curplayer];
   bool controlled = key->state & GDK_CONTROL_MASK;
   bool shifted = key->state & GDK_SHIFT_MASK;

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("%d (%s)  xx", key->keyval, gdk_keyval_name(key->keyval));
   #endif /* debug */

   /* ? */
   if(w->state < 4 && w->c->insanity &&
     !(SC_STATE_IS_ENABLED(w->c->game) &&
     !SC_STATE_IS_PAUSE(w->c->game))) {
      if(w->state == 0 && key->keyval == GDK_b)       ++w->state;
      else if(w->state == 1 && key->keyval == GDK_o)  ++w->state;
      else if(w->state == 2 && key->keyval == GDK_o)  ++w->state;
      else if(w->state == 3 && key->keyval == GDK_m)  ++w->state;
      else w->state = 0;
      if(w->state >= 4) {
         sc_status_message((sc_window *)w, "BOOM!  Heh heh heh...");
      }
   }

   /* NOTE: the g_signal_stop_emission_by_name() calls prevent GTK's
      focus manager from attempting to tab to other active controls
      on the screen; this prevents, f.e., the Tab key from simult.
      refocusing the controls and advancing weapons, which if you
      think about it really makes no sense. */

   /* Main game */
   if(SC_STATE_IS_ENABLED(w->c->game) && !SC_STATE_IS_PAUSE(w->c->game) && !controlled) {
      /* Make sure the current player is human */
      if(curplayer != NULL && curplayer->aitype == SC_AI_HUMAN) switch(key->keyval) {
         case GDK_Tab:
            sc_player_advance_weapon(w->c, curplayer, shifted ? -1 : 1);
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
         case GDK_Up:
         case GDK_KP_Up:
         case GDK_Page_Up:
            sc_player_advance_power(w->c, curplayer,
                                    shifted ? SC_PLAYER_POWER_STEP : SC_PLAYER_POWER_BIGSTEP);
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
         case GDK_Down:
         case GDK_KP_Down:
         case GDK_Page_Down:
            sc_player_advance_power(w->c, curplayer, 
                                    -(shifted ? SC_PLAYER_POWER_STEP : SC_PLAYER_POWER_BIGSTEP));
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
         case GDK_Right:
         case GDK_KP_Right:
            sc_player_advance_turret(w->c, curplayer,
                                     -(shifted ? SC_PLAYER_TURRET_STEP : SC_PLAYER_TURRET_BIGSTEP));
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
         case GDK_Left:
         case GDK_KP_Left:
            sc_player_advance_turret(w->c, curplayer,
                                     shifted ? SC_PLAYER_TURRET_STEP : SC_PLAYER_TURRET_BIGSTEP);
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
         case GDK_B:
         case GDK_b:
            sc_player_activate_battery(w->c, curplayer);
            return(TRUE);
         case GDK_E:
         case GDK_e:
            sc_player_activate_shield(w->c, curplayer);
            return(TRUE);
         case GDK_F:
         case GDK_f:
            sc_window_tank_move_gtk(w, curplayer);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_R:
         case GDK_r:
            sc_window_paint((sc_window *)w, 0, 0,
                            w->c->fieldwidth, w->c->fieldheight,
                            SC_PAINT_EVERYTHING);
            return(TRUE);
         case GDK_S:
         case GDK_s:
            sc_player_advance_shield(w->c, curplayer, SC_PLAYER_SHIELD_DEFAULTS);
            return(TRUE);
         case GDK_T:
         case GDK_t:
            sc_player_toggle_contact_triggers(w->c, curplayer);
            return(TRUE);
         case GDK_1:
         case GDK_KP_1:
            if(w->c->numplayers < 1) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[0]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_2:
         case GDK_KP_2:
            if(w->c->numplayers < 2) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[1]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_3:
         case GDK_KP_3:
            if(w->c->numplayers < 3) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[2]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_4:
         case GDK_KP_4:
            if(w->c->numplayers < 4) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[3]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_5:
         case GDK_KP_5:
            if(w->c->numplayers < 5) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[4]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_6:
         case GDK_KP_6:
            if(w->c->numplayers < 6) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[5]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_7:
         case GDK_KP_7:
            if(w->c->numplayers < 7) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[6]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_8:
         case GDK_KP_8:
            if(w->c->numplayers < 8) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[7]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_9:
         case GDK_KP_9:
            if(w->c->numplayers < 9) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[8]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_0:
         case GDK_KP_0:
            if(w->c->numplayers < 10) return(FALSE);
            sc_window_tank_info_gtk(w, w->c->players[9]);
            sc_game_pause(w->c, w->c->game);
            return(TRUE);
         case GDK_Return:
         case GDK_KP_Enter:
            sc_status_message((sc_window *)w, "");
            sc_game_set_state_asap(w->c->game, SC_STATE_TURN_PL_DONE);
            g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
            return(TRUE);
      }
   }

   return(FALSE);

}



static bool _sc_load_images(sc_window_gtk *w) {

   const char *filename;

   filename = SC_GLOBAL_DIR "/" SC_IMAGE_DIR "/xscorch-logo.xpm";
   w->logo = gdk_pixmap_colormap_create_from_xpm(w->app->window, NULL,
                                                 &w->logo_m, NULL,
                                                 filename);
   if(w->logo == NULL) {
      fprintf(stderr, "Cannot load \"%s\", aborting.\n", filename);
      return(false);
   }

   filename = SC_GLOBAL_DIR "/" SC_IMAGE_DIR "/xscorch-icon.xpm";
   w->icon = gdk_pixmap_colormap_create_from_xpm(w->app->window, NULL,
                                                 &w->icon_m, NULL,
                                                 filename);
   if(w->icon == NULL) {
      fprintf(stderr, "Cannot load \"%s\", aborting.\n", filename);
      return(false);
   }

   return(true);

}



sc_window *sc_window_new(sc_config *c, __libj_unused int argc, __libj_unused char **argv) {

   sc_window_gtk *w;
   GtkWidget *cont;
   GdkColor black;
   gint   fake_argc = 1;
   gchar* fake_argv[] = { "xscorch", NULL };
   char **fake_argv_p;

   /* initialise GTK */
   fake_argv_p = fake_argv;
   gtk_init(&fake_argc, &fake_argv_p);

   /* Initialise w */
   w = g_new0(sc_window_gtk, 1);
   c->window = (sc_window *)w;
   w->exposed = FALSE;
   w->ready = FALSE;
   w->statenabled = TRUE;
   w->chatbox = NULL;
   w->windarrowsize = 0;
   w->state = 0;
   w->c = c;

   /* Load the necessary fonts */
   sc_window_load_fonts(w);

   /* Create the window */
   w->app = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(w->app), "XScorch");
   g_signal_connect(G_OBJECT(w->app), "delete_event",
                    (GCallback)_sc_delete_event_gtk, w);
   gtk_window_set_resizable(GTK_WINDOW(w->app), FALSE);

   /* Create the main (vertical) container */
   cont = gtk_vbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(cont), 1);
   gtk_container_add(GTK_CONTAINER(w->app), cont);

   /* Create menus, etc */
   sc_window_create_menus_gtk(w);
   gtk_box_pack_start(GTK_BOX(cont), w->mainmenu, TRUE, TRUE, 0);

   /* Next comes the top statusbar */
   w->status = sc_window_active_console_new(w, 0, 0, 1, 1, CONSOLE_BORDERLESS);
   sc_status_configure_gtk(w);
   sc_status_setup((sc_window *)w);
   gtk_box_pack_start(GTK_BOX(cont), w->status, FALSE, TRUE, 1);

   /* Create drawable screen area */
   /*w->border = sc_display_new(w->c->fieldwidth + 4, w->c->fieldheight + 4);
   gtk_box_pack_start(GTK_BOX(cont), w->border, TRUE, TRUE, 0);*/
   w->screen = sc_display_new(w->c->fieldwidth, w->c->fieldheight);
   g_signal_connect(G_OBJECT(sc_display_get_drawbuf(SC_DISPLAY(w->screen))),
                    "expose_event", (GCallback)_sc_screen_expose_gtk, w);
   /*gtk_fixed_put(GTK_FIXED(w->border), w->screen, 2, 2);*/
   gtk_box_pack_start(GTK_BOX(cont), w->screen, TRUE, TRUE, 0);

   /* Please initialise the colormaps */
   w->colormap = sc_colormap_new_gtk();
   sc_colormap_alloc_colors_gtk(w);

   /* Show everything */
   gtk_widget_show_all(w->app);

   /* Setup display background color */
   gdk_color_black(gtk_widget_get_colormap(w->screen), &black);
   gdk_window_set_background(w->screen->window, &black);

   /* Setup the offscreen land buffer */
   w->landbuffer = gdk_pixmap_new(w->app->window, w->c->fieldwidth, w->c->fieldheight, -1);

   /* Setup explosion cache */
   w->explcache = sc_expl_cache_new_gtk();

   /* Load the images */
   if(!_sc_load_images(w)) return(NULL);
   gdk_window_set_icon(w->app->window, NULL, w->icon, w->icon_m);
   w->ready = TRUE;

   /* connect after loading, just to retain sanity. */
   g_signal_connect(G_OBJECT(w->app), "key_press_event",
                    (GCallback)_sc_window_keypress_gtk, w);

   /* Return the structure */
   return((sc_window *)w);

}



static inline void _sc_window_unref_land_buffer(sc_window_gtk *w) {

   if(w->landbuffer != NULL) {
      g_object_unref(w->landbuffer);
   }

}



void sc_window_free(sc_window **w_) {

   sc_window_gtk *w = (sc_window_gtk *)(*w_);

   _sc_window_unref_land_buffer(w);
   gtk_widget_destroy(w->app);
   w->c->window = NULL;
   sc_colormap_free_gtk(&w->colormap);
   sc_expl_cache_free_gtk(&w->explcache);

   sc_window_unload_fonts(w);

   free(w);
   *w_ = NULL;

}



void sc_window_run(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;

   if(w == NULL) return;
   gtk_main();
   return;

}



void sc_window_idle(sc_window *w_) {

   if(w_ == NULL) return;
   while (gtk_events_pending()) gtk_main_iteration();

}



void sc_window_message(__libj_unused sc_window *w_, const char *title, const char *msg) {

   sc_dialog_message(title, msg);

}



void sc_window_update(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;
   sc_window_update_menus_gtk(w);

}



void sc_window_resize(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;
   sc_config *c = w->c;

   /* There went the landbuffer... */
   _sc_window_unref_land_buffer(w);
   w->landbuffer = gdk_pixmap_new(w->app->window, c->fieldwidth, c->fieldheight, -1);

   /*  We just seriously fsck'd things up  */
   gtk_widget_set_size_request(sc_display_get_drawbuf(SC_DISPLAY(w->screen)),
                               c->fieldwidth, c->fieldheight);
   sc_status_setup(c->window);
   sc_window_idle(c->window);

   /* Redraw everything */
   sc_land_generate(c, c->land);
   sc_window_paint(c->window, 0, 0,
                   c->land->width, c->land->height,
                   SC_REGENERATE_LAND | SC_REDRAW_LAND);
   sc_pixmap_copy_gtk(sc_display_get_buffer(SC_DISPLAY(w->screen)),
                      sc_display_get_gc(SC_DISPLAY(w->screen)),
                      w->logo, w->logo_m,
                      c->land->width - sc_pixmap_width_gtk(w->logo),
                      c->land->height - sc_pixmap_height_gtk(w->logo));

}



GtkWidget *sc_window_console_new(sc_window_gtk *w, int x, int y, int width, int height, ScConsoleStyle style) {

   assert(w != NULL);
   return(sc_console_new(x, y, width, height, style, w->fixed_font, w->bold_fixed_font));

}



GtkWidget *sc_window_active_console_new(sc_window_gtk *w, int x, int y, int width, int height, ScConsoleStyle style) {

   assert(w != NULL);
   return(sc_active_console_new(x, y, width, height, style, w->fixed_font, w->bold_fixed_font));

}



void sc_window_console_set_fonts(sc_window_gtk *w, ScConsole *cons) {

   sc_console_set_fonts(cons, w->fixed_font, w->bold_fixed_font);

}



void sc_window_load_fonts(sc_window_gtk *w) {

   sc_config *c;

   assert(w != NULL);
   c = w->c;

   /* Load the necessary fonts */
   w->fixed_font = gdk_font_load(c->fixed_font);
   w->italic_fixed_font = gdk_font_load(c->italic_fixed_font);
   w->bold_fixed_font = gdk_font_load(c->bold_fixed_font);

   /* Make sure the fonts actually loaded */
   if(w->fixed_font == NULL) {
      printf("WARNING:  The font \"%s\" could not be loaded.  Trying \"fixed\"\n", c->fixed_font);
      w->fixed_font = gdk_font_load("fixed");
      if(w->fixed_font == NULL) {
         printf("ERROR:    Oh hell, you've got serious problems.  I couldn't find \"fixed\" either. Bailing out.\n");
         abort();
      }
   }
   if(w->italic_fixed_font == NULL) {
      printf("WARNING:  The font \"%s\" could not be loaded.  Trying \"%s\".  You won't have italic fonts.\n",
             c->italic_fixed_font, c->fixed_font);
      w->italic_fixed_font = w->fixed_font;
      gdk_font_ref(w->italic_fixed_font);
   }
   if(w->bold_fixed_font == NULL) {
      printf("WARNING:  The font \"%s\" could not be loaded.  Trying \"%s\".  You won't have bold fonts.\n",
             c->bold_fixed_font, c->fixed_font);
      w->bold_fixed_font = w->fixed_font;
      gdk_font_ref(w->bold_fixed_font);
   }

}



void sc_window_unload_fonts(sc_window_gtk *w) {

int i;
   assert(w != NULL);

   if(w->fixed_font != NULL)        gdk_font_unref(w->fixed_font);
   if(w->italic_fixed_font != NULL) gdk_font_unref(w->italic_fixed_font);
   if(w->bold_fixed_font != NULL)   gdk_font_unref(w->bold_fixed_font);

}



void sc_window_reload_fonts(sc_window_gtk *w) {

   sc_window_unload_fonts(w);
   sc_window_load_fonts(w);

   sc_window_console_set_fonts(w, SC_CONSOLE(w->status));
   sc_status_setup((sc_window *)w);
   sc_status_message((sc_window *)w, "BUG: Sure would be nice if the status bar was restored here...");

   sc_display_console_set_fonts(SC_DISPLAY(w->screen), w->fixed_font, w->bold_fixed_font);

}
