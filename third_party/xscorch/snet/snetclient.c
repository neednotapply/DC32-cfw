/* $Header: /fridge/cvs/xscorch/snet/snetclient.c,v 1.28 2011-08-01 00:01:43 jacob Exp $ */
/*

   xscorch - snetclient.c     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Client control loop


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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <snetint.h>

#include <sgame/saccessory.h>
#include <sgame/sgame.h>
#include <sgame/sinventory.h>
#include <sgame/splayer.h>
#include <sgame/sshield.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>
#include <sgame/swindow.h>
#include <sutil/srand.h>

#include <libj/jstr/libjstr.h>



static bool _sc_net_client_send_name(sc_client *cli) {
/* _sc_net_client_send_name
   Send the name of this player to the server. This packet is usually sent
   out immediately after the ACCEPTED packet is received from the server,
   when the player connects to the game.  This code, fortunately, is very
   straightforward. Returns true on success.  */

   sc_packet packet;       /* Data packet to send */

   /* Construct a simple name packet */
   if(!sc_net_packet_init( &packet, 
                           SC_NET_CLI_PLAYER_NAME, 
                           SC_PLAYER_NAME_LENGTH )) return(false);
   memcpy(packet.data, cli->name, SC_PLAYER_NAME_LENGTH);

   /* Attempt to send the packet */
   sc_net_set_info("cli_send_name", "Sending our name to the server");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



static void _sc_net_cli_missync(sc_config *c) {
/* _sc_net_cli_missync
   Call this function if a missync is detected in what the server has sent
   us. We will eventually try to do error recovery here, but for now all
   we can do is display this solemn error message. */

   sc_window_message( c->window, "Network Error", 
                      "This client is no longer synchronized with the main game server. "\
                      "This could cause serious problems with the network game in progress. "\
                      "Unfortunately, there is no recovery code at this time, so it's pretty "\
                      "much over. Sorry..." );

}



static bool _sc_net_cli_recv_sync(sc_config *c, sc_client *cli, sc_packet *packet) {
/* _sc_net_cli_recv_sync

   See sc_net_client_sync() for an overview of the process. This is the tail
   end to a synchronization from the client to the current game event. On
   success, the server will respond to us when all players have reached that
   event, telling us we may now continue with simulation or turn. The server
   will also remind us what TYPE of sync event we were going through.  This
   will normally agree with the type of sync event we sent in to the server.

   The server will also send us a copy of its current random value.  This
   should agree with the next random value we expect to get -- if it does
   then we are okay. If it does not, then something has gone horribly wrong. 
   And I do mean horribly. If that happens, we will have to try to negotiate
   with the server to see what went wrong.

   Otherwise, we may now continue with the game. This function returns true
   if everything worked properly. */

   dword randval;          /* Random value sanity check */
   dword *p;               /* Pointer into packet data */
   bool missync = false;   /* True if we are out of sync */

   /* Check data size, etc. */
   if(!sc_net_check_size( packet, 
                          2 * sizeof(dword), 
                          "cli_recv_sync" )) return(false);

   /* Update flags, sync was successfully processed */
   SC_CONN_CLEAR_FLAGS(cli->server, SC_CONN_WAIT_SYNC);
   sc_net_set_info("cli_recv_sync", "Received sync authorization from server");

   /* Check that the sync message is what we expected */
   p = (dword *)packet->data;
   if(ntohl(*p++) != cli->server.syncarg) {
      sc_net_set_error("cli_recv_sync", "Sync for wrong event received");
      missync = true;
   }

   /* Check that random values are still in sync */
   randval = ntohl(*p++);
   if(randval != game_rand_peek()) {
      sc_net_set_error("cli_recv_sync", "Random values are OUT OF ALIGNMENT!!!");
      missync = true;
   }

   if(missync) _sc_net_cli_missync(c);
   return(true);

}



