/* $Header: /fridge/cvs/xscorch/sgtk/smenu-gtk.c,v 1.23 2011-08-01 00:01:42 jacob Exp $ */
/*
   
   xscorch - smenu-gtk.c      Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c)      2009 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/
    
   GTK menus for xscorch
    

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
#include <stdio.h>
#include <stdlib.h>

#include <sgtk.h>
#include <sdialog.h>

#include <smenu-gtk.h>
#include <snet-gtk.h>
#include <ssetup-gtk.h>
#include <ssystem-gtk.h>

#include <sgame/scffile.h>
#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/shelpdata.h>
#include <sgame/sinfo.h>
#include <sgame/splayer.h>

#include <libj/jstr/libjstr.h>



static void _sc_action_game_restart_gtk(sc_window_gtk *w) {

   sc_game_pause(w->c, w->c->game);
   if(sc_dialog_query("Resign Game?", "Are you sure you want to RESIGN this game?")) {
      sc_config_init_game(w->c);
   }
   sc_game_unpause(w->c, w->c->game);

}



static void _sc_action_game_pause_gtk(sc_window_gtk *w) {

   sc_game_pause(w->c, w->c->game);
   sc_dialog_message("Game Paused", "Game paused.  Click `Ok' to unpause the game");
   sc_game_unpause(w->c, w->c->game);

}



static void _sc_action_game_exit_gtk(__libj_unused sc_window_gtk *w) {

   gtk_main_quit();
     
}



static void _sc_action_help_manual_gtk(sc_window_gtk *w) {

   sc_dialog_text_buffer("Xscorch manual", data_xscorch_txt, w->fixed_font, w->italic_fixed_font, w->bold_fixed_font);

}



static void _sc_action_help_license_gtk(sc_window_gtk *w) {

   sc_dialog_text_buffer("GNU General Public License", data_copying_txt, w->fixed_font, w->italic_fixed_font, w->bold_fixed_font);

}



static void _sc_action_help_about_gtk(__libj_unused sc_window_gtk *w) {

   sc_dialog_message("About XScorch", g_convert_with_fallback("XScorch version " VERSION "\n" 
                     SC_COPYRIGHT_NOTICE "\nContributors:\n" SC_CONTRIBUTORS_NOTICE, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL, NULL));

}



#if USE_NETWORK

static void _sc_action_network_server_gtk(sc_window_gtk *w) {

   sc_network_server_gtk(w);
   sc_window_update_menus_gtk(w);

}



static void _sc_action_network_client_gtk(sc_window_gtk *w) {

   sc_network_client_gtk(w);
   sc_window_update_menus_gtk(w);

}



static void _sc_action_network_disconnect_gtk(sc_window_gtk *w) {

   if(w->c->client != NULL) sc_net_client_free(&w->c->client, "User disconnected");
   if(w->c->server != NULL) sc_net_server_free(&w->c->server, "User disconnected");
   sc_window_update_menus_gtk(w);

}



static void _sc_action_network_status_gtk(sc_window_gtk *w) {

   char buf[SC_GTK_STRING_BUFFER];
   const char *title;
   sc_connection *conn;
   sc_net_status *status;
   sc_player *p;
   int i;

   if(w->c->client != NULL) {
      /* we have a connection; is it client or server? */
      if(w->c->server != NULL) {
         sbprintf(buf, sizeof(buf), "Running as server.  Other players:\n");
         for(i = 0; i < w->c->server->connections; ++i) {
            p = w->c->players[i];
            conn = &w->c->server->clients[i];
            sbprintf_concat(buf, sizeof(buf), "   %s (%s, %08x, %d, %d)\n", 
                            p->name, inet_ntoa(conn->address.sin_addr), conn->flags, 
                            p->turret, p->power);
         }
         title = "Server";
      } else {
         conn = &w->c->client->server;
         sbprintf(buf, sizeof(buf), "Connected to server %s as %s\n", 
                  inet_ntoa(conn->address.sin_addr), w->c->client->name);
         title = "Client";
      }
      
      /* dump the status information from the client */
      strconcatb(buf, "\nPlayer Status:\n", sizeof(buf));
      for(i = 0; i < w->c->numplayers; ++i) {
         p = w->c->players[i];
         status = &w->c->client->status[i];
         sbprintf_concat(buf, sizeof(buf), "   %s (%s, %08x, %08x, %08x, %08x)\n", 
                         p->name, inet_ntoa(status->address.sin_addr), 
                         status->cli_flags, status->cli_syncarg, 
                         status->srv_flags, status->srv_syncarg);
      }
      
      /* display the messagebox */
      sc_dialog_message(title, buf);
   } else {
      /* no connection is currently active. */
      sc_dialog_message("No connection", "No network connection\n");
   }

}

