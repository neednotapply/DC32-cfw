/* $Header: /fridge/cvs/xscorch/sgtk/sweapons-gtk.c,v 1.16 2009-04-26 17:39:51 jacob Exp $ */
/*
   
   xscorch - sweapons-gtk.c   Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Weapon configuration dialogue
    

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
#include <slinkspin.h>

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/sweapon.h>
#include <snet/snet.h>



typedef struct _sc_weapons_setup_data_gtk {
   sc_config *c;
   sc_weapon_config *wpc;
   int armslevel;
   int bombicon;
   bool tunneling;
   double scaling;
   bool tracepaths;
   bool uselessitems;
} sc_weapons_setup_data_gtk;



static void _sc_weapons_setup_apply_gtk(__libj_unused ScDialog *dlg, sc_weapons_setup_data_gtk *setup) {

   sc_weapon_config *wpc = setup->wpc;

   wpc->armslevel    = setup->armslevel;
   wpc->bombiconsize = setup->bombicon;
   wpc->tunneling    = setup->tunneling;
   wpc->scaling      = setup->scaling;
   wpc->tracepaths   = setup->tracepaths;
   wpc->uselessitems = setup->uselessitems;
   
   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif

}



void sc_weapons_setup_gtk(sc_window_gtk *w) {

   sc_weapon_config *wpc = w->c->weapons;
   sc_weapons_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK : 0);
   int row = 0;

   setup.c = w->c;
   setup.wpc = wpc;
   setup.armslevel   = wpc->armslevel;
   setup.bombicon    = wpc->bombiconsize;
   setup.tunneling   = wpc->tunneling;
   setup.scaling     = wpc->scaling;
   setup.tracepaths  = wpc->tracepaths;
   setup.uselessitems= wpc->uselessitems;

   dialog = SC_DIALOG(sc_dialog_new("Weapon Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_weapons_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Arms Level",    sc_link_spin_new(&setup.armslevel, 0, SC_ARMS_LEVEL_MAX, 1), &row);
   attach_option(dialog, w, "Bomb Icon Size",sc_link_spin_new(&setup.bombicon, 0, SC_WEAPON_BOMB_ICON_MAX, 1), &row);
   attach_option(dialog, w, "Scaling",       sc_link_spinf_new(&setup.scaling, 0, SC_WEAPON_SCALING_MAX, 0.01), &row);
   attach_option(dialog, w, "Tunneling",     sc_link_check_new(&setup.tunneling), &row);
   attach_option(dialog, w, "Trace Paths",   sc_link_check_new(&setup.tracepaths), &row);
   attach_option(dialog, w, "Useless Items", sc_link_check_new(&setup.uselessitems), &row);

   sc_dialog_run(dialog);

}
