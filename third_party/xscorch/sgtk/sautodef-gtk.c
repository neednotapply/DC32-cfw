/* $Header: /fridge/cvs/xscorch/sgtk/sautodef-gtk.c,v 1.36 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - sautodef-gtk.c   Copyright(c) 2001-2003 Jacob Luna Lundberg
                              Copyright(c) 2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/

   Auto Defense settings screen


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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <sgtk.h>
#include <sactiveconsole.h>
#include <sactoggle.h>
#include <scolor-gtk.h>
#include <sdisplay.h>
#include <swindow-gtk.h>

#include <sai/sai.h>
#include <sgame/saccessory.h>
#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/splayer.h>
#include <sgame/spreround.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



/* list_sets are used to carry about item lists */
typedef struct _sc_list_set {
   int length;
   int *items;
} sc_list_set;



/*
 * We have to keep track of the player.
 * You MUST keep this sync'd with sc_auto_def_set in spreround.h!
 */
typedef struct _sc_auto_def_gtk {

   /* sc_auto_def_set copy */
   struct _sc_accessory_info *auto_guidance;    /* special guidance */
   struct _sc_accessory_info *auto_shield;      /* activate a shield */
   int chute_height;                            /* parachute threshold */
   bool triggers;                               /* use contact triggers */

   /* sc_auto_def_gtk only */
   GtkWidget *console_main;                     /* The primary display console */
   GtkWidget *console_guidance;                 /* The guidance list console */
   GtkWidget *console_shield;                   /* The shield list console */
   GtkWidget *console_parachute;                /* The parachute distance console */
   GtkWidget *console_triggers;                 /* The contact triggers console */
   GtkWidget *console_activate;                 /* The activation console */
   sc_list_set set_guidance;                    /* The items in the guidance list */
   sc_list_set set_shield;                      /* The items in the shield list */
   ScACToggle *trigger_toggle;                  /* The contact trigger toggle */
   sc_window_gtk *w;                            /* the parent window */
   sc_player *p;                                /* the controlling player */

   /* Keystroke preparedness */
   bool ready;					/* Until it's true, ignore keys */

} sc_auto_def_gtk;



sc_auto_def_gtk *_sc_auto_def_gtk_new(void) {
/* _sc_auto_def_gtk_new
   Set up a new auto_def_gtk */

   sc_auto_def_gtk *adg;

   adg = (sc_auto_def_gtk *)malloc(sizeof(sc_auto_def_gtk));
   if(adg == NULL) return(NULL);

   /* sc_auto_def_set items */
   adg->auto_guidance       = NULL;
   adg->auto_shield         = NULL;
   adg->chute_height        = 0;
   adg->triggers            = false;

   /* sc_auto_def_gtk items */
   adg->console_main        = NULL;
   adg->console_guidance    = NULL;
   adg->console_shield      = NULL;
   adg->console_parachute   = NULL;
   adg->console_triggers    = NULL;
   adg->console_activate    = NULL;
   adg->set_guidance.length = 0;
   adg->set_guidance.items  = NULL;
   adg->set_shield.length   = 0;
   adg->set_shield.items    = NULL;
   adg->trigger_toggle      = NULL;
   adg->w                   = NULL;
   adg->p                   = NULL;

   adg->ready               = false;

   return(adg);

}



void _sc_auto_def_gtk_free(sc_auto_def_gtk **adg) {
/* _sc_auto_def_gtk_free
   Free the memory in an sc_auto_def_gtk. */

   if(adg == NULL || *adg == NULL) return;

   free((*adg)->set_guidance.items);
   free((*adg)->set_shield.items);

   free(*adg);
   *adg = NULL;

}



inline void _sc_window_auto_def_engage(sc_auto_def_gtk *adg) {
/* _sc_window_auto_def_engage
   Engage the Auto Defense system with whatever settings we have. */

   /* Read the Contact Trigger toggle. */
   adg->triggers = sc_ac_toggle_get(adg->trigger_toggle) ? true : false;

   /* Activate the system. */
   sc_autodef_activate(adg->w->c, adg->p, (sc_auto_def_set *)adg);

}