static bool _sc_net_cli_recv_orders(sc_config *c, sc_packet *packet) {
/* _sc_net_cli_recv_orders
   Receiving orders from another player in the game. We are given our
   client data structure, and their packet (as relayed from the server).
   We are generally expected to comply with their orders for their tank
   but we will not accept server orders for OUR tank -- we will only 
   verify the orders sent to us for our tank. Returns true on success */

   sc_player *pl;          /* Current player structure */
   dword *p;               /* Pointer into packet data */
   dword x;                /* X variable */
   dword y;                /* Y variable */
   int i;                  /* Iterator */

   /* Check size and sequence numbers */
   if(!sc_net_check_size( packet, 
                          (1 + 7 * c->numplayers) * sizeof(dword), 
                          "cli_recv_orders" )) return(false);

   /* Check that the total player count is accurate */
   p = (dword *)packet->data;
   if(!sc_net_check_param( c->numplayers, 
                           ntohl(*p++), 
                           "cli_recv_orders", 
                           "player count" )) return(false);

   /* Update each player, in turn */
   for(i = 0; i < c->numplayers; ++i) {
      pl = c->players[i];
      if(!sc_net_check_param( i, ntohl(*p++), 
                              "cli_recv_orders", 
                              "marker" )) return(false);
      if(pl->aitype == SC_AI_NETWORK) {
         /* Setting params for a network player */
         sc_player_set_turret(c, pl, ntohl(*p++));
         sc_player_set_power (c, pl, ntohl(*p++));
         sc_player_set_weapon(c, pl, sc_weapon_lookup(c->weapons, ntohl(*p++), SC_WEAPON_LIMIT_NONE));
         sc_player_set_shield(c, pl, sc_accessory_lookup(c->accessories, ntohl(*p++), SC_ACCESSORY_LIMIT_NONE));
         x = ntohl(*p++);
         y = ntohl(*p++);
         sc_player_set_position(c, pl, x, y);
      } else {
         /* Verifying orders for a local player -- note, it is possible we 
            are receiving an old (but valid) orders packet here -- which is
            possible given network latencies if the player is rapidly enter
            new orders. Therefore no action is taken currently if one of
            these assertions fails. */
         sc_net_check_param(pl->turret,           ntohl(*p++), "cli_recv_orders", "turret");
         sc_net_check_param(pl->power,            ntohl(*p++), "cli_recv_orders", "power");
         sc_net_check_param(pl->selweapon->ident, ntohl(*p++), "cli_recv_orders", "weapon");
         sc_net_check_param(pl->selshield->ident, ntohl(*p++), "cli_recv_orders", "shield");
         sc_net_check_param(pl->x,                ntohl(*p++), "cli_recv_orders", "player x");
         sc_net_check_param(pl->y,                ntohl(*p++), "cli_recv_orders", "player y");
      } /* Modifying network or verifying params? */
   } /* Loop through all players */

   /* Return with success */
   sc_net_set_info("cli_recv_orders", "Received orders from server");
   return(true);

}



static bool _sc_net_cli_recv_shields(sc_config *c, sc_packet *packet) {
/* _sc_net_cli_recv_shields
   Similar to the orders packet, except we are receiving a player request
   to activate their shields. Again, returns true on success. */

   sc_accessory_info *info;/* Shield requested for activation */
   sc_player *pl;          /* Player structure */
   sdword playerid;        /* Player ID to shield */
   dword *p;               /* counter */

   /* Check the packet size */
   if(!sc_net_check_size( packet, 
                          2 * sizeof(dword), 
                          "cli_recv_shields" )) return(false);
   p = (dword *)packet->data;

   /* Get the player ID */
   playerid = ntohl(*p++);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("cli_recv_shields", "Invalid playerid received");
      return(false);
   }
   pl = c->players[playerid];

   /* Test that the shield is in inventory */
   info = sc_accessory_lookup(c->accessories, ntohl(*p++), SC_ACCESSORY_LIMIT_NONE);
   if(info == NULL || info->inventories[playerid] <= 0) {
      sc_net_set_error("cli_recv_shields", "Shield to activate not in inventory");
      return(false);
   }

   /* Activate shields if needed */
   if(pl->aitype == SC_AI_NETWORK) {
      /* Setting params for a network player */
      sc_player_activate_shield(c, pl);
   }

   /* Return with success */
   sc_net_set_info("cli_recv_shields", "Received shield status from server");
   return(true);

}



