/* $Header: /fridge/cvs/xscorch/sgtk/snet-gtk.c,v 1.18 2009-04-26 17:39:49 jacob Exp $ */
/*
   
   xscorch - snet-gtk.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Chat dialogue, network server/client connects
    

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
#include <sdialog.h>
#include <slabel.h>
#include <slinkentry.h>

#include <sdialog-gtk.h>
#include <snet-gtk.h>

#include <gdk/gdkkeysyms.h>

#if USE_NETWORK



#define  SC_PLAYER_HELP       "Specify the name you will connect to the server as; this will be the name of your player."
#define  SC_SERVER_HELP       "Specify the name or IP address of the server to connect to."
#define  SC_PORT_HELP         "Specify the port number on the server to connect to."



typedef struct _sc_chat {
   sc_config *c;
   GtkWidget *entry;
} sc_chat;


/* TEMP - Serious revisions needed in this file for GTK 2.0 support. */
#if GTK12_ENABLED

static void _sc_chat_window_delete(__libj_unused GtkWidget *window, sc_chat *config) {

   sc_window_gtk *w = (sc_window_gtk *)config->c->window;
   w->chatbox = NULL;

}


static void _sc_chat_send_gtk(__libj_unused GtkWidget *send, sc_chat *config) {

   if(config->c->client != NULL) {
      const char *text = gtk_entry_get_text(GTK_ENTRY(config->entry));
      if(strlenn(text) > 0) {
         sc_net_client_chat(config->c->client, text);
         gtk_entry_set_text(GTK_ENTRY(config->entry), "");
      }
   }

}


static gboolean _sc_chat_send_key_gtk(GtkWidget *text, GdkEventKey *key, sc_chat *config) {

   if(key->keyval == GDK_KP_Enter || key->keyval == GDK_Return) {
      _sc_chat_send_gtk(text, config);
      return(TRUE);
   }
   return(FALSE);

}


void sc_chat_window_update(sc_window *_w, const char *msg) {

   sc_window_gtk *w = (sc_window_gtk *)_w;
   if(w->chatbox == NULL) return;
   gtk_text_insert(GTK_TEXT(w->chatbox), NULL, NULL, NULL, msg, strlenn(msg));
   gtk_text_insert(GTK_TEXT(w->chatbox), NULL, NULL, NULL, "\n", 1);

}


void sc_chat_window_gtk(sc_window_gtk *w) {

   sc_chat config;
   char input[SC_NET_INPUT_SIZE];
   ScDialog *dialog;
   GtkWidget *widget;
   GtkWidget *view;
   
   input[0] = '\0';
   
   config.c = w->c;

   dialog = SC_DIALOG(sc_dialog_new("Chat", NULL, SC_DIALOG_CLOSE | SC_DIALOG_NONMODAL));
   g_signal_connect(G_OBJECT(dialog), "destroy",
                    (GCallback)_sc_chat_window_delete, &config);

   view = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_set_usize(view, 550, 100);
   gtk_table_attach(GTK_TABLE(dialog->grid), view, 0, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 2);

   widget = gtk_text_new(NULL, NULL);
   gtk_text_set_editable(GTK_TEXT(widget), FALSE);
   gtk_container_add(GTK_CONTAINER(view), widget);
   w->chatbox = widget;

   widget = sc_link_entry_new(input, SC_NET_INPUT_SIZE);
   gtk_widget_set_usize(widget, 450, 0);
   g_signal_connect(G_OBJECT(widget), "key-press-event",
                    (GCallback)_sc_chat_send_key_gtk, &config);
   gtk_table_attach(GTK_TABLE(dialog->grid), widget, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);
   config.entry = widget;

   widget = gtk_button_new_with_label(" Send ");
   g_signal_connect(G_OBJECT(widget), "clicked",
                    (GCallback)_sc_chat_send_gtk, &config);
   gtk_table_attach(GTK_TABLE(dialog->grid), widget, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 2, 2);

   sc_dialog_run(dialog);

}

#else /* GTK 2.0 */
/* TEMP - GtkTextView (GTK2) support not implemented yet for chat window. */

void sc_chat_window_update(sc_window *_w, const char *msg) {

   /* GtkTextView (GTK2) support not implemented yet for chat window. */

}


void sc_chat_window_gtk(sc_window_gtk *w) {

   /* GtkTextView (GTK2) support not implemented yet for chat window. */
   sc_dialog_message("Text viewer unimplemented", "Sorry, the GTK 2 text viewer has not been implemented yet.");

}
      
#endif /* GTK version? */


typedef struct _sc_server_config {
   sc_config *c;
   char name[SC_NET_NAME_SIZE];
   char port[SC_NET_INPUT_SIZE];
} sc_server_config;



