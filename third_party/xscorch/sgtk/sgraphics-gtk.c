/* $Header: /fridge/cvs/xscorch/sgtk/sgraphics-gtk.c,v 1.14 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - sgraphics-gtk.c  Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Graphics configuration dialogue
    

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
#include <slinkspin.h>

#include <sdialog-gtk.h>
#include <simage-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/scolor.h>
#include <sgame/sconfig.h>
#include <sgame/sland.h>
#include <snet/snet.h>



typedef struct _sc_graphics_setup_data_gtk {
   sc_config *c;
   int fieldwidth;
   int fieldheight;
   bool gfxdither;
   bool gfxanimate;
   bool gfxfast;
   bool gfxcompfast;
} sc_graphics_setup_data_gtk;



static void _sc_graphics_setup_apply_gtk(__libj_unused ScDialog *dlg,
                                         sc_graphics_setup_data_gtk *setup) {

   sc_config *c = setup->c;
   sc_window_gtk *w = (sc_window_gtk *)c->window;

   c->graphics.gfxanimate  = setup->gfxanimate;
   c->graphics.gfxfast     = setup->gfxfast;
   c->graphics.gfxcompfast = setup->gfxcompfast;

   if(c->fieldwidth != setup->fieldwidth || c->fieldheight != setup->fieldheight) {
      c->graphics.gfxdither = setup->gfxdither;
      c->fieldwidth  = setup->fieldwidth;
      c->fieldheight = setup->fieldheight;
      c->maxheight   = c->fieldheight - (SC_DEF_FIELD_HEIGHT - SC_DEF_MAX_HEIGHT);

      /* There went the landbuffer... */
      /* Colormap MUST be recalculated before land is regenerated! */
      sc_color_gradient_init(c, c->colors);

      /* Attempt to rebuild land */   
      sc_land_setup(c->land, c->fieldwidth, c->fieldheight, sc_land_flags(c));
      sc_land_generate(c, c->land);

      /*  We just seriously fsck'd things up  */
      
      /* Redraw everything */
      sc_window_resize(c->window);
      
   } else if(c->graphics.gfxdither != setup->gfxdither) {
      c->graphics.gfxdither = setup->gfxdither;

      /* Attempt to rebuild land */   
      sc_land_generate(c, c->land);
      sc_window_paint(c->window, 0, 0, c->land->width, c->land->height, SC_REGENERATE_LAND | SC_REDRAW_LAND);
      sc_pixmap_copy_gtk(sc_display_get_buffer(SC_DISPLAY(w->screen)), sc_display_get_gc(SC_DISPLAY(w->screen)), w->logo, w->logo_m, c->land->width - sc_pixmap_width_gtk(w->logo), c->land->height - sc_pixmap_height_gtk(w->logo));
   } /* Screen size was changed? */
   
   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif
   
}



void sc_graphics_setup_gtk(sc_window_gtk *w) {

   sc_config *c = w->c;
   sc_graphics_setup_data_gtk setup;
   ScDialog *dialog;
   GtkWidget *button;
   int confirm = (SC_NETWORK_AUTH(c) ? SC_DIALOG_OK | SC_DIALOG_APPLY : 0);
   int row = 0;

   setup.c = c;
   setup.fieldwidth  = c->fieldwidth;
   setup.fieldheight = c->fieldheight;
   setup.gfxdither   = c->graphics.gfxdither;
   setup.gfxanimate  = c->graphics.gfxanimate;
   setup.gfxfast     = c->graphics.gfxfast;
   setup.gfxcompfast = c->graphics.gfxcompfast;

   dialog = SC_DIALOG(sc_dialog_new("Graphics Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_graphics_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Screen Width",        sc_link_spin_new(&setup.fieldwidth, SC_MIN_FIELD_WIDTH, SC_MAX_FIELD_WIDTH, 8), &row);
   attach_option(dialog, w, "Screen Height",       sc_link_spin_new(&setup.fieldheight, SC_MIN_FIELD_HEIGHT, SC_MAX_FIELD_HEIGHT, 8), &row);
   attach_option(dialog, w, "Dithering",           sc_link_check_new(&setup.gfxdither), &row);
   attach_option(dialog, w, "Animation",           sc_link_check_new(&setup.gfxanimate), &row);
   attach_option(dialog, w, "Graphics Are Fast",   sc_link_check_new(&setup.gfxfast), &row);
   attach_option(dialog, w, "Computers Are Fast",  sc_link_check_new(&setup.gfxcompfast), &row);
   
   button = gtk_button_new_with_label(" Setup Fonts ");
   g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(sc_font_gtk), w);  /*swapped args for sc_font_gtk */
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_dialog_run(dialog);

}