static bool _sc_net_cli_recv_battery(sc_config *c, sc_packet *packet) {
/* _sc_net_cli_recv_battery
   Similar to the orders packet, except we are receiving a player request
   to activate one of their batteries. Again, returns true on success. */

   sc_player *pl;          /* Player structure */
   sdword playerid;        /* Player ID to shield */

   /* Check the packet size */
   if(!sc_net_check_size( packet, 
                          sizeof(dword), 
                          "cli_recv_batterys" )) return(false);

   /* Get the player ID */
   playerid = ntohl(*(dword *)packet->data);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("cli_recv_battery", "Invalid playerid received");
      return(false);
   }
   pl = c->players[playerid];

   /* Activate batteries if needed */
   if(pl->aitype == SC_AI_NETWORK) {
      /* Setting params for a network player */
      sc_player_activate_battery(c, pl);
   }

   /* Return with success */
   sc_net_set_info("cli_recv_battery", "Received battery command from server");
   return(true);

}



static bool _sc_net_cli_recv_flags(sc_config *c, sc_packet *packet) {
/* _sc_net_cli_recv_flags
   Similar to the orders packet, except we are receiving a player's
   set of command flags.  Again, returns true on success. */

   sc_player *pl;          /* Player structure */
   sdword playerid;        /* Player ID to shield */
   dword *p;               /* Packet data pointer */

   /* Check the packet size */
   if(!sc_net_check_size( packet, 
                          3 * sizeof(dword), 
                          "cli_recv_flags" )) return(false);

   /* Get the player ID */
   p = (dword *)packet->data;
   playerid = ntohl(*p++);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("cli_recv_flags", "Invalid playerid received");
      return(false);
   }
   pl = c->players[playerid];

   /* Get command flags */
   if(pl->aitype == SC_AI_NETWORK) {
      /* Setting params for a network player */
      pl->contacttriggers = ntohl(*p++);
      pl->ac_state        = ntohl(*p++);
   } else {
      /* Verify configuration */
      sc_net_check_param(pl->contacttriggers, ntohl(*p++), "cli_recv_flags", "contact triggers");
      sc_net_check_param(pl->ac_state,        ntohl(*p++), "cli_recv_flags", "accessory state flags");
   } /* Local or remote? */

   /* Return with success */
   sc_net_set_info("cli_recv_flags", "Received command flags from server");
   return(true);

}



/* The following markers are used in an inventory packet */
#define  SC_PACKET_INVENTORY_WEAPONS      0xEEEE0001
#define  SC_PACKET_INVENTORY_ACCESSORIES  0xEEEE0002
#define  SC_PACKET_INVENTORY_MISC         0xEEEE0003