static gboolean _sc_window_auto_def_close_gtk(__libj_unused GtkWidget *widget,
                                              __libj_unused ScActiveConsoleSpot *spot,
                                              sc_auto_def_gtk *adg) {
/* _sc_window_auto_def_close_gtk
   Enact the auto defense settings and restart the auto defense loop. */

   sc_window_gtk *w = adg->w;

   if(!adg->ready) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("accepting autodef close%s", "");
   #endif /* debug */

   /* Enact the user requests. */
   _sc_window_auto_def_engage(adg);

   /* Destroy the display and restart the auto defense player loop. */
   sc_display_console_detach_all(SC_DISPLAY(w->screen));
   sc_game_set_state_now(w->c, w->c->game, SC_STATE_AUTO_DEFENSE_LOOP);
   sc_status_message((sc_window *)w, "");
   _sc_auto_def_gtk_free(&adg);
   return(TRUE);

}



static gboolean _sc_window_auto_def_key_gtk(__libj_unused GtkWidget *widget,
                                            __libj_unused ScActiveConsoleSpot *spot,
                                            GdkEventKey *key, sc_auto_def_gtk *adg) {
/* _sc_window_auto_def_key_gtk
   Enact the auto defense settings and restart the auto defense loop. */

   sc_window_gtk *w = adg->w;

   if(!adg->ready) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("got autodef cancel (esc)%s", "");
   #endif /* debug */

   switch(key->keyval) {
      case GDK_Return:
      case GDK_KP_Enter:
         /* Push out the settings the user gave us. */
         _sc_window_auto_def_engage(adg);
      case GDK_Escape:
         /* Destroy the display and restart the auto defense player loop. */
         sc_display_console_detach_all(SC_DISPLAY(w->screen));
         sc_game_set_state_now(w->c, w->c->game, SC_STATE_AUTO_DEFENSE_LOOP);
         _sc_auto_def_gtk_free(&adg);
         return(TRUE);
   }
   return(FALSE);

}



static gboolean _sc_autodef_guidance_enter_gtk(__libj_unused ScActiveConsole *cons,
                                               ScActiveConsoleSpot *spot,
                                               sc_auto_def_gtk *adg) {
/* _sc_autodef_guidance_enter_gtk
   Move the selection to a new item in the list. */

   if(adg == NULL || spot == NULL) return(FALSE);

   if(!adg->ready) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("got autodef guidance entry%s", "");
   #endif /* debug */

   /* Perform the selection settings. */
   if(spot->y) {
      /* The spots are offset +1 to allow for the (none) selection. */
      adg->auto_guidance = sc_accessory_lookup(adg->w->c->accessories,
                                               adg->set_guidance.items[spot->y - 1],
                                               SC_ACCESSORY_LIMIT_ALL);
   } else {
      adg->auto_guidance = NULL;
   }

   return(TRUE);

}



static gboolean _sc_autodef_shield_enter_gtk(__libj_unused ScActiveConsole *cons,
                                             ScActiveConsoleSpot *spot,
                                             sc_auto_def_gtk *adg) {
/* _sc_autodef_shield_enter_gtk
   Move the selection to a new item in the list. */

   if(adg == NULL || spot == NULL) return(FALSE);

   if(!adg->ready) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("got autodef shield entry%s", "");
   #endif /* debug */

   /* Perform the selection settings. */
   if(spot->y) {
      /* The spots are offset +1 to allow for the (none) selection. */
      adg->auto_shield = sc_accessory_lookup(adg->w->c->accessories,
                                             adg->set_shield.items[spot->y - 1],
                                             SC_ACCESSORY_LIMIT_ALL);
   } else {
      adg->auto_shield = NULL;
   }

   return(TRUE);

}



