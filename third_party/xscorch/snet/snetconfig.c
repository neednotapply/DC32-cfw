/* $Header: /fridge/cvs/xscorch/snet/snetconfig.c,v 1.18 2011-08-01 00:01:43 jacob Exp $ */
/*

   xscorch - snetconfig.c     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Functions for handling the configuration data


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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <snetint.h>

#include <sai/sai.h>
#include <sgame/scolor.h>
#include <sgame/seconomy.h>
#include <sgame/sgame.h>
#include <sgame/sland.h>
#include <sgame/sphysics.h>
#include <sgame/splayer.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>
#include <sgame/swindow.h>
#include <sutil/srand.h>

#include <libj/jstr/libjstr.h>



#define  SC_NET_CONFIG_OPTIONS   0xFFFF0001
#define  SC_NET_CONFIG_GRAPHICS  0xFFFF0002
#define  SC_NET_CONFIG_ECONOMY   0xFFFF0004
#define  SC_NET_CONFIG_PHYSICS   0xFFFF0008
#define  SC_NET_CONFIG_WEAPONS   0xFFFF0010
#define  SC_NET_CONFIG_LAND      0xFFFF0020
#define  SC_NET_CONFIG_AIC       0xFFFF0040
#define  SC_NET_CONFIG_RUNTIME   0xFFFFF000



bool sc_net_svr_send_config(sc_config *c, sc_server *svr) {

   sc_packet config;    /* Data packet to transmit to server broadcast */
   sc_packet player;    /* player data to transmit to server broadcast */
   dword randseed;      /* Initial random seed for game */
   dword *p;            /* Pointer  into packet data */
   ubyte *pc;            /* Char ptr into packet data */
   int i;               /* Iterator through connections */
   int j;               /* Inner loop iterator */

   /* Sanity checks */
   if(c == NULL || svr == NULL) return(false);

   /* Obtain a new random seed */
   randseed = sys_rand();

   /*  B E G I N   C O N F I G   P A C K E T  */
   #define SC_PACKET_CONFIG_SIZE    (sizeof(dword) * 55)

   /* Construct the data area of config packet */
   if(!sc_net_packet_init(&config, SC_NET_SVR_CONFIG_DATA, SC_PACKET_CONFIG_SIZE)) return(false);
   p = (dword *)config.data;
   *p++ = htonl(SC_NET_CONFIG_OPTIONS);
   *p++ = htonl(c->options.mode);
   *p++ = htonl(c->options.team);
   *p++ = htonl(c->options.order);
   *p++ = htonl(c->options.talk);
   *p++ = htonl(c->options.talkprob);
   *p++ = htonl(c->options.interleave);

   *p++ = htonl(SC_NET_CONFIG_GRAPHICS);
   *p++ = htonl(c->graphics.gfxdither);
   *p++ = htonl(c->graphics.gfxanimate);
   *p++ = htonl(c->graphics.gfxfast);
   *p++ = htonl(c->graphics.gfxcompfast);
   *p++ = htonl(c->fieldwidth);
   *p++ = htonl(c->fieldheight);
   *p++ = htonl(c->maxheight);

   *p++ = htonl(SC_NET_CONFIG_ECONOMY);
   *p++ = htonl((dword)(c->economics->interestrate / 100 * DWORD_MAX));
   *p++ = htonl(c->economics->dynamicinterest);
   *p++ = htonl(c->economics->initialcash);
   *p++ = htonl(c->economics->computersbuy);
   *p++ = htonl(c->economics->computersaggressive);
   *p++ = htonl(c->economics->freemarket);
   *p++ = htonl(c->economics->lottery);
   { /* TEMP HACK - This is so evil I can't believe I'm doing it. */
      sc_scoring_info *info = sc_scoring_lookup_by_name(c->economics, c->economics->scoringname);
      assert(info != NULL);
      *p++ = htonl(info->ident);
   }

   *p++ = htonl(SC_NET_CONFIG_PHYSICS);
   *p++ = htonl((dword)(c->physics->airviscosity / SC_PHYSICS_VISCOUS_MAX * DWORD_MAX));
   *p++ = htonl((dword)(c->physics->gravity / SC_PHYSICS_GRAVITY_MAX * DWORD_MAX));
   *p++ = htonl((dword)(c->physics->damping / SC_TRAJ_DAMPING_MAX * DWORD_MAX));
   *p++ = htonl((dword)(c->physics->maxwind / SC_PHYSICS_WIND_MAX * DWORD_MAX));
   *p++ = htonl(c->physics->dynamicwind);
   *p++ = htonl(c->physics->suspenddirt);
   *p++ = htonl(c->physics->tanksfall);
   *p++ = htonl(c->physics->bordersextend);
   *p++ = htonl(c->physics->walls);

   *p++ = htonl(SC_NET_CONFIG_WEAPONS);
   *p++ = htonl(c->weapons->armslevel);
   *p++ = htonl(c->weapons->bombiconsize);
   *p++ = htonl(c->weapons->tunneling);
   *p++ = htonl((dword)(c->weapons->scaling / SC_WEAPON_SCALING_MAX * DWORD_MAX));
   *p++ = htonl(c->weapons->tracepaths);
   *p++ = htonl(c->weapons->uselessitems);

   *p++ = htonl(SC_NET_CONFIG_LAND);
   *p++ = htonl(c->land->sky);
   *p++ = htonl(c->land->hostileenv);
   *p++ = htonl(c->land->generator);
   *p++ = htonl((dword)(c->land->bumpiness / SC_LAND_BUMPINESS_MAX * DWORD_MAX));

   *p++ = htonl(SC_NET_CONFIG_AIC);
   *p++ = htonl(c->aicontrol->humantargets);
   *p++ = htonl(c->aicontrol->allowoffsets);
   *p++ = htonl(c->aicontrol->alwaysoffset);
   *p++ = htonl(c->aicontrol->enablescan);
   *p++ = htonl(c->aicontrol->nobudget);

   *p++ = htonl(SC_NET_CONFIG_RUNTIME);
   *p++ = htonl(randseed);
   *p++ = htonl(c->numplayers);
   if((char *)p - (char *)config.data != SC_PACKET_CONFIG_SIZE) {
      sc_net_set_error("svr_send_config", "Size mismatch on config packet");
      return(false);
   }

   /*  E N D   C O N F I G   P A C K E T  */


   /* Send data to each client connection */
   for(i = 0; i < svr->connections; ++i) {
      /* Queue config packet */
      sc_net_send_packet(&svr->clients[i], &config);

      /*  B E G I N   P L A Y E R   P A C K E T  */
      #define  SC_PACKET_PLAYER_SIZE      (SC_PLAYER_NAME_LENGTH + sizeof(dword))
      if(!sc_net_packet_init(&player, SC_NET_SVR_PLAYER_DATA, SC_PACKET_PLAYER_SIZE * SC_MAX_PLAYERS)) return(false);

      /* Setup player data packet */
      for(j = 0; j < SC_MAX_PLAYERS; ++j) {
         pc = player.data + SC_PACKET_PLAYER_SIZE * j;
         memcpy(pc, c->players[j]->name, SC_PLAYER_NAME_LENGTH);
         if(i == j) {
            *(dword *)(pc + SC_PLAYER_NAME_LENGTH) = htonl(SC_AI_HUMAN);
         } else if(c->players[j]->aitype == SC_AI_HUMAN || c->players[j]->aitype == SC_AI_NETWORK) {
            *(dword *)(pc + SC_PLAYER_NAME_LENGTH) = htonl(SC_AI_NETWORK);
         } else {
            *(dword *)(pc + SC_PLAYER_NAME_LENGTH) = htonl(c->players[j]->aitype);
         }
      }

      /*  E N D   P L A Y E R   P A C K E T  */

      /* Queue player data packet */
      sc_net_send_packet(&svr->clients[i], &player);
      sc_net_packet_release(&player);

      /* Flush the outgoing packet queue. */
      sc_net_flush_packets(&svr->clients[i]);
   }

   /* We're all done */
   sc_net_packet_release(&config);      
   sc_net_set_info("srv_send_config", "Sent game configuration");
   return(true);

}