static bool _sc_net_cli_recv_inventory(sc_config *c, sc_packet *packet) {

   sc_accessory_info *ai;  /* A temp pointer into info structs */
   sc_weapon_info *wi;     /* A temp pointer into info structs */
   sc_player *pl;          /* Player structure */
   sdword playerid;        /* Player ident */
   dword *p;               /* Pointer into packet data */
   int acount;             /* Accessory count (total) */
   int wcount;             /* Weapon count (total) */
   int size;               /* Section size parametre */
   int i;                  /* Iterator */

   /* Make sure size is large enough to contain the player ID 
      and individual sizes for each section (3 dwords) */
   if(!sc_net_check_size(packet, 3 * sizeof(dword), "cli_recv_inventory")) return(false);

   /* Get the player ID and section sizes */
   p = (dword *)packet->data;
   playerid = ntohl(*p++);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("cli_recv_inventory", "Invalid playerid received");
      return(false);
   }
   pl = c->players[playerid];
   acount = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_NONE);
   wcount = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_NONE);
   if(!sc_net_check_param(ntohl(*p++), wcount, "cli_recv_inventory", "weapon count")) return(false);
   if(!sc_net_check_param(ntohl(*p++), acount, "cli_recv_inventory", "accessory count")) return(false);

   /* Make sure packet is large enough to hold additional data.
      Packets for weapons and accessories look like this:
      [ integer item_id ], [ integer item_count ]  (repeat) */
   size = (8 + 2 * (wcount + acount)) * sizeof(dword);
   if(!sc_net_check_size(packet, size, "cli_recv_inventory")) return(false);

   /* Process weapons in packet */
   if(!sc_net_check_param(ntohl(*p++), SC_PACKET_INVENTORY_WEAPONS, "cli_recv_inventory", "weapon marker")) return(false);
   size = wcount;
   if(pl->aitype == SC_AI_NETWORK) {
      /* Setting weapons for a network player */
      for(i = 0; i < size; ++i) {
         wi = sc_weapon_lookup(c->weapons, ntohl(*p++), SC_WEAPON_LIMIT_NONE);
         if(wi == NULL) {
            sc_net_set_error("cli_recv_player_state", "setting inventory of nonexistant weapon");
            return(false);
         } else {
            wi->inventories[playerid] = ntohl(*p++);
         }
      } /* Loop through weapons */
   } else {
      /* Verifying weapons for a local player */
      for(i = 0; i < size; ++i) {
         wi = sc_weapon_lookup(c->weapons, ntohl(*p++), SC_WEAPON_LIMIT_NONE);
         if(wi == NULL) {
            sc_net_set_error("cli_recv_player_state", "checking inventory of nonexistant weapon");
            return(false);
         } else {
            if(!sc_net_check_param(ntohl(*p++), wi->inventories[playerid], "cli_recv_inventory", "weapon"))
               return(false);
         }
      } /* Loop through weapons */
   } /* Network or local player? */

   /* Process accessories in packet */
   if(!sc_net_check_param(ntohl(*p++), SC_PACKET_INVENTORY_ACCESSORIES, "cli_recv_inventory", "accessory marker")) return(false);
   size = acount;
   if(pl->aitype == SC_AI_NETWORK) {
      /* Setting accessories for a network player */
      for(i = 0; i < size; ++i) {
         ai = sc_accessory_lookup(c->accessories, ntohl(*p++), SC_ACCESSORY_LIMIT_NONE);
         if(ai == NULL) {
            sc_net_set_error("cli_recv_player_state", "setting inventory of nonexistant accessory");
            return(false);
         } else {
            ai->inventories[playerid] = ntohl(*p++);
         }
      } /* Loop through accessories */
   } else {
      /* Verifying accessories for a local player */
      for(i = 0; i < size; ++i) {
         ai = sc_accessory_lookup(c->accessories, ntohl(*p++), SC_ACCESSORY_LIMIT_NONE);
         if(ai == NULL) {
            sc_net_set_error("cli_recv_player_state", "checking inventory of nonexistant accessory");
            return(false);
         } else {
            if(!sc_net_check_param(ntohl(*p++), ai->inventories[playerid], "cli_recv_inventory", "accessory"))
               return(false);
         }
      } /* Loop through accessories */
   } /* Network or local player? */

   /* Process miscellaneous section */
   if(!sc_net_check_param(ntohl(*p++), SC_PACKET_INVENTORY_MISC, "cli_recv_inventory", "misc marker")) return(false);
   if(pl->aitype == SC_AI_NETWORK) {
      pl->money = ntohl(*p++);
      pl->oldmoney = ntohl(*p++);
   } else {
      sc_net_check_param(ntohl(*p++), pl->money,    "cli_recv_inventory", "misc: money");
      sc_net_check_param(ntohl(*p++), pl->oldmoney, "cli_recv_inventory", "misc: oldmoney");
   }

   /* All went well */
   sc_net_set_info("cli_recv_inventory", "Received inventory from server");
   return(true);

}