#endif /* USE_NETWORK */



void sc_window_update_menus_gtk(sc_window_gtk *w) {

   #if USE_NETWORK
      sc_config *c = w->c;

      gtk_widget_set_sensitive(w->net_server, TRUE);
      gtk_widget_set_sensitive(w->net_client, TRUE);
      gtk_widget_set_sensitive(w->net_disconnect, c->client != NULL || c->server != NULL);
      gtk_widget_set_sensitive(w->net_chat,       c->client != NULL || c->server != NULL);
      gtk_widget_set_sensitive(w->net_status,     c->client != NULL || c->server != NULL);
   #endif /* Network? */

}



void sc_window_create_menus_gtk(sc_window_gtk *w) {

   GError *gerr = NULL;
   GtkAction *action = NULL;
   GtkActionGroup *menu_action_group = gtk_action_group_new("menu");
   GtkUIManager *menu_ui_manager = gtk_ui_manager_new();

   /* OMG I HATE XML */

   char *main_menu_xml = "<ui> \
   <menubar name=\"MenuBar\"> \
      <menu action=\"MenuGame\"> \
         <menuitem action=\"Pause\" /> \
         <menuitem action=\"RestartGame\" /> \
         <separator /> \
         <menuitem action=\"SystemMenu\" /> \
         <menuitem action=\"FontSetup\" /> \
         <menuitem action=\"SoundSetup\" /> \
         <menuitem action=\"SaveConfig\" /> \
         <separator /> \
         <menuitem action=\"Exit\" /> \
      </menu> \
      <placeholder name=\"PlaceholderNetwork\" /> \
      <menu action=\"MenuHelp\"> \
         <menuitem action=\"Manual\" /> \
         <menuitem action=\"License\" /> \
         <menuitem action=\"About\" /> \
      </menu> \
   </menubar> \
</ui>";

   char *network_menu_xml = "<ui> \
   <menubar name=\"MenuBar\"> \
      <placeholder name=\"PlaceholderNetwork\" > \
         <menu action=\"MenuNetwork\"> \
            <menuitem action=\"CreateServer\" /> \
            <menuitem action=\"ConnectClient\" /> \
            <menuitem action=\"DisconnectClient\" /> \
            <separator /> \
            <menuitem action=\"Chat\" /> \
            <menuitem action=\"Status\" /> \
         </menu> \
      </placeholder> \
   </menubar> \
</ui>";

   /* OMG what was so simple in GTK 1.2 is this complex in GTK 2.12.  x_x  */
   gtk_action_group_add_action(menu_action_group, gtk_action_new("MenuGame", "_Game", NULL, NULL));

   action = gtk_action_new("Pause", "_Pause", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Pause", 'z', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Pause");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_game_pause_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("RestartGame", "_Restart Game", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Restart Game", 'r', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Restart Game");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_game_restart_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("SystemMenu", "S_ystem Menu", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/System Menu", 'y', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/System Menu");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(sc_system_menu_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("FontSetup", "_Font Setup ...", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Font Setup ...", 'f', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Font Setup ...");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(sc_font_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("SoundSetup", "S_ound Setup ...", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Sound Setup ...", 'o', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Sound Setup ...");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(sc_sound_setup_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("SaveConfig", "Sa_ve Configuration", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Save Configuration", 's', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Save Configuration");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(sc_config_file_save_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("Exit", "E_xit", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Game/Exit", 'x', GDK_CONTROL_MASK);
   gtk_action_set_accel_path(action, "<Xscorch>/Game/Exit");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_game_exit_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   gtk_action_group_add_action(menu_action_group, gtk_action_new("MenuHelp", "_Help", NULL, NULL));

   action = gtk_action_new("Manual", "_Manual page ...", NULL, NULL);
   gtk_accel_map_add_entry("<Xscorch>/Help/Manual page ...", gdk_keyval_from_name("F1"), 0);
   gtk_action_set_accel_path(action, "<Xscorch>/Help/Manual page ...");
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_help_manual_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("License", "_License ...", NULL, NULL);
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_help_license_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   action = gtk_action_new("About", "_About ...", NULL, NULL);
   g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_help_about_gtk), w);
   gtk_action_group_add_action(menu_action_group, action);
   g_object_unref(action);

   #if USE_NETWORK

      gtk_action_group_add_action(menu_action_group, gtk_action_new("MenuNetwork", "_Network", NULL, NULL));

      action = gtk_action_new("CreateServer", "Create _Server ...", NULL, NULL);
      g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_network_server_gtk), w);
      gtk_action_group_add_action(menu_action_group, action);
      g_object_unref(action);

      action = gtk_action_new("ConnectClient", "Connect as _Client ...", NULL, NULL);
      g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_network_client_gtk), w);
      gtk_action_group_add_action(menu_action_group, action);
      g_object_unref(action);

      action = gtk_action_new("DisconnectClient", "_Disconnect", NULL, NULL);
      g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_network_disconnect_gtk), w);
      gtk_action_group_add_action(menu_action_group, action);
      g_object_unref(action);

      action = gtk_action_new("Chat", "C_hat ...", NULL, NULL);
      g_signal_connect_swapped(action, "activate", G_CALLBACK(sc_chat_window_gtk), w);
      gtk_action_group_add_action(menu_action_group, action);
      g_object_unref(action);

      action = gtk_action_new("Status", "S_tatus ...", NULL, NULL);
      g_signal_connect_swapped(action, "activate", G_CALLBACK(_sc_action_network_status_gtk), w);
      gtk_action_group_add_action(menu_action_group, action);
      g_object_unref(action);

   #endif /* Network? */

   /* Make the new actions available to the menu items. */
   gtk_ui_manager_insert_action_group(menu_ui_manager, menu_action_group, 0);

   /* Turn XML crap into menu structures. */
   gtk_ui_manager_add_ui_from_string(menu_ui_manager, main_menu_xml, -1, &gerr);
   if (gerr != NULL) {
      fprintf(stderr, "Failed parsing main menu structure: \n%s\n", gerr->message);
      g_error_free(gerr);
   }

   #if USE_NETWORK
      gtk_ui_manager_add_ui_from_string(menu_ui_manager, network_menu_xml, -1, &gerr);
      if (gerr != NULL) {
         fprintf(stderr, "Failed parsing main menu structure: \n%s\n", gerr->message);
         g_error_free(gerr);
      }
      w->net_server     = gtk_ui_manager_get_widget(menu_ui_manager, "/ui/NetworkMenu/Create Server ...");
      w->net_client     = gtk_ui_manager_get_widget(menu_ui_manager, "/ui/NetworkMenu/Connect as Client ...");
      w->net_disconnect = gtk_ui_manager_get_widget(menu_ui_manager, "/ui/NetworkMenu/Disconnect");
      w->net_chat       = gtk_ui_manager_get_widget(menu_ui_manager, "/ui/NetworkMenu/Chat ...");
      w->net_status     = gtk_ui_manager_get_widget(menu_ui_manager, "/ui/NetworkMenu/Status ...");
   #endif /* Network? */

   /* Install the new menus. */
   gtk_window_add_accel_group(GTK_WINDOW(w->app), gtk_ui_manager_get_accel_group(menu_ui_manager));

   /* Finally we have something we can call a menu. */
   w->mainmenu = gtk_ui_manager_get_widget(menu_ui_manager, "/MenuBar");

   /* Do it now.  Yes really. */
   gtk_ui_manager_ensure_update(menu_ui_manager);

   /* I don't see a way to free the UI Manager, oh well... */

}
