/* $Header: /fridge/cvs/xscorch/sgame/soptions.c,v 1.21 2009-04-26 17:39:41 jacob Exp $ */
/*

   xscorch - soptions.c       Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched option processing
   Blame waif for the egg


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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <soptions.h>
#include <scffile.h>
#include <sconfig.h>
#include <sland.h>
#include <sweapon.h>

#include <snet/snet.h>
#include <sutil/md5.h>

#include <libj/jreg/libjreg.h>
#include <libj/jstr/libjstr.h>


#define  SC_OPTION_BUFFER        0x100

#define  SC_OPTION_CONFIG_FILE   0x1001
#define  SC_OPTION_INSANITY      0x1002
#define  SC_OPTION_YIELDS        0x1003
#define  SC_OPTION_HQMIXER       0x1004
#define  SC_OPTION_NO_HQMIXER    0x1005

#define  SC_OPTION_NAME          0x2001
#define  SC_OPTION_PORT          0x2002
#define  SC_OPTION_SERVER        0x2003
#define  SC_OPTION_CLIENT        0x2004

/* These variables are provided by either getopt.c OR glibc,
   depending on where we are getting getopt from... */
extern int opterr;
extern int optind;
extern int optopt;

static struct option _sc_long_options[] = {
 { "config", 1, 0, SC_OPTION_CONFIG_FILE },
 { "geometry", 1, 0, 'g' },
 { "help", 0, 0, 'h' },
 { "insanity", 0, 0, SC_OPTION_INSANITY },
 { "yields", 0, 0, SC_OPTION_YIELDS },
 { "sound", 0, 0, 'S' },
 { "nosound", 0, 0, 's' },
 { "hqmixer", 0, 0, SC_OPTION_HQMIXER },
 { "nohqmixer", 0, 0, SC_OPTION_NO_HQMIXER },
#if USE_NETWORK
 { "name", 1, 0, SC_OPTION_NAME },
 { "port", 1, 0, SC_OPTION_PORT },
 { "server", 0, 0, SC_OPTION_SERVER },
 { "client", 1, 0, SC_OPTION_CLIENT },
#endif /* Network? */
 { NULL, 0, 0, 0 }
};



static bool _sc_options_x_y(char *arg, int *x, int *y) {

   char *p = arg;

   /* nothing following -g */
   if(p == NULL) return(false);

   while(*p != '\0' && *p != 'x' && *p != 'X') ++p;
   if(*p == '\0') return(false);

   *p = '\0';
   *x = atoi(arg);
   *y = atoi(p + 1);
   return(true);

}



int sc_options_usage(const char *progname) {

   printf("Usage:  %s [options]\n\n", progname);

   printf("--config=file       Load an alternate config file\n");
   printf("--geometry=nxm, -g  Set geometry to width n, height m\n");
   printf("--help, -h          Display this help\n");
   printf("--insanity          If you have to ask ...\n");
   printf("--yields            Display weapon yields\n");
   printf("--sound, -S         Enable music and sound effects\n");
   printf("--nosound, -s       Disable music and sound effects\n");
   printf("--hqmixer           Enable use of high-quality mixer\n");
   printf("--nohqmixer         Disable use of high-quality mixer\n");
   #if USE_NETWORK
      printf("--name=playername   Player name to use in network connection\n");
      printf("--port=number       Network port to use (client or server)\n");
      printf("--client=servername Connect as a client to specified server\n");
      printf("--server            Start a new server\n");
   #endif /* Network? */

   printf("\n");
   return(1);

}