static bool _sc_net_cli_recv_player_state(sc_config *c, sc_packet *packet) {

   sc_player *pl;          /* Player data structure */
   dword *p;               /* Pointer into packet data */
   sdword x;               /* Miscellaneous variable X */
   sdword y;               /* Miscellaneous variable Y */
   bool b;                 /* Miscellaneous boolean B */
   int i;                  /* Iterator variable */

   /* Check that the packet is large enough */
   if(!sc_net_check_size(packet, (11 * c->numplayers + 1) * sizeof(dword), "cli_recv_player_state")) return(false);

   /* Check that the total player count is accurate */
   p = (dword *)packet->data;
   if(!sc_net_check_param(c->numplayers, ntohl(*p++), "cli_recv_player_state", "player count")) return(false);

   /* For each player ... */
   for(i = 0; i < c->numplayers; ++i) {
      pl = c->players[i];
      /* First field is authority flag */
      if(ntohl(*p++)) {
         /* Remote player claims authority on this player */
         if(pl->aitype != SC_AI_NETWORK) {
            sc_net_set_error("cli_recv_player_state", "Remote player authority on locally controlled player");
            return(false);
         }

         if(!sc_net_check_param(pl->index, ntohl(*p++), "cli_recv_player_state", "index")) return(false);
         x = ntohl(*p++);
         y = ntohl(*p++);
         if(x != pl->x || y != pl->y) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player position");
            sc_player_set_position(c, pl, x, y);
         }
         x = ntohl(*p++);
         if(x != pl->fuel) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player fuel level");
            pl->fuel = x;
         }
         x = ntohl(*p++);
         y = ntohl(*p++);
         if(x != pl->turret || y != pl->power) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player turret/power");
            sc_player_set_turret(c, pl, x);
            sc_player_set_power(c, pl, y);
         }
         x = ntohl(*p++);
         b = ntohl(*p++);
         if(x != pl->life || b != pl->dead) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player life");
            pl->life = x;
            pl->dead = b;
         }
         x = ntohl(*p++);
         if(x != pl->money) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player money");
            pl->money = x;
         }
         x = ntohl(*p++);
         if(x != pl->selweapon->ident) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player selected weapon");
            sc_player_set_weapon(c, pl, sc_weapon_lookup(c->weapons, x, SC_WEAPON_LIMIT_NONE));
         }
      } else {
         /* Player does not claim to be authoritative */
         if(!sc_net_check_param(pl->index, ntohl(*p++), "cli_recv_player_state", "index")) return(false);
         x = ntohl(*p++);
         y = ntohl(*p++);
         if(x != pl->x || y != pl->y) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player position");
         }
         x = ntohl(*p++);
         if(x != pl->fuel) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player fuel level");
         }
         x = ntohl(*p++);
         y = ntohl(*p++);
         if(x != pl->turret || y != pl->power) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player turret/power");
         }
         x = ntohl(*p++);
         b = ntohl(*p++);
         if(x != pl->life || b != pl->dead) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player life");
         }
         x = ntohl(*p++);
         if(x != pl->money) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player money");
         }
         x = ntohl(*p++);
         if(x != pl->selweapon->ident) {
            sc_net_set_error("cli_recv_player_state", "Discrepancy on remote player selected weapon");
         }
      }
   }

   /* Okay, we're done with that mess... */
   sc_net_set_info("cli_recv_player_state", "Player state received");
   return(true);

}



static bool _sc_net_process_message(sc_config *c, sc_client *cli, sc_packet *packet) {

   char buffer[SC_NET_BUFFER_SIZE];   /* Temporary output buffer */
   char infomsg[SC_NET_BUFFER_SIZE];  /* Text to be displayed to user */

   switch(packet->msg_type) {
   case SC_NET_SVR_ACCEPTED:
      SC_CONN_CLEAR_FLAGS(cli->server, SC_CONN_NEED_ACCEPT);
      _sc_net_client_send_name(cli);
      memcpy(buffer, packet->data + sizeof(dword), packet->data_size - sizeof(dword));
      buffer[packet->data_size - sizeof(dword)] = '\0';
      sbprintf(infomsg, sizeof(infomsg), "We have connected to server, version \"%s\"", buffer);
      sc_net_set_info("process_message", infomsg);
      break;
   case SC_NET_SVR_REJECTED:
      SC_CONN_SET_FLAGS(cli->server, SC_CONN_REJECTED);
      sbprintf(infomsg, sizeof(infomsg), "We were rejected from server:  %s", packet->data);
      sc_net_set_error("process_message", infomsg);
      sc_window_message(c->window, "Client Rejected", sc_net_get_error());
      break;
   case SC_NET_SVR_QUIT:
      SC_CONN_SET_FLAGS(cli->server, SC_CONN_QUIT);
      sc_net_set_info("process_message", "client: server sent QUIT");
      break;
   case SC_NET_CHAT:
      memcpy(buffer, packet->data, min(packet->data_size, SC_NET_BUFFER_SIZE - 1));
      buffer[min(packet->data_size, SC_NET_BUFFER_SIZE - 1)] = '\0';
      sc_net_set_info("chat", buffer);
#if 0 /* TEMP - Chat is broken in GTK 2.  As in, this does not compile. */
      sc_chat_window_update(c->window, buffer);
#endif
      break;
   case SC_NET_SVR_CONFIG_DATA:
      sc_net_cli_recv_config(c, cli, packet);
      break;
   case SC_NET_SVR_PLAYER_DATA:
      sc_net_cli_recv_players(c, cli, packet);
      break;
   case SC_NET_SVR_ORDERS:
      _sc_net_cli_recv_orders(c, packet);
      break;
   case SC_NET_INVENTORY:
      _sc_net_cli_recv_inventory(c, packet);
      break;
   case SC_NET_SHIELDS:
      _sc_net_cli_recv_shields(c, packet);
      break;
   case SC_NET_BATTERY:
      _sc_net_cli_recv_battery(c, packet);
      break;
   case SC_NET_PLFLAGS:
      _sc_net_cli_recv_flags(c, packet);
      break;
   case SC_NET_PLAYER_STATE:
      _sc_net_cli_recv_player_state(c, packet);
      break;
   case SC_NET_SVR_SYNC_RESP:
      _sc_net_cli_recv_sync(c, cli, packet);
      break;
   case SC_NET_CLIENT_STATUS:
      sc_net_client_recv_status(c, cli, packet);
      break;
   default:
      sbprintf(infomsg, sizeof(infomsg), "invalid packet type (%08x) received by client", packet->msg_type);
      sc_net_set_error("process_message", infomsg);
      break;
   }

   return(true);

}