static void _sc_network_server_apply_gtk(__libj_unused ScDialog *dlg, sc_server_config *config) {

   int portnum;
   
   if(config->c->server != NULL) {
      ScDialog *dlg = (ScDialog *)sc_dialog_new("Close Existing Server?",
                                                "Are you sure you want to close the existing server?",
                                                SC_DIALOG_YES | SC_DIALOG_NO);
      if(!sc_dialog_run(dlg)) return;
      sc_net_client_free(&config->c->client, "User creating new server");
      sc_net_server_free(&config->c->server, "User creating new server");
   } else if(config->c->client != NULL) {
      ScDialog *dlg = (ScDialog *)sc_dialog_new("Close Existing Connection?",
                                                "Are you sure you want to close the existing connection to a server?",
                                                SC_DIALOG_YES | SC_DIALOG_NO);
      if(!sc_dialog_run(dlg)) return;
      sc_net_client_free(&config->c->client, "User creating new server");
   }
   
   portnum = atoi(config->port);
   config->c->server = sc_net_server_new(config->c, portnum);
   if(config->c->server == NULL) {
      sc_dialog_message("Server failed", "Server failed");
   } else {
      config->c->client = sc_net_client_new(config->name, SC_NET_LOCALHOST, portnum);
      if(config->c->client == NULL) {
         sc_dialog_message("Client connection to server failed",
                           "Client connection to server failed");
      }
   }
   
}



void sc_network_server_gtk(sc_window_gtk *w) {

   sc_server_config config;
   ScDialog  *dialog;
   int row = 0;
   
   config.c = w->c;
   strcopyb(config.name, getenv("USER"), sizeof(config.name));
   sbprintf(config.port, sizeof(config.port), "%d", SC_NET_DEFAULT_PORT);
   
   dialog = SC_DIALOG(sc_dialog_new("Create server", NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_network_server_apply_gtk, &config);

   attach_option_help(dialog, w, "Player name",  SC_PLAYER_HELP, sc_link_entry_new(config.name, SC_NET_NAME_SIZE), &row);
   attach_option_help(dialog, w, "Listen on port", SC_PORT_HELP, sc_link_entry_new(config.port, SC_NET_INPUT_SIZE), &row);

   sc_dialog_run(dialog);

}



typedef struct _sc_client_config {
   sc_config *c;
   char name[SC_NET_NAME_SIZE];
   char port[SC_NET_INPUT_SIZE];
   char server[SC_NET_INPUT_SIZE];
} sc_client_config;



static void _sc_network_client_apply_gtk(__libj_unused ScDialog *dlg, sc_client_config *config) {

   int portnum;
   
   if(config->c->server != NULL) {
      ScDialog *dlg = (ScDialog *)sc_dialog_new("Close Existing Server?",
                                                "Are you sure you want to close the existing server?",
                                                SC_DIALOG_YES | SC_DIALOG_NO);
      if(!sc_dialog_run(dlg)) return;
      sc_net_client_free(&config->c->client, "User creating new connection");
      sc_net_server_free(&config->c->server, "User creating new connection");
   } else if(config->c->client != NULL) {
      ScDialog *dlg = (ScDialog *)sc_dialog_new("Close Existing Connection?",
                                                "Are you sure you want to close the existing connection to a server?",
                                                SC_DIALOG_YES | SC_DIALOG_NO);
      if(!sc_dialog_run(dlg)) return;
      sc_net_client_free(&config->c->client, "User creating new connection");
   }
   
   portnum = atoi(config->port);
   config->c->client = sc_net_client_new(config->name, config->server, portnum);
   if(config->c->client == NULL) {
      sc_dialog_message("Client failed", "Client failed");
   }

}



void sc_network_client_gtk(sc_window_gtk *w) {

   sc_client_config config;
   ScDialog  *dialog;
   int row = 0;

   config.c = w->c;   
   strcopyb(config.name, getenv("USER"), sizeof(config.name));
   sbprintf(config.port, sizeof(config.port), "%d", SC_NET_DEFAULT_PORT);
   strcopyb(config.server, SC_NET_DEFAULT_SERVER, sizeof(config.server));
   
   dialog = SC_DIALOG(sc_dialog_new("Connect as client", NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_network_client_apply_gtk, &config);

   attach_option_help(dialog, w, "Player name",  SC_PLAYER_HELP, sc_link_entry_new(config.name, SC_NET_NAME_SIZE), &row);
   attach_option_help(dialog, w, "Server name",  SC_SERVER_HELP, sc_link_entry_new(config.server, SC_NET_INPUT_SIZE), &row);
   attach_option_help(dialog, w, "Listen on port", SC_PORT_HELP, sc_link_entry_new(config.port, SC_NET_INPUT_SIZE), &row);

   sc_dialog_run(dialog);

}



#endif /* Network? */
