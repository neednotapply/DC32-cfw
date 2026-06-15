/* $Header: /fridge/cvs/xscorch/sgtk/sinventory-gtk.c,v 1.46 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - sinventory-gtk.c Copyright(c) 2000-2004 Justin David Smith
                              Copyright(c) 2001-2004 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Player configuration dialogue


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
#include <sgtk.h>
#include <sactiveconsole.h>
#include <sconsole.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <swindow-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/sinventory.h>
#include <sgame/splayer.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



/* Alter this constant at risk of total annihilation. */
#define  SC_MIN_WINDOW_SIZE      10



typedef struct _sc_inventory_gtk {
   sc_window_gtk *w;             /* Backpointer to main window */
   sc_player *p;                 /* Inventory is for this player */
   GtkWidget *infopane;          /* Top panel, which has name */
   GtkWidget *weaponpane;        /* Weapons panel */
   GtkWidget *accessorypane;     /* Accessories panel */
   GtkWidget *invinfopane;       /* Bottom information panel */
} sc_inventory_gtk;



static void _sc_inventory_info_paint_gtk(sc_inventory_gtk *inv) {
/* sc_inventory_info_paint_gtk
   Paints the inventory window with player name, budget, etc. */

   char buf[SC_GTK_STRING_BUFFER];
   sbprintf(buf, sizeof(buf), "Inventory for player %-17s  $%-9d    Round #%-2d (%-2d remaining)",
            inv->p->name, inv->p->money, inv->w->c->curround + 2,
            inv->w->c->numrounds - inv->w->c->curround - 1);
   sc_console_write_line(SC_CONSOLE(inv->infopane), 0, 0, buf);

}



static void _sc_inventory_weapon_paint_gtk(sc_inventory_gtk *inv) {
/* sc_inventory_weapon_paint_gtk
   Paint the weapons window pane.  */

   int count;                       /* Number of weapons to display */
   int index;                       /* Current weapon index/iterator */
   sc_weapon_config *wc;            /* Weapon configuration data */
   const sc_weapon_info *info;      /* Data on the current weapon */
   char buf[SC_GTK_STRING_BUFFER];  /* Arbitrary text buffer */
   char less, great;                /* Can buy/sell indicators */

   /* Get the weapon lists */
   wc = inv->w->c->weapons;

   /* Deactivate any highlighting in the weapon panel */
   sc_console_highlight_detach_all(SC_CONSOLE(inv->weaponpane));

   /* Iterate through the list of weapons */
   count = sc_weapon_count(wc, SC_WEAPON_LIMIT_ALL);
   info = sc_weapon_first(wc, SC_WEAPON_LIMIT_ALL);
   for(index = 0; index < count; ++index) {
      /* Get data about this weapon */
      less = (sc_inventory_can_sell_weapon(inv->p, info) ? '<' : ' ');
      great = (sc_inventory_can_buy_weapon(inv->p, info, SC_INVENTORY_INFINITE) ? '>' : ' ');
      sbprintf(buf, sizeof(buf), " %c %-17s %2d/$%-7d  %2d %c",
               less, info->name, info->bundle, info->price,
               info->inventories[inv->p->index], great);

      /* Display the weapon data. If the player's budget currently
         does not allow them to buy or sell, or the inventory constraints
         kick in, then disable the item. */
      sc_console_write_line(SC_CONSOLE(inv->weaponpane), 0, index, buf);
      if(less == ' ' && great == ' ') {
         sc_console_highlight_attach_disabled(SC_CONSOLE(inv->weaponpane), 0, index,
                                              sc_console_get_width(SC_CONSOLE(inv->weaponpane)), 1);
      } /* Disabling the item? */
      info = sc_weapon_next(wc, info, SC_WEAPON_LIMIT_ALL);
   } /* Iterate through weapons */

}