bool sc_net_client_handle_packet(sc_config *c, void *parm, sc_packet *p) {

   sc_client *cli = (sc_client *)parm;

   /* Sanity checks */
   if(c == NULL || parm == NULL || p == NULL) return(false);

   /* Compare RNG state with that of the server... */
   if(p->next_rnd != game_rand_peek()) {
      /* TEMP - This area needs a more graceful failure path
                and a better error message in general. */
      printf("warning: server says our next random value is %i but we thought it was %i\n",
             p->next_rnd, game_rand_peek());
   }

   /* This is an existing connection, process as event */
   return(_sc_net_process_message(c, cli, p));

}



bool sc_net_client_run(sc_config *c, sc_client *cli) {

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);

   /* Check for incoming packets */
   while(sc_net_recv_packet(&cli->server, c, (void *)cli, sc_net_client_handle_packet)) /* just loop */;
   return(true);

}



bool sc_net_client_death(sc_client **cli) {

   /* Check if a client died; if so, then free it. */
   if(cli == NULL || *cli == NULL) return(false);
   if(SC_CONN_IS_DEAD((*cli)->server)) {
      /* This client has died; free the connection. */
      sc_net_client_free(cli, "Connection died");
      return(true);
   }

   /* No connections to free, this time */
   return(false);

}



bool sc_net_client_chat(sc_client *cli, const char *msg) {

   char buffer[SC_NET_BUFFER_SIZE];

   /* Sanity checks */
   if(cli == NULL) return(false);

   if(!SC_CONN_IS_DEAD(cli->server)) {
      sbprintf(buffer, sizeof(buffer), "%s: %s", cli->name, msg);
      return(sc_net_send_message(&cli->server, SC_NET_CHAT, buffer));
   }
   return(false);

}