void _sc_window_auto_defense_guidance_init(sc_auto_def_gtk *adg) {
/* _sc_window_auto_defense_guidance_init
   Draw up the console pane for Guidance System selection.
   There is allocation here so don't call it multiple times. */

   int count;
   char buf[SC_GTK_STRING_BUFFER];
   sc_window_gtk *w = adg->w;
   sc_accessory_config *ac = w->c->accessories;
   sc_accessory_info *info;

   /* Make a title on the main window. */
   sc_console_write_line(SC_CONSOLE(adg->console_main), 9, 3, "Guidance System");
   sc_console_highlight_attach(SC_CONSOLE(adg->console_main), 
                               sc_console_get_color(SC_CONSOLE(adg->console_main), SC_CONSOLE_FORESTANDARD), 
                               NULL, TRUE, 9, 3, 15, 1);

   /* Create storage space for the working set and set up for the search. */
   info = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);
   count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);
   adg->set_guidance.items = (int *)malloc(count * sizeof(int));
   adg->set_guidance.length = 0;
   /* Find the guidance systems we have in inventory. */
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_GUIDANCE(info) && info->inventories[adg->p->index] > 0) {
         /* We can allow them to activate this guidance system. */
         adg->set_guidance.items[adg->set_guidance.length++] = info->ident;
      }
      info = sc_accessory_next(ac, info, SC_ACCESSORY_LIMIT_ALL);
   }
   /* Shrink the memory to package just what we need. */
   adg->set_guidance.items = (int *)realloc(adg->set_guidance.items, adg->set_guidance.length * sizeof(int));

   /* Create the guidance system window. */
   adg->console_guidance = sc_window_active_console_new(w, 9, 9, 32, 3, CONSOLE_NORMAL);
   sc_console_buffer_size(SC_CONSOLE(adg->console_guidance), 32, adg->set_guidance.length + 1);

   /* Draw the ``nothing selected'' item and hook it up. */
   sbprintf(buf, sizeof(buf), "(none)");
   sc_console_write_line(SC_CONSOLE(adg->console_guidance), 0, 0, buf);
   sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(adg->console_guidance), 0, NULL);

   /* Draw the guidance system selections and hook them up. */
   for(count = 0; count < adg->set_guidance.length; ++count) {
      /* Generate a name tag for the item in question. */
      info = sc_accessory_lookup(ac, adg->set_guidance.items[count], SC_ACCESSORY_LIMIT_ALL);
      sbprintf(buf, sizeof(buf), "%s (%02i)", info->name, info->inventories[adg->p->index]);
      /* Display is offset +1 from index because of the ``(none)'' item. */
      sc_console_write_line(SC_CONSOLE(adg->console_guidance), 0, count + 1, buf);
      sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(adg->console_guidance), count + 1, NULL);
   }

   /*
    * Attach the guidance system signal handlers.  Right now there's
    * just enter_spot.  Can you think of others that would be useful?
    * I can't.  :)
    */
   g_signal_connect_after(G_OBJECT(adg->console_guidance), "enter-spot",
                          (GCallback)_sc_autodef_guidance_enter_gtk, adg);
   g_signal_connect_after(G_OBJECT(adg->console_guidance), "key_press_spot",
                          (GCallback)_sc_window_auto_def_key_gtk, adg);

   /* Display the guidance system window. */
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(adg->console_guidance));

}