static void _sc_inventory_accessory_paint_gtk(sc_inventory_gtk *inv) {
/* sc_inventory_accessory_paint_gtk
   Paints the accessories pane. */

   int count;                       /* Number of accessories to display */
   int index;                       /* Current accessory index/iterator */
   sc_accessory_config *ac;         /* Accessory configuration data */
   const sc_accessory_info *info;   /* Data on the current accessory */
   char buf[SC_GTK_STRING_BUFFER];  /* Arbitrary text buffer */
   char less, great;                /* Can buy/sell indicators */

   /* Get the accessory lists */
   ac = inv->w->c->accessories;

   /* Deactivate any highlighting in the accessory panel */
   sc_console_highlight_detach_all(SC_CONSOLE(inv->accessorypane));

   /* Iterate through the list of accessories */
   count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);
   for(index = 0; index < count; ++index) {
      /* Get data about this accessory */
      less = (sc_inventory_can_sell_accessory(inv->p, info) ? '<' : ' ');
      great = (sc_inventory_can_buy_accessory(inv->p, info, SC_INVENTORY_INFINITE) ? '>' : ' ');
      sbprintf(buf, sizeof(buf), " %c %-17s %2d/$%-7d  %2d %c",
               less, info->name, info->bundle, info->price,
               info->inventories[inv->p->index], great);

      /* Display the accessory data. If the player's budget currently
         does not allow them to buy or sell, or the inventory constraints
         kick in, then disable the item. */
      sc_console_write_line(SC_CONSOLE(inv->accessorypane), 0, index, buf);
      if(less == ' ' && great == ' ') {
         sc_console_highlight_attach_disabled(SC_CONSOLE(inv->accessorypane), 0, index,
                                              sc_console_get_width(SC_CONSOLE(inv->accessorypane)), 1);
      } /* Disabling the item? */
      info = sc_accessory_next(ac, info, SC_ACCESSORY_LIMIT_ALL);
   } /* Iterate through accessories */

}



static gboolean _sc_inventory_weapon_key_gtk(__libj_unused ScActiveConsole *cons,
                                             ScActiveConsoleSpot *spot,
                                             GdkEventKey *event, sc_inventory_gtk *inv) {
/* sc_inventory_weapon_key_gtk
   User hit a key in the weapons panel; process it and return
   TRUE if the key has been processed by this function. */

   sc_weapon_info *info = (sc_weapon_info *)spot->data;

   switch(event->keyval) {
      case GDK_Right:
      case GDK_KP_Right:
         sc_inventory_buy_weapon(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);

      case GDK_Left:
      case GDK_KP_Left:
         sc_inventory_sell_weapon(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);
   }

   return(FALSE);

}



static gboolean _sc_inventory_accessory_key_gtk(__libj_unused ScActiveConsole *cons,
                                                ScActiveConsoleSpot *spot,
                                                GdkEventKey *event, sc_inventory_gtk *inv) {
/* sc_inventory_accessory_key_gtk
   User hit a key in the accessory panel; process it and return
   TRUE if the key has been processed by this function. */

   sc_accessory_info *info = (sc_accessory_info *)spot->data;

   switch(event->keyval) {
      case GDK_Right:
      case GDK_KP_Right:
         sc_inventory_buy_accessory(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);

      case GDK_Left:
      case GDK_KP_Left:
         sc_inventory_sell_accessory(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);
   }

   return(FALSE);

}



static gboolean _sc_inventory_weapon_button_gtk(__libj_unused ScActiveConsole *cons,
                                                ScActiveConsoleSpot *spot,
                                                GdkEventButton *event, sc_inventory_gtk *inv) {
/* sc_inventory_weapon_button_gtk
   User clicked in the weapon panel; process it and return
   TRUE if the key has been processed by this function. */

   sc_weapon_info *info = (sc_weapon_info *)spot->data;

   /* Make sure this is a SINGLE click event */
   if(event->type != GDK_BUTTON_PRESS) return(FALSE);

   switch(event->button) {
      case 1:  /* Left mouse */
         sc_inventory_buy_weapon(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);

      case 3:  /* Right mouse */
         sc_inventory_sell_weapon(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);
   }

   return(FALSE);

}



static gboolean _sc_inventory_accessory_button_gtk(__libj_unused ScActiveConsole *cons,
                                                   ScActiveConsoleSpot *spot,
                                                   GdkEventButton *event, sc_inventory_gtk *inv) {
/* sc_inventory_accessory_button_gtk
   User clicked in the accessory panel; process it and return
   TRUE if the key has been processed by this function. */

   sc_accessory_info *info = (sc_accessory_info *)spot->data;

   /* Make sure this is a SINGLE click event */
   if(event->type != GDK_BUTTON_PRESS) return(FALSE);

   switch(event->button) {
      case 1:  /* Left mouse */
         sc_inventory_buy_accessory(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);

      case 3:  /* Right mouse */
         sc_inventory_sell_accessory(inv->p, info);
         _sc_inventory_info_paint_gtk(inv);
         _sc_inventory_weapon_paint_gtk(inv);
         _sc_inventory_accessory_paint_gtk(inv);
         return(TRUE);
   }

   return(FALSE);

}