int sc_options_parse(sc_config *c, int argc, char **argv) {

  #if USE_NETWORK
   char name[SC_OPTION_BUFFER];
   char server[SC_OPTION_BUFFER];
   bool isserver = false;
   bool isclient = false;
   int port;
  #endif /* Net? */
   int ch;
   char digest[16];

   /* Waif's Hidden Easter Egg =)  */
   unsigned char whee[16]  = { 0x0f, 0x6f, 0xfd, 0xa3, 0xc9, 0xc9, 0xce, 0x15,
                               0xf2, 0x79, 0xb7, 0x88, 0xb4, 0x86, 0xe9, 0xca };
   /* Justin's response to Waif's Hidden Easter Egg :)  */
   unsigned char wheee[16] = { 0x22, 0x47, 0x0e, 0x3a, 0x15, 0x8a, 0x76, 0x86,
                               0xe1, 0xf7, 0x5c, 0xd9, 0xb8, 0x14, 0x52, 0x3a };

   /* Set network default options */
   #if USE_NETWORK
      strcopyb(name, getenv("USER"), sizeof(name));
      strcopyb(server, SC_NET_DEFAULT_SERVER, sizeof(name));
      port = SC_NET_DEFAULT_PORT;
   #endif /* Net? */

   /* Set up getopt.  We turn off the "unknown option" warning for the sake
      of hidden options.  We keep track of the option parsing so we can look
      hidden options and print a warning if they're really missing options. */
   opterr = optind = optopt = 0;

   while(EOF != (ch = getopt_long(argc, argv, "g:sSh", _sc_long_options, NULL))) switch(ch) {
      case SC_OPTION_CONFIG_FILE:
         /* New config file to parse in! */
         reg_set_name(c->cfreg, optarg);
         if(!sc_config_file_load(c)) {
            fprintf(stderr, "Failed to load alternate configuration file \"%s\".\n", optarg);
         }
         break;

      case SC_OPTION_INSANITY:
         /* If you are reading this, then umm... wow. */
         /* You'll find no help here, why do you want to know? */
         /* This is where mechanical excellence, one-thousand
            four-hundred horsepower pays off.  */
         fprintf(stderr, "Warning:  The insanity subsystem is enabled.\n\n");
         fprintf(stderr, "   Life... Dreams... Hope...\n");
         fprintf(stderr, "   Where'd they come from...\n");
         fprintf(stderr, "   And where are they headed?\n");
         fprintf(stderr, "   These things... I am going to destroy!\n");
         fprintf(stderr, "    -- Kefka, Fourth Tier, Final Fantasy VI\n\n");

         c->insanity = true;
         break;

      case SC_OPTION_YIELDS:
         sc_weapon_print_yields(c->weapons);
         return(1);

      case 'g':
         if(_sc_options_x_y(optarg, &c->fieldwidth, &c->fieldheight)) {
            if(c->fieldwidth < SC_MIN_FIELD_WIDTH || c->fieldwidth > SC_MAX_FIELD_WIDTH) {
               fprintf(stderr, "--geometry, width given, %d, out of range [%d, %d]\n",
                       c->fieldwidth, SC_MIN_FIELD_WIDTH, SC_MAX_FIELD_WIDTH);
               return(1);
            } else if(c->fieldheight < SC_MIN_FIELD_HEIGHT || c->fieldheight > SC_MAX_FIELD_HEIGHT) {
               fprintf(stderr, "--geometry, height given, %d, out of range [%d, %d]\n",
                       c->fieldheight, SC_MIN_FIELD_HEIGHT, SC_MAX_FIELD_HEIGHT);
               return(1);
            }
            if(c->fieldheight < c->maxheight) {
               c->maxheight = c->fieldheight - 32;
            }
            sc_land_setup(c->land, c->fieldwidth, c->fieldheight, sc_land_flags(c));
         } else {
            fprintf(stderr, "--geometry requires an option of form <width>x<height>, e.g. \"640x480\"\n");
            return(1);
         }
         break;

      case 'h':
         return(sc_options_usage(argv[0]));

      case 'S':
         c->enablesound = true;
         break;

      case 's':
         c->enablesound = false;
         break;

      case SC_OPTION_HQMIXER:
         c->usehqmixer = true;
         break;

      case SC_OPTION_NO_HQMIXER:
         c->usehqmixer = false;
         break;

      #if USE_NETWORK
         case SC_OPTION_NAME:
            strcopyb(name, optarg, sizeof(name));
            break;
         case SC_OPTION_PORT:
            port = atoi(optarg);
            break;
         case SC_OPTION_SERVER:
            isserver = true;
            break;
         case SC_OPTION_CLIENT:
            isclient = true;
            strcopyb(server, optarg, sizeof(server));
            break;
      #endif /* Network? */

      case '?':
         /* Unknown option. Hidden? */
         if(0 == optopt) {
            /* Unknown, starts with -- */
            md5_buffer(argv[optind - 1] + 2, strlenn(argv[optind - 1]) - 2, (void *)digest);

            if(!memcmp((void *)whee, (void *)digest, 16)) {
               printf("WHEE! You've found Waif's Hidden Easter Egg!\n");
               printf("   ...which is still in progress =/\n\n");
               printf("JDS:  I *finally* figured out what this bloody encrypted string is.\n");
               printf("      Could it be, perhaps, that Waif is dropping a subtle hint here?\n\n");
               printf("JDS:  So, since you found this, I'll give a little story.  A long time\n"
                      "      ago, YEARS ago probably, I figured out what this option was.  If\n"
                      "      you're reading this from the console output then you obviously\n"
                      "      know what it is too.  Anywho, at some point the code broke and\n"
                      "      the check was negated, allowing almost any option to \"match\"\n"
                      "      except for the correct one.\n\n"
                      "      Well, one day I tried \"--version\" and triggered the egg due to\n"
                      "      the bug.  My above comment was in response to triggering that\n"
                      "      bug, and thinking that waif was perhaps dropping a hint for a new\n"
                      "      option we needed to support.  I am clearly mistaken :)  Still, it\n"
                      "      makes for an odd story.\n\n"
                      "      At any rate, my response to waif's option is the next line down :)\n");
               break;
            }

            if(!memcmp((void *)wheee, (void *)digest, 16)) {
               printf("Wow, you found Justin's response to waif's egg!  Impressive...\n\n");
               break;
            }

            /* really unknown */
            fprintf(stderr, "%s: unrecognized option `%s'\n", argv[0], argv[optind-1]);
         } else {
            /* really unknown, single - type option */
            fprintf(stderr, "%s: unrecognized option `%c'\n", argv[0], optopt);
         }

         /* FALLTHROUGH OK */

      case ':':
      default:
         return(sc_options_usage(argv[0]));

   }

   /* Running a server or client connection? */
   #if USE_NETWORK
      if(isserver) {
         c->server = sc_net_server_new(c, port);
         if(c->server != NULL) isclient = true;
      }
      if(isclient) {
         c->client = sc_net_client_new(name, server, port);
      }
   #endif /* Network? */

   return(0);

}