void _sc_window_auto_defense_shield_init(sc_auto_def_gtk *adg) {
/* _sc_window_auto_defense_shield_init
   Draw up the console pane for Shield Selection.
   There is allocation here so don't call it multiple times. */

   int count;
   char buf[SC_GTK_STRING_BUFFER];
   sc_window_gtk *w = adg->w;
   sc_accessory_config *ac = w->c->accessories;
   sc_accessory_info *info;

   /* Make a title on the main window. */
   sc_console_write_line(SC_CONSOLE(adg->console_main), 45, 3, "Shield Selection");
   sc_console_highlight_attach(SC_CONSOLE(adg->console_main), 
                               sc_console_get_color(SC_CONSOLE(adg->console_main), SC_CONSOLE_FORESTANDARD), 
                               NULL, TRUE, 45, 3, 16, 1);

   /* Create storage space for the working set and set up for the search. */
   info = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);
   count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);
   adg->set_shield.items = (int *)malloc(count * sizeof(int));
   adg->set_shield.length = 0;
   /* Find the shields we have in inventory. */
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_SHIELD(info) && info->inventories[adg->p->index] > 0) {
         /* We can allow them to activate this shield. */
         adg->set_shield.items[adg->set_shield.length++] = info->ident;
      }
      info = sc_accessory_next(ac, info, SC_ACCESSORY_LIMIT_ALL);
   }
   /* Shrink the memory to package just what we need. */
   adg->set_shield.items = (int *)realloc(adg->set_shield.items, adg->set_shield.length * sizeof(int));

   /* Create the shield selection window. */
   adg->console_shield = sc_window_active_console_new(w, 45, 9, 32, 3, CONSOLE_NORMAL);
   sc_console_buffer_size(SC_CONSOLE(adg->console_shield), 32, adg->set_shield.length + 1);

   /* Draw the ``nothing selected'' item and hook it up. */
   sbprintf(buf, sizeof(buf), "(none)");
   sc_console_write_line(SC_CONSOLE(adg->console_shield), 0, 0, buf);
   sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(adg->console_shield), 0, NULL);

   /* Draw the shield selections and hook them up. */
   for(count = 0; count < adg->set_shield.length; ++count) {
      /* Generate a name tag for the item in question. */
      info = sc_accessory_lookup(ac, adg->set_shield.items[count], SC_ACCESSORY_LIMIT_ALL);
      sbprintf(buf, sizeof(buf), "%s (%02i)", info->name, info->inventories[adg->p->index]);
      /* Display is offset +1 from index because of the ``(none)'' item. */
      sc_console_write_line(SC_CONSOLE(adg->console_shield), 0, count + 1, buf);
      sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(adg->console_shield), count + 1, NULL);
   }

   /*
    * Attach the shield selection signal handlers.  Right now there's
    * just enter_spot.  Can you think of others that would be useful?
    * I can't.  :)
    */
   g_signal_connect_after(G_OBJECT(adg->console_shield), "enter-spot",
                          (GCallback)_sc_autodef_shield_enter_gtk, adg);
   g_signal_connect_after(G_OBJECT(adg->console_shield), "key_press_spot",
                          (GCallback)_sc_window_auto_def_key_gtk, adg);

   /* Display the shield selection window. */
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(adg->console_shield));

}



void _sc_window_auto_defense_parachute_init(sc_auto_def_gtk *adg) {
/* _sc_window_auto_defense_parachute_init
   Draw up the console pane for Parachute Configuration.
   There is allocation here so don't call it multiple times. */

   /*
    * TEMP - This is all supposed to be on its own active console.
    *        But to avoid focus issues, not until it's implemented.
    */

   sc_console_write_line(SC_CONSOLE(adg->console_main),
                         48, 12, "Parachutes");
   sc_console_highlight_attach(SC_CONSOLE(adg->console_main), 
                               sc_console_get_color(SC_CONSOLE(adg->console_main), SC_CONSOLE_FORESTANDARD), 
                               NULL, TRUE, 48, 12, 10, 1);

   /*
    * TEMP - We need something different here, probably like
    *        a selection dial or text box in nature. - JL
    *
    * The value to be adjusted here is: adg->chute_height
    */
   sc_console_write_line(SC_CONSOLE(adg->console_main),
                         47, 13, "Unimplemented");
   sc_console_highlight_attach_disabled(SC_CONSOLE(adg->console_main),
                                        47, 13, 13, 1);

}



void _sc_window_auto_defense_triggers_init(sc_auto_def_gtk *adg) {
/* _sc_window_auto_defense_triggers_init
   Draw up the console pane for Contact Trigger settings.
   There is allocation here so don't call it multiple times. */

   sc_window_gtk *w = adg->w;

   /* Create the contact triggers window. */
   adg->console_triggers = sc_window_active_console_new(w, 17, 16, 16, 5, CONSOLE_BORDERLESS);

   sc_console_write_line(SC_CONSOLE(adg->console_triggers),
                         0, 0, "Contact Triggers");
   sc_console_highlight_attach(SC_CONSOLE(adg->console_triggers),
                               NULL, NULL, TRUE,
                               0, 0, 16, 1);
   adg->trigger_toggle = SC_AC_TOGGLE(sc_ac_toggle_new(6, 1, 4, 2));
   sc_ac_toggle_set(adg->trigger_toggle, adg->triggers ? TRUE : FALSE);
   sc_active_console_add_gadget_spot(SC_ACTIVE_CONSOLE(adg->console_triggers),
                                     SC_GADGET(adg->trigger_toggle), NULL);

   /* Desensitize the toggle if the player can't use triggers. */
   if(!(adg->p->ac_state & SC_ACCESSORY_STATE_TRIGGER))
      gtk_widget_set_sensitive(GTK_WIDGET(adg->console_triggers), FALSE);

   /* Handle signals; most are handled natively by the toggle... */
   g_signal_connect_after(G_OBJECT(adg->console_triggers), "key_press_spot",
                          (GCallback)_sc_window_auto_def_key_gtk, adg);

   /* Display the contact triggers window. */
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(adg->console_triggers));

}