bool sc_net_client_sync(sc_client *cli, dword flag, bool isserver) {
/* sc_net_client_sync

   Okay, this is how this works. The sstate.c code calls this function
   whenever it wants to synchronise with the server (end of inventory, end
   of turn, or end of round confirmation). When this is called, the client
   expects to block until server has confirmed they may continue.  The flag
   indicates what type of network sync event the client is waiting for.

   isserver is relevant because the server is the ``authority'' on sync
   requests, i.e. the server is always keeping proper game. So if we are the
   server, we set isserver to true, which munges a special flag into the
   packet.

   When the server is damn well ready, they will send us a sync response; if
   it is at all useful, it will also include the server's next expected 
   random value.  This allows us to easily check to see if we are in sync
   with the server; if we share the same next random value then we are 
   probably okay, whereas if they differ, we know something has gone
   horribly wrong.

   Note that if we were syncing on one type of event, and the server was
   syncing on a different event, something went wrong -- they will send
   us a bogus random value and from there we will have to play the same
   ``what went wrong?'' game that we must play if random values normally
   go out of alignment.  */

   sc_packet packet;       /* Packet to send */
   dword *p;               /* Pointer to dat */

   /* Make sure we have a client socket */
   if(cli == NULL) return(false);

   /* Set state flags; we'll be waiting now */
   SC_CONN_SET_FLAGS(cli->server, SC_CONN_WAIT_SYNC);
   SC_CONN_SET_ARG(cli->server, flag);

   /* If server, add server sync flag */
   if(isserver) flag |= SC_CONN_SYNC_SERV;
   else flag &= ~SC_CONN_SYNC_SERV;

   /* Setup the packet */
   if(!sc_net_packet_init(&packet, SC_NET_CLI_SYNC_RQST, 2 * sizeof(dword))) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(flag);              /* Send the sync flag to use */
   *p++ = htonl(game_rand_peek());  /* Send our next random value */

   /* Attempt to send the packet */
   sc_net_set_info("client_sync", "syncing with server on game event");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);

   /* Send a status update */
   sc_net_client_update_status(cli);
   return(true);

}



bool sc_net_client_send_orders(const sc_config *c, sc_client *cli, int playerid) {
/* sc_net_client_send_orders
   Send our orders to the server. The server will hopefully accept
   our orders and send a broadcast packet to all players containing
   the composite of player orders. I hope. At any rate, if we are
   able to send the packet, then we'll return true. This packet is
   not directly relayed to other clients. */

   const sc_player *pl;    /* Player data structure */
   sc_packet packet;       /* Packet to send */
   dword *p;               /* Pointer into data */

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);
   pl = c->players[playerid];
   if(!pl || pl->aitype == SC_AI_NETWORK) return(false);

   /* Initialise the packet */
   if(!sc_net_packet_init( &packet, 
                           SC_NET_CLI_ORDERS, 
                           7 * sizeof(dword) )) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(playerid);
   *p++ = htonl(pl->turret);
   *p++ = htonl(pl->power);
   *p++ = htonl(pl->selweapon->ident);
   *p++ = htonl(pl->selshield->ident);
   *p++ = htonl(pl->x);
   *p++ = htonl(pl->y);

   /* Attempt to send the packet */
   sc_net_set_info("client_send_orders", "sending our game orders");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



bool sc_net_client_send_shields(const sc_config *c, sc_client *cli, int playerid) {
/* sc_net_client_send_shields
   Let the server know that we are activating our shields. This packet
   may be directly relayed to other clients. Returns true if the packet
   makes it out the door; from there it's anyone's game... */

   sc_packet packet;       /* Packet to be sent */
   sc_player *pl;
   dword *p;

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);
   pl = c->players[playerid];
   if(pl == NULL || pl->shield == NULL || pl->aitype == SC_AI_NETWORK) return(false);

   /* Initialise the packet */
   if(!sc_net_packet_init(&packet, SC_NET_SHIELDS, 2 * sizeof(dword))) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(playerid);
   *p++ = htonl(pl->shield->info->ident);

   /* Attempt to send the packet */
   sc_net_set_info("client_send_shields", "sending our shield status");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



bool sc_net_client_send_battery(const sc_config *c, sc_client *cli, int playerid) {
/* sc_net_client_send_battery
   Let the server know that we are activating a battery. This packet
   may be directly relayed to other clients. Returns true if the packet
   makes it out the door; from there it's anyone's game... */

   sc_packet packet;       /* Packet to be sent */

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);
   if(c->players[playerid]->aitype == SC_AI_NETWORK) return(false);

   /* Initialise the packet */
   if(!sc_net_packet_init(&packet, SC_NET_BATTERY, sizeof(dword))) return(false);
   *(dword *)packet.data = htonl(playerid);

   /* Attempt to send the packet */
   sc_net_set_info("client_send_battery", "activating a battery");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



