/* $Header: /fridge/cvs/xscorch/sgtk/sphysics-gtk.c,v 1.14 2009-04-26 17:39:49 jacob Exp $ */
/*
   
   xscorch - sphysics-gtk.c   Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Physics configuration dialogue
    

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
#include <sgame/sphysics.h>
#include <snet/snet.h>



typedef struct _sc_physics_setup_data_gtk {
   sc_config *c;
   sc_physics *ph;
   double airviscosity;
   double gravity;
   double damping;
   double maxwind;
   bool dynamicwind;
   int suspenddirt;
   int tanksfall;
   int bordersextend;
   int wallidx;
} sc_physics_setup_data_gtk;



static void _sc_physics_setup_apply_gtk(__libj_unused ScDialog *dlg,
                                        sc_physics_setup_data_gtk *setup) {

   sc_physics *ph = setup->ph;
   
   ph->airviscosity = setup->airviscosity;
   ph->gravity      = setup->gravity;
   ph->damping      = setup->damping;
   ph->maxwind = setup->maxwind;
   ph->dynamicwind  = setup->dynamicwind;
   ph->suspenddirt  = setup->suspenddirt;
   ph->tanksfall    = setup->tanksfall;
   ph->bordersextend= setup->bordersextend;
   ph->walls        = sc_physics_wall_types()[setup->wallidx];

   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif
   
}



void sc_physics_setup_gtk(sc_window_gtk *w) {

   sc_physics *ph = w->c->physics;
   sc_physics_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK : 0);
   int row = 0;

   setup.c = w->c;
   setup.ph = ph;
   setup.airviscosity = ph->airviscosity;
   setup.gravity      = ph->gravity;
   setup.damping      = ph->damping;
   setup.maxwind      = ph->maxwind;
   setup.dynamicwind  = ph->dynamicwind;
   setup.suspenddirt  = ph->suspenddirt;
   setup.tanksfall    = ph->tanksfall;
   setup.bordersextend= ph->bordersextend;
   setup.wallidx      = 0;
   while(ph->walls != sc_physics_wall_types()[setup.wallidx]) ++setup.wallidx;

   dialog = SC_DIALOG(sc_dialog_new("Physics Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_physics_setup_apply_gtk, &setup);

   attach_option(dialog, w, "N/A:  Air Viscosity", sc_link_spinf_new(&setup.airviscosity, 0, SC_PHYSICS_VISCOUS_MAX, 0.001), &row);
   attach_option(dialog, w, "Gravity",             sc_link_spinf_new(&setup.gravity, 0, SC_PHYSICS_GRAVITY_MAX, 0.01), &row);
   attach_option(dialog, w, "Ground Damping",      sc_link_spinf_new(&setup.damping, 0, SC_TRAJ_DAMPING_MAX, 0.01), &row);
   attach_option(dialog, w, "N/A:  Suspend Dirt",  sc_link_spin_new(&setup.suspenddirt, 0, 100, 1), &row);
   attach_option(dialog, w, "N/A:  Tanks Fall",    sc_link_spin_new(&setup.tanksfall, 0, 100, 1), &row);
   attach_option(dialog, w, "Borders Extend",      sc_link_spin_new(&setup.bordersextend, 0, SC_PHYSICS_BORDERS_MAX, SC_PHYSICS_BORDERS_MAX / 100), &row);
   attach_option(dialog, w, "Walls Are",           sc_link_combo_new(&setup.wallidx, sc_physics_wall_names()), &row);
   attach_option(dialog, w, "Maximum Wind Speed",  sc_link_spinf_new(&setup.maxwind, 0, SC_PHYSICS_WIND_MAX, 0.01), &row);
   attach_option(dialog, w, "Wind is Dynamic",     sc_link_check_new(&setup.dynamicwind), &row);

   sc_dialog_run(dialog);

}
