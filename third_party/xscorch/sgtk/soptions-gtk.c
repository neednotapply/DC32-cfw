/* $Header: /fridge/cvs/xscorch/sgtk/soptions-gtk.c,v 1.19 2011-07-31 20:06:43 jacob Exp $ */
/*
   
   xscorch - soptions-gtk.c   Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Generic options configuration dialogue
    

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
#include <slinkcheck.h>
#include <slinkcombo.h>
#include <slinkspin.h>

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/sconfig.h>
#include <snet/snet.h>



typedef struct _sc_options_setup_data_gtk {
   sc_config *c;
   sc_config_options *co;
   int modeidx;
   int teamidx;
   int orderidx;
   int talkidx;
   int talkprob;
   bool extstatus;
   bool tooltips;
   bool interleave;
} sc_options_setup_data_gtk;



static void _sc_options_setup_apply_gtk(__libj_unused ScDialog *dlg, sc_options_setup_data_gtk *setup) {

   sc_config_options *co = setup->co;
   sc_window_gtk *w = (sc_window_gtk *)setup->c->window;
   bool oldextstatus;
   bool oldtooltips;

   oldextstatus = co->extstatus;
   oldtooltips  = co->tooltips;
   co->mode     = sc_config_mode_types()[setup->modeidx];
   co->team     = sc_config_team_types()[setup->teamidx];
   co->order    = sc_config_order_types()[setup->orderidx];
   co->talk     = sc_config_talk_types()[setup->talkidx];
   co->talkprob = setup->talkprob;
   co->extstatus= setup->extstatus;
   co->tooltips = setup->tooltips;
   co->interleave=setup->interleave;
   
   /* Reconfigure the statusbar */
   if(oldextstatus != co->extstatus) sc_status_setup(setup->c->window);

   /* TEMP: I can't find a way to do this with GTK2...
    * Probably tooltips won't appear if they're off until you enable them and restart xscorch.
    *
   if(oldtooltips != co->tooltips) {
      if(co->tooltips) gtk_tooltips_enable(w);
      else             gtk_tooltips_disable(w);
   }
    */
      
   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif
   
}



void sc_options_setup_gtk(sc_window_gtk *w) {

   sc_config_options *co = &w->c->options;
   sc_options_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK : 0);
   int row = 0;

   setup.c = w->c;
   setup.co = co;
   setup.modeidx  = 0;
   setup.teamidx  = 0;
   setup.orderidx = 0;
   setup.talkidx  = 0;
   setup.talkprob = co->talkprob;
   setup.extstatus= co->extstatus;
   setup.tooltips = co->tooltips;
   setup.interleave=co->interleave;
   while(sc_config_mode_types()[setup.modeidx] != co->mode)  ++setup.modeidx;
   while(sc_config_team_types()[setup.teamidx] != co->team)  ++setup.teamidx;
   while(sc_config_order_types()[setup.orderidx]!=co->order)++setup.orderidx;
   while(sc_config_talk_types()[setup.talkidx] != co->talk)  ++setup.talkidx;

   dialog = SC_DIALOG(sc_dialog_new("Options Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_options_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Mode",                 sc_link_combo_new(&setup.modeidx, sc_config_mode_names()), &row);
   attach_option(dialog, w, "N/A:  Teams",          sc_link_combo_new(&setup.teamidx, sc_config_team_names()), &row);
   attach_option(dialog, w, "Order",                sc_link_combo_new(&setup.orderidx, sc_config_order_names()), &row);
   attach_option(dialog, w, "Talk Mode",            sc_link_combo_new(&setup.talkidx, sc_config_talk_names()), &row);
   attach_option(dialog, w, "Talk Probability",     sc_link_spin_new(&setup.talkprob, 0, 100, 1), &row);
   attach_option(dialog, w, "Extended Status",      sc_link_check_new(&setup.extstatus), &row);
   attach_option(dialog, w, "Tooltips",             sc_link_check_new(&setup.tooltips), &row);
   attach_option(dialog, w, "Interleaved Tracking", sc_link_check_new(&setup.interleave), &row);
   
   sc_dialog_run(dialog);

}