bool sc_net_client_send_flags(const sc_config *c, sc_client *cli, int playerid) {
/* sc_net_client_send_flags
   Let the server know what command flags we have set, e.g. contact
   trigger status.  Returns true if the packet makes it out the door;
   from there it's anyone's game... */

   sc_packet packet;       /* Packet to be sent */
   dword *p;               /* Packet data pointer */

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);
   if(c->players[playerid]->aitype == SC_AI_NETWORK) return(false);

   /* Initialise the packet */
   if(!sc_net_packet_init(&packet, SC_NET_PLFLAGS, 3 * sizeof(dword))) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(playerid);
   *p++ = htonl(c->players[playerid]->contacttriggers);
   *p++ = htonl(c->players[playerid]->ac_state);

   /* Attempt to send the packet */
   sc_net_set_info("client_send_flags", "sending our command flags");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



bool sc_net_client_send_inventory(const sc_config *c, sc_client *cli, int playerid) {

   const sc_accessory_info *ai;  /* Accessory info */
   const sc_weapon_info *wi;  /* Weapon info */
   const sc_player *pl;    /* Player data */
   sc_packet packet;       /* Packet to be sent */
   dword *p;               /* Pointer into packet data */
   int acount;             /* Accessory count (total) */
   int wcount;             /* Weapon count (total) */
   int size;               /* Expected packet size */
   int i;                  /* Iterator variable */

   /* Sanity checks */   
   if(c == NULL || cli == NULL) return(false);
   pl = c->players[playerid];
   if(pl->aitype == SC_AI_NETWORK) return(false);

   /* Make sure packet is large enough to hold additional data.
      Packets for weapons and accessories look like this:
      [ integer item_id ], [ integer item_count ]  (repeat) */
   acount = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_NONE);
   wcount = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_NONE);
   size = (8 + 2 * (wcount + acount)) * sizeof(dword);

   /* Initialise header data */
   if(!sc_net_packet_init(&packet, SC_NET_INVENTORY, size)) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(playerid);
   *p++ = htonl(wcount);
   *p++ = htonl(acount);

   /* Initialise weapons block */
   *p++ = htonl(SC_PACKET_INVENTORY_WEAPONS);
   wi = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_NONE);
   for(i = 0; i < wcount; ++i) {
      *p++ = htonl(wi->ident);
      *p++ = htonl(wi->inventories[playerid]);
      wi = sc_weapon_next(c->weapons, wi, SC_WEAPON_LIMIT_NONE);
   }

   /* Initialise accessories block */
   *p++ = htonl(SC_PACKET_INVENTORY_ACCESSORIES);
   ai = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_NONE);
   for(i = 0; i < acount; ++i) {
      *p++ = htonl(ai->ident);
      *p++ = htonl(ai->inventories[playerid]);
      ai = sc_accessory_next(c->accessories, ai, SC_ACCESSORY_LIMIT_NONE);
   }

   /* Intiialise miscellaneous block */
   *p++ = htonl(SC_PACKET_INVENTORY_MISC);
   *p++ = htonl(pl->money);
   *p++ = htonl(pl->oldmoney);

   /* Attempt to send the packet */
   sc_net_set_info("client_send_inventory", "sending our inventory");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);

}



bool sc_net_client_send_player_state(const sc_config *c, sc_client *cli) {

   const sc_player *pl;    /* Player data */
   sc_packet packet;       /* Packet to send */
   dword *p;               /* Pointer to packet data */
   int size;               /* Expected packet size */
   int i;                  /* Iterator variable */

   /* Sanity checks */
   if(c == NULL || cli == NULL) return(false);

   /* Initialise packet data */
   size = (1 + 11 * c->numplayers) * sizeof(dword);
   if(!sc_net_packet_init(&packet, SC_NET_PLAYER_STATE, size)) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(c->numplayers);
   for(i = 0; i < c->numplayers; ++i) {
      /* First field is authority flag */
      pl = c->players[i];
      *p++ = htonl(pl == SC_AI_HUMAN);
      *p++ = htonl(pl->index);
      *p++ = htonl(pl->x);
      *p++ = htonl(pl->y);
      *p++ = htonl(pl->fuel);
      *p++ = htonl(pl->turret);
      *p++ = htonl(pl->power);
      *p++ = htonl(pl->life);
      *p++ = htonl(pl->dead);
      *p++ = htonl(pl->money);
      *p++ = htonl(pl->selweapon->ident);
   }

   /* Attempt to send */
   sc_net_set_info("client_send_player_state", "sending our player state");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);

   /* Send a status update */
   sc_net_client_update_status(cli);
   return(true);

}