void sc_window_auto_defense(sc_window *w_, sc_player *p) {
/* sc_window_auto_defense
   Display the auto defense window. */

   sc_auto_def_gtk *adg;
   sc_window_gtk *w = (sc_window_gtk *)w_;
   char buf[SC_GTK_STRING_BUFFER];

   /* Put together the persistant information. */
   adg = _sc_auto_def_gtk_new();
   if(adg == NULL || w == NULL || p == NULL) {
      /* Terminating conditions! */
      free(adg);
      sc_game_set_state_now(w->c, w->c->game, SC_STATE_AUTO_DEFENSE_LOOP);
      return;
   }
   adg->w = w;
   adg->p = p;


   /*   H E A D E R   */

   /* Make a new window.                               x   y   w   h */
   adg->console_main = sc_window_active_console_new(w, 8,  4, 70, 20, CONSOLE_NORMAL);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(adg->console_main));
   sc_console_set_foreground(SC_CONSOLE(adg->console_main), &w->colormap->pcolors[p->index]);
   gtk_widget_set_sensitive(GTK_WIDGET(adg->console_main), FALSE);

   /* Put a highlighted title on the new window. */
   sbprintf(buf, sizeof(buf), "Tank Defense Controller for %s", p->name);
   sc_console_write_line(SC_CONSOLE(adg->console_main),
                         70 / 2 - 14 - strlenn(p->name) / 2, 0, buf);
   sc_console_highlight_attach(SC_CONSOLE(adg->console_main),
                               NULL, NULL, TRUE,
                               70 / 2 - 14 - strlenn(p->name) / 2, 0,
                               28 + strlenn(p->name), 1);


   /*   S E L E C T I O N S   */

   /* Display the guidance device selection. */
   adg->auto_guidance = NULL;
   _sc_window_auto_defense_guidance_init(adg);

   /* Display the shield selection. */
   adg->auto_shield = NULL;
   _sc_window_auto_defense_shield_init(adg);

   /* Display the parachute selection. */
   adg->chute_height = 8;
   _sc_window_auto_defense_parachute_init(adg);

   /* Display the contact trigger selection. */
   adg->triggers = p->contacttriggers;
   _sc_window_auto_defense_triggers_init(adg);


   /*   F O O T E R   */

   /* Float the exit instructions on their own console to fix null-focus crap. */
   adg->console_activate = sc_window_active_console_new(w, 8, 23, 70, 1, CONSOLE_BORDERLESS);

   /* Display the EXIT instructions. */
   sc_console_write_line(SC_CONSOLE(adg->console_activate), 0, 0,
                         "Press [Enter] to activate, or [Escape] to cancel");

   /* Set up the EXIT button. */
   sc_console_write_line(SC_CONSOLE(adg->console_activate), 58, 0, "< Activate >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(adg->console_activate), 58, 0, 12, 1, NULL);

   /* Connect the signal handlers for EXIT conditions. */
   g_signal_connect_after(G_OBJECT(adg->console_activate), "key_press_spot",
                          (GCallback)_sc_window_auto_def_key_gtk, adg);
   g_signal_connect_after(G_OBJECT(adg->console_activate), "select-spot",
                          (GCallback)_sc_window_auto_def_close_gtk, adg);

   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(adg->console_activate));

   /* Focus on this window. */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(adg->console_activate));
   /* I'd do console_main here but it gives focus to some null handler. */
   gtk_widget_grab_focus(GTK_WIDGET(adg->console_guidance));

   /* Everything is ready and running. */
   adg->ready = true;

}