static gboolean _sc_inventory_weapon_enter_gtk(__libj_unused ScActiveConsole *cons,
                                               ScActiveConsoleSpot *spot,
                                               sc_inventory_gtk *inv) {
/* sc_inventory_weapon_enter_gtk */

   char buf[SC_GTK_STRING_BUFFER];
   const sc_weapon_config *wc = inv->w->c->weapons;
   const sc_weapon_info *info = (const sc_weapon_info *)spot->data;

   /* Display its info */
   sc_weapon_info_line(wc, info, buf, sizeof(buf));
   sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 0, buf);
   if(info->description != NULL) {
      sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 1, info->description);
   }

   return(FALSE);

}



static gboolean _sc_inventory_accessory_enter_gtk(__libj_unused ScActiveConsole *cons,
                                                  ScActiveConsoleSpot *spot,
                                                  sc_inventory_gtk *inv) {
/* sc_inventory_accessory_enter_gtk */

   char buf[SC_GTK_STRING_BUFFER];
   const sc_accessory_config *ac = inv->w->c->accessories;
   const sc_accessory_info *info = (const sc_accessory_info *)spot->data;

   /* Display its info */
   sc_accessory_info_line(ac, info, buf, sizeof(buf));
   sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 0, buf);
   if(info->description != NULL) {
      sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 1, info->description);
   }

   return(FALSE);

}



static gboolean _sc_inventory_info_leave_gtk(__libj_unused ScActiveConsole *cons,
                                             __libj_unused ScActiveConsoleSpot *spot,
                                             sc_inventory_gtk *inv) {
/* sc_inventory_info_leave_gtk */

   /* Clear the info line */
   sc_console_clear_line(SC_CONSOLE(inv->invinfopane), 0);
   sc_console_clear_line(SC_CONSOLE(inv->invinfopane), 1);
   return(FALSE);

}



static gboolean _sc_inventory_continue_gtk(__libj_unused ScActiveConsole *cons,
                                           __libj_unused ScActiveConsoleSpot *spot,
                                           sc_inventory_gtk *inv) {
/* sc_inventory_continue_gtk
   User has closed the inventory window. */

   sc_status_message((sc_window *)inv->w, "");
   sc_display_console_detach_all(SC_DISPLAY(inv->w->screen));
   sc_game_set_state_asap(inv->w->c->game, SC_STATE_INVENTORY_PL_DONE);
   free(inv);
   return(TRUE);

}