bool sc_net_cli_recv_config(sc_config *c, sc_client *cli, sc_packet *packet) {

   char buffer[SC_NET_BUFFER_SIZE];   /* For info messages */
   dword *p;            /* Pointer to packet data */

   /* Sanity checks */
   if(c == NULL || cli == NULL || packet == NULL) return(false);
   if(!sc_net_check_size(packet, SC_PACKET_CONFIG_SIZE, "cli_recv_config")) return(false);

   /* Prepare to read data from config packet */
   p = (dword *)packet->data;

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_OPTIONS, "cli_recv_config", "options marker")) return(false);
   c->options.mode         = ntohl(*p++);
   c->options.team         = ntohl(*p++);
   c->options.order        = ntohl(*p++);
   c->options.talk         = ntohl(*p++);
   c->options.talkprob     = ntohl(*p++);
   c->options.interleave   = ntohl(*p++);

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_GRAPHICS, "cli_recv_config", "graphics marker")) return(false);
   c->graphics.gfxdither   = ntohl(*p++);
   c->graphics.gfxanimate  = ntohl(*p++);
   c->graphics.gfxfast     = ntohl(*p++);
   c->graphics.gfxcompfast = ntohl(*p++);
   c->fieldwidth           = ntohl(*p++);
   c->fieldheight          = ntohl(*p++);
   c->maxheight            = ntohl(*p++);

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_ECONOMY, "cli_recv_config", "economy marker")) return(false);
   c->economics->interestrate        = ((double)ntohl(*p++)) * 100 / DWORD_MAX;
   c->economics->dynamicinterest     = ntohl(*p++);
   c->economics->initialcash         = ntohl(*p++);
   c->economics->computersbuy        = ntohl(*p++);
   c->economics->computersaggressive = ntohl(*p++);
   c->economics->freemarket          = ntohl(*p++);
   c->economics->lottery             = ntohl(*p++);
   { /* TEMP HACK - More of the same evil.  But I know the eventual solution... */
      sc_scoring_info *info = sc_scoring_lookup(c->economics, ntohl(*p++));
      assert(info != NULL);
      strcopyb(c->economics->scoringname, info->name, SC_ECONOMY_MAX_NAME_LEN);
   }

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_PHYSICS, "cli_recv_config", "physics marker")) return(false);
   c->physics->airviscosity  = ((double)ntohl(*p++)) * SC_PHYSICS_VISCOUS_MAX / DWORD_MAX;
   c->physics->gravity       = ((double)ntohl(*p++)) * SC_PHYSICS_GRAVITY_MAX / DWORD_MAX;
   c->physics->damping       = ((double)ntohl(*p++)) * SC_TRAJ_DAMPING_MAX / DWORD_MAX;
   c->physics->maxwind       = ((double)ntohl(*p++)) * SC_PHYSICS_WIND_MAX / DWORD_MAX;
   c->physics->dynamicwind   = ntohl(*p++);
   c->physics->suspenddirt   = ntohl(*p++);
   c->physics->tanksfall     = ntohl(*p++);
   c->physics->bordersextend = ntohl(*p++);
   c->physics->walls         = ntohl(*p++);

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_WEAPONS, "cli_recv_config", "weapons marker")) return(false);
   c->weapons->armslevel    = ntohl(*p++);
   c->weapons->bombiconsize = ntohl(*p++);
   c->weapons->tunneling    = ntohl(*p++);
   c->weapons->scaling      = ((double)ntohl(*p++)) * SC_WEAPON_SCALING_MAX / DWORD_MAX;
   c->weapons->tracepaths   = ntohl(*p++);
   c->weapons->uselessitems = ntohl(*p++);

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_LAND, "cli_recv_config", "land marker")) return(false);
   c->land->sky        = ntohl(*p++);
   c->land->hostileenv = ntohl(*p++);
   c->land->generator  = ntohl(*p++);
   c->land->bumpiness  = ((double)ntohl(*p++)) * SC_LAND_BUMPINESS_MAX / DWORD_MAX;

   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_AIC, "cli_recv_config", "aic marker")) return(false);
   c->aicontrol->humantargets = ntohl(*p++);
   c->aicontrol->allowoffsets = ntohl(*p++);
   c->aicontrol->alwaysoffset = ntohl(*p++);
   c->aicontrol->enablescan   = ntohl(*p++);
   c->aicontrol->nobudget     = ntohl(*p++);

   /* Pick up runtime data */
   if(!sc_net_check_param(ntohl(*p++), SC_NET_CONFIG_RUNTIME, "cli_recv_config", "runtime marker")) return(false);
   sbprintf(buffer, sizeof(buffer), "Random seed is %08x", ntohl(*p));
   sc_net_set_info("cli_recv_config", buffer);
   game_randomize(ntohl(*p++));
   c->numplayers = ntohl(*p++);

   /*  B E G I N   C O D E   B A S E D   O N   G R A P H I C S   S E T U P  */

   /* Sloppily reconfigure everything on-the-fly */
   /* Colormap MUST be recalculated before land is regenerated! */
   sc_color_gradient_init(c, c->colors);

   /* Attempt to rebuild land */   
   sc_land_setup(c->land, c->fieldwidth, c->fieldheight, sc_land_flags(c));
   sc_land_generate(c, c->land);

   /* Resize window and redraw everything */
   sc_window_resize(c->window);

   /*  E N D   C O D E   B A S E D   O N   G R A P H I C S   S E T U P  */

   /* We are done */
   sc_net_set_info("cli_recv_config", "config structure received");
   SC_CONN_CLEAR_FLAGS(cli->server, SC_CONN_NEED_CONFIG);

   /* Return with success */
   return(true);

}



bool sc_net_cli_recv_players(sc_config *c, sc_client *cli, sc_packet *packet) {

   ubyte *p;            /* Pointer to packet data */
   int i;               /* Iterator variable */

   /* Sanity checks */
   if(c == NULL || cli == NULL || packet == NULL) return(false);
   if(!sc_net_check_size(packet, SC_PACKET_PLAYER_SIZE * SC_MAX_PLAYERS, "cli_recv_players")) return(false);

   /* Iterate through player data */
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      p = packet->data + SC_PACKET_PLAYER_SIZE * i;
      memcpy(c->players[i]->name, p, SC_PLAYER_NAME_LENGTH);
      c->players[i]->aitype = ntohl(*(dword *)(p + SC_PLAYER_NAME_LENGTH));
   }

   /* We are done */
   sc_net_set_info("cli_recv_players", "player structure received");
   SC_CONN_CLEAR_FLAGS(cli->server, SC_CONN_NEED_PLAYERS);

   return(true);

}



bool sc_net_server_send_config(sc_config *c, sc_server *srv) {

   return(sc_net_svr_send_config(c, srv));

}
