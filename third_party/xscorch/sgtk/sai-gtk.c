/* $Header: /fridge/cvs/xscorch/sgtk/sai-gtk.c,v 1.14 2009-04-26 17:39:47 jacob Exp $ */
/*
   
   xscorch - sai-gtk.c        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   AI controller configuration dialogue
    

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

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sai/sai.h>
#include <sgame/sconfig.h>
#include <snet/snet.h> 



typedef struct _sc_ai_controller_setup_data_gtk {
   sc_config *c;
   sc_ai_controller *aic;
   bool humantargets;
   bool allowoffsets;
   bool alwaysoffset;
   bool enablescan;
   bool nobudget;
} sc_ai_controller_setup_data_gtk;



static void _sc_ai_controller_setup_apply_gtk(__libj_unused ScDialog *dlg, sc_ai_controller_setup_data_gtk *setup) {

   sc_ai_controller *aic = setup->aic;
   
   aic->humantargets = setup->humantargets;
   aic->allowoffsets = setup->allowoffsets;
   aic->alwaysoffset = setup->alwaysoffset;
   aic->enablescan = setup->enablescan;
   aic->nobudget = setup->nobudget;
   
   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif
   
}



void sc_ai_controller_setup_gtk(sc_window_gtk *w) {

   sc_ai_controller *aic = w->c->aicontrol;
   sc_ai_controller_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK : 0);
   int row = 0;

   setup.c = w->c;
   setup.aic = aic;
   setup.humantargets = aic->humantargets;
   setup.allowoffsets = aic->allowoffsets;
   setup.alwaysoffset = aic->alwaysoffset;
   setup.enablescan = aic->enablescan;
   setup.nobudget = aic->nobudget;

   dialog = SC_DIALOG(sc_dialog_new("AI Controller Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_ai_controller_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Human Target Practice",  sc_link_check_new(&setup.humantargets), &row);
   attach_option(dialog, w, "Allow Offset Targetting",sc_link_check_new(&setup.allowoffsets), &row);
   attach_option(dialog, w, "Always Offset",          sc_link_check_new(&setup.alwaysoffset), &row);
   attach_option(dialog, w, "Enable Scan Refinement", sc_link_check_new(&setup.enablescan), &row);
   attach_option(dialog, w, "No Budget Constraints",  sc_link_check_new(&setup.nobudget), &row);

   sc_dialog_run(dialog);

}