static void _sc_inventory_create_gtk(sc_window_gtk *w, sc_player *p) {
/* sc_inventory_create_gtk
   Construct the inventory window for given player. */

   sc_inventory_gtk *inv;
   int windowheight;
   int i;

   /* Weapons */
   int wpcount;
   sc_weapon_config *wc = w->c->weapons;
   sc_weapon_info *wpinfo = sc_weapon_first(wc, SC_WEAPON_LIMIT_ALL);

   /* Accessories */
   int account;
   sc_accessory_config *ac = w->c->accessories;
   sc_accessory_info *acinfo = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);

   /* Compute the window size */
   windowheight = w->c->fieldheight / sc_console_get_row_height(SC_CONSOLE(w->status)) - 16;
   if(windowheight < SC_MIN_WINDOW_SIZE) windowheight = SC_MIN_WINDOW_SIZE;

   /* How many weapons/accessories, so we can decide height of panes? */
   wpcount = sc_weapon_count(w->c->weapons, SC_WEAPON_LIMIT_ALL);
   account = sc_accessory_count(w->c->accessories, SC_ACCESSORY_LIMIT_ALL);
   inv = (sc_inventory_gtk *)malloc(sizeof(sc_inventory_gtk));
   inv->w = w;
   inv->p = p;

   /* Construct the info, weapons, and accessories panels. */
   inv->weaponpane      = sc_window_active_console_new(w, 4, 7, 40, windowheight, CONSOLE_NORMAL);
   inv->accessorypane   = sc_window_active_console_new(w, 50, 7, 40, windowheight, CONSOLE_NORMAL);
   inv->infopane        = sc_window_active_console_new(w, 4, 2, 86, 2, CONSOLE_NORMAL);

   /* Weapons and accessories maintain offscreen buffers for scrolling */
   sc_console_buffer_size(SC_CONSOLE(inv->weaponpane), 40, wpcount);
   sc_console_buffer_size(SC_CONSOLE(inv->accessorypane), 40, account);

   /* Construct the bottom informational console. */
   inv->invinfopane = sc_window_console_new(w, 4, windowheight + 10, 86, 4, CONSOLE_NORMAL);
   sc_console_highlight_attach(SC_CONSOLE(inv->invinfopane), &w->colormap->white, NULL, FALSE,
                               0, 0, sc_console_get_width(SC_CONSOLE(inv->invinfopane)), 2);
   sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 2,
                         "Keyboard: [Right] buys a bundle, [Left] sells, and [Tab]/[Up]/[Down] navigate.");
   sc_console_write_line(SC_CONSOLE(inv->invinfopane), 0, 3,
                         "Mouse:  [Left click] buys, [Right click] sells.  Press [Enter] when you're done.");
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(inv->invinfopane));

   /* Hook up the various callbacks and add hotspots for
      each weapon which is in the panel. */
   g_signal_connect_after(G_OBJECT(inv->weaponpane), "key_press_spot",
                          (GCallback)_sc_inventory_weapon_key_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->weaponpane), "button_press_spot",
                          (GCallback)_sc_inventory_weapon_button_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->weaponpane), "enter_spot",
                          (GCallback)_sc_inventory_weapon_enter_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->weaponpane), "leave_spot",
                          (GCallback)_sc_inventory_info_leave_gtk, inv);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(inv->weaponpane));
   for(i = 0; i < wpcount; ++i) {
      sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(inv->weaponpane), i, wpinfo);
      wpinfo = sc_weapon_next(wc, wpinfo, SC_WEAPON_LIMIT_ALL);
   }

   /* Hook up the various callbacks and add hotspots for
      each accessory which is in the panel. */
   g_signal_connect_after(G_OBJECT(inv->accessorypane), "key_press_spot",
                          (GCallback)_sc_inventory_accessory_key_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->accessorypane), "button_press_spot",
                          (GCallback)_sc_inventory_accessory_button_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->accessorypane), "enter_spot",
                          (GCallback)_sc_inventory_accessory_enter_gtk, inv);
   g_signal_connect_after(G_OBJECT(inv->accessorypane), "leave_spot",
                          (GCallback)_sc_inventory_info_leave_gtk, inv);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(inv->accessorypane));
   for(i = 0; i < account; ++i) {
      sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(inv->accessorypane), i, acinfo);
      acinfo = sc_accessory_next(ac, acinfo, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Display the panes, and hookup miscellaneous signals. */
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(inv->infopane));
   sc_console_set_foreground(SC_CONSOLE(inv->infopane), &w->colormap->pcolors[p->index]);
   sc_console_write_line(SC_CONSOLE(inv->infopane), 70, 1, "< Continue >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(inv->infopane), 70, 1, 12, 1, NULL);
   g_signal_connect_after(G_OBJECT(inv->infopane), "select_spot",
                          (GCallback)_sc_inventory_continue_gtk, inv);

   /* Paint every pane. */
   _sc_inventory_weapon_paint_gtk(inv);
   _sc_inventory_accessory_paint_gtk(inv);
   _sc_inventory_info_paint_gtk(inv);

   /* Focus on the weapon pane */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(inv->infopane));
   gtk_widget_grab_focus(GTK_WIDGET(inv->weaponpane));

}



void sc_window_inventory(sc_window *w_, sc_player *p) {
/* sc_window_inventory
   Interface function for obtaining inventory (purchases and sales)
   from the specified player.  Usually we do all players in order,
   at the beginning of every round. */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   sc_display_console_detach_all(SC_DISPLAY(w->screen));
   _sc_inventory_create_gtk(w, p);

}
