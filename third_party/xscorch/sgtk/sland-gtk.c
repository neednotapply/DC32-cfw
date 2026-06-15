/* $Header: /fridge/cvs/xscorch/sgtk/sland-gtk.c,v 1.14 2009-04-26 17:39:49 jacob Exp $ */
/*
   
   xscorch - sland-gtk.c      Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Landscape configuration dialogue
    

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
#include <sdisplay.h>
#include <slabel.h>
#include <slinkcheck.h>
#include <slinkcombo.h>
#include <slinkspin.h>

#include <sdialog-gtk.h>
#include <simage-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/sland.h>
#include <snet/snet.h>



typedef struct _sc_land_setup_data_gtk {
   sc_config *c;
   sc_window_gtk *w;
   sc_land *land;
   int skyidx;
   bool hostileenv;
   int generatoridx;
   double bumpiness;
} sc_land_setup_data_gtk;



static void _sc_land_setup_apply_gtk(__libj_unused ScDialog *dlg, sc_land_setup_data_gtk *setup) {

   sc_land *land = setup->land;

   land->sky        = sc_land_sky_types()[setup->skyidx];
   land->hostileenv = setup->hostileenv;
   land->generator  = sc_land_generator_types()[setup->generatoridx];
   land->bumpiness  = setup->bumpiness;
   
   sc_land_generate(setup->w->c, land);
   sc_window_paint((sc_window *)setup->w, 0, 0, land->width, land->height, 
                   SC_REGENERATE_LAND | SC_REDRAW_LAND);
   sc_pixmap_copy_gtk(sc_display_get_buffer(SC_DISPLAY(setup->w->screen)), 
                      sc_display_get_gc(SC_DISPLAY(setup->w->screen)), 
                      setup->w->logo, setup->w->logo_m, 
                      land->width - sc_pixmap_width_gtk(setup->w->logo), 
                      land->height - sc_pixmap_height_gtk(setup->w->logo));
   
   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif
   
}



void sc_land_setup_gtk(sc_window_gtk *w) {

   sc_land *land = w->c->land;
   sc_land_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK | SC_DIALOG_APPLY : 0);
   int row = 0;

   setup.c = w->c;
   setup.w = w;
   setup.land = land;
   setup.hostileenv  = land->hostileenv;
   setup.generatoridx= 0;
   setup.bumpiness   = land->bumpiness;
   setup.skyidx      = 0;
   while(land->sky != sc_land_sky_types()[setup.skyidx]) ++setup.skyidx;
   while(land->generator != sc_land_generator_types()[setup.generatoridx]) ++setup.generatoridx;

   dialog = SC_DIALOG(sc_dialog_new("Landscape Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_land_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Land Generator",            sc_link_combo_new(&setup.generatoridx, sc_land_generator_names()), &row);
   attach_option(dialog, w, "Bumpiness",                 sc_link_spinf_new(&setup.bumpiness, 0, SC_LAND_BUMPINESS_MAX, 0.1), &row);
   attach_option(dialog, w, "Sky",                       sc_link_combo_new(&setup.skyidx, sc_land_sky_names()), &row);
   attach_option(dialog, w, "N/A:  Hostile Environment", sc_link_check_new(&setup.hostileenv), &row);

   sc_dialog_run(dialog);

}
