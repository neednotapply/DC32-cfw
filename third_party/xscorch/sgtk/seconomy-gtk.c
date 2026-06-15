/* $Header: /fridge/cvs/xscorch/sgtk/seconomy-gtk.c,v 1.17 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - seconomy-gtk.c   Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2003      Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Economy configuration dialogue


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
#include <assert.h>

#include <sgtk.h>
#include <sdialog.h>
#include <slabel.h>
#include <slinkcheck.h>
#include <slinkcombo.h>
#include <slinkspin.h>

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/seconomy.h>
#include <sgame/sregistry.h>
#include <snet/snet.h>



typedef struct _sc_economy_setup_data_gtk {
   sc_config *c;
   sc_economy_config *ec;
   double interestrate;
   bool dynamicinterest;
   int initialcash;
   bool computersbuy;
   bool computersaggressive;
   bool freemarket;
   bool lottery;
   int economyidx;
   const char **enames;
} sc_economy_setup_data_gtk;



static void _sc_economy_setup_apply_gtk(__libj_unused ScDialog *dlg,
                                        sc_economy_setup_data_gtk *setup) {

   sc_scoring_info *info;

   setup->ec->interestrate    = setup->interestrate;
   setup->ec->dynamicinterest = setup->dynamicinterest;
   setup->ec->initialcash     = setup->initialcash;
   setup->ec->computersbuy    = setup->computersbuy;
   setup->ec->computersaggressive = setup->computersaggressive;
   setup->ec->freemarket      = setup->freemarket;
   setup->ec->lottery         = setup->lottery;

   /* TEMP HACK - Well, this is ugly, maybe it should be done differently...
                  We could make it sane by reviving old interfaces in seconomy. -JL */

   info = sc_registry_find_first(setup->ec->registry, setup->ec->registryclass,
                                 SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
   for(; setup->economyidx > 0; --setup->economyidx)
      info = sc_registry_find_next(setup->ec->registry, setup->ec->registryclass, info->ident,
                                   SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);

   /* Set the new selected economy. */
   strcopyb(setup->ec->scoringname, info->name, SC_ECONOMY_MAX_NAME_LEN);

   #if USE_NETWORK
   if(SC_NETWORK_SERVER(setup->c)) sc_net_server_send_config(setup->c, setup->c->server);
   #endif

}



static int _sc_economy_init_names_gtk(sc_economy_setup_data_gtk *setup) {
/* _sc_economy_init_names_gtk
   Make a list of names, appropriate for the combo box. */

   int count, idx;
   sc_scoring_info *info, *target;

   /* Find the first economy listed. */
   info = sc_registry_find_first(setup->ec->registry, setup->ec->registryclass,
                                 SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);

   /* There must exist at least one economy scoring! */
   assert(info != NULL);

   /* Try to find the currently selected scoring so we can hilight it. */
   target = sc_scoring_lookup_by_name(setup->ec, setup->ec->scoringname);
   if(target == NULL) target = info;

   /* Build a list of economy names, and find the currently selected one. */
   count = idx = 0;
   setup->enames = NULL;
   while(info != NULL) {
      if(count % 15 == 0)
         setup->enames = (const char **)realloc(setup->enames, (count + 16) * sizeof(char *));
      setup->enames[count] = info->name;
      if(info->ident == target->ident)
         idx = count;
      ++count;
      info = sc_registry_find_next(setup->ec->registry, setup->ec->registryclass, info->ident,
                                   SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
   }

   /* The list should be null terminated. */
   if(count)
      setup->enames[count] = NULL;

   return(idx);

}



void sc_economy_setup_gtk(sc_window_gtk *w) {

   sc_economy_config *ec = w->c->economics;
   sc_economy_setup_data_gtk setup;
   ScDialog *dialog;
   int confirm = (SC_NETWORK_AUTH(w->c) ? SC_DIALOG_OK : 0);
   int row = 0;

   setup.c = w->c;
   setup.ec = ec;
   setup.interestrate   = ec->interestrate;
   setup.dynamicinterest= ec->dynamicinterest;
   setup.initialcash    = ec->initialcash;
   setup.computersbuy   = ec->computersbuy;
   setup.computersaggressive = ec->computersaggressive;
   setup.freemarket     = ec->freemarket;
   setup.lottery        = ec->lottery;
   setup.economyidx     = _sc_economy_init_names_gtk(&setup);

   dialog = SC_DIALOG(sc_dialog_new("Economy Setup", NULL, confirm | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_economy_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Scoring",              sc_link_combo_new(&setup.economyidx, setup.enames), &row);
   attach_option(dialog, w, "Initial Cash",         sc_link_spin_new(&setup.initialcash, 0, SC_ECONOMY_MAX_CASH, SC_ECONOMY_MAX_CASH / 100), &row);
   attach_option(dialog, w, "Interest Rate",        sc_link_spinf_new(&setup.interestrate, 0, SC_ECONOMY_MAX_INTEREST, 0.01), &row);
   attach_option(dialog, w, "Dynamic Interest",     sc_link_check_new(&setup.dynamicinterest), &row);
   attach_option(dialog, w, "AIs Can Buy",          sc_link_check_new(&setup.computersbuy), &row);
   attach_option(dialog, w, "AIs Buy Aggressively", sc_link_check_new(&setup.computersaggressive), &row);
   attach_option(dialog, w, "N/A:  Free Market",    sc_link_check_new(&setup.freemarket), &row);
   attach_option(dialog, w, "The Lottery",          sc_link_check_new(&setup.lottery), &row);

   sc_dialog_run(dialog);

}
