/* $Header: /fridge/cvs/xscorch/sgame/stankpro.c,v 1.21 2009-04-26 17:39:45 jacob Exp $ */
/*
   
   xscorch - stankpro.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Tank profiles
    

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
#include <stankpro.h>

#include <libj/jreg/libjreg.h>
#include <libj/jstr/libjstr.h>



static const reg_class_data _reg_tank_profile_class[] = {
   { "radius",                  REG_INTEGER,    NULL },
   { "turretRadius",            REG_INTEGER,    NULL },
   { "shelfSize",               REG_INTEGER,    NULL },
   { "efficiency",              REG_INTEGER,    NULL },
   { "hardness",                REG_INTEGER,    NULL },
   { "mobile",                  REG_BOOLEAN,    NULL },
   { 0, 0, 0 }
};



static bool _sc_tank_profile_verify(sc_tank_profile *profile) {
/* sc_tank_profile_verify
   This function is responsible for making certain that the tank profile is 
   valid.  In particular, this function checks that the tank is base-aligned
   (the bottom row is not blank), and that the tank has a minimum area. The
   latter requirement keeps people from registering tanks that are one pixel
   in size, which is just far too evil.  Note, this function is tolerant of
   custom tank sizes.  True is returned if the tank is valid (or has been
   adjusted to a valid configuration), and false is returned on an error.  
   Caution, no sanity checks on profile pointer.  */

   const unsigned char *ps;   /* Used in profile reads and copy */
   unsigned char *pd;         /* Used in profile copy - destination */
   int lastrow;      /* Last row that wasn't completely clear */
   int height;       /* Total height of the profile */
   int width;        /* Total width of the profile */
   int area;         /* Total area of the profile (pixels) */
   int row;          /* Iterator: current row */
   int col;          /* Iterator: current column */

   /* Calculate the width, height of the profile; initialise area */
   width  = 1 + profile->radius * 2;
   height = 1 + profile->radius;
   area   = 0;

   /* Scan to determine the last nonclear row, and also calculate area */
   for(ps = profile->data, lastrow = 0, row = 0; row < height; ++row) {
      for(col = 0; col < width; ++col, ++ps) {
         if(*ps != SC_TANK_PROFILE_CLEAR) {
            /* Found a solid pixel */
            lastrow = row;
            ++area;
         } /* Found solid? */
      } /* Scanning columns */
   } /* Scanning rows */

   /* If last solid row was not base, then we will need to move the 
      profile downward so it is.  This is basically a memory copy. */
   if(lastrow < height - 1) {
      /* We need to move (height - 1 - lastrow) rows down. */
      ps = profile->data + (lastrow + 1) * width;  /* Source */
      pd = profile->data + height * width;         /* Destination */
      for(row = lastrow; row >= 0; --row) {
         for(col = width; col >= 0; --col) {
            /* Move this row down */
            --ps;
            --pd;
            *pd = *ps;
         } /* Moving column */
      } /* Moving rows down */
      
      /* This loop clears the top area, which is garbage now. */
      for(pd = profile->data, row = height - 1 - lastrow; row >= 0; --row) {
         for(col = width; col >= 0; ++pd, --col) {
            *pd = SC_TANK_PROFILE_CLEAR;
         } /* Clear columns */
      } /* Clear rows */
   } /* Had to move down profile? */

   /* Make sure tank isn't a single pixel. */   
   if(area < SC_TANK_PROFILE_MIN_AREA) {
      /* Too small! */
      fprintf(stderr, "Area of tank \"%s\" is too small, calculated %d < required %d\n",
              profile->name, area, SC_TANK_PROFILE_MIN_AREA);
      return(false);
   } /* Sanity check */

   /* Okay, the tank checks out! */
   return(true);

}



static bool _sc_tank_profile_new(sc_tank_profile **plist, reg *r, reg_var *rv) {
/* sc_tank_profile_new
   This function, if successful, allocates and reads a new tank profile, and
   adds it to the END of the linked list specified.  True is returned on
   success; if an error occurred while parsing the tank profile, false is
   returned.  The tank profile data is read from the rowXX assignments in
   the tank_profile_class object, given in rv.  Caution, input pointers are
   not checked for sanity.  */

   sc_tank_profile *profile;  /* Newly allocated profile */
   char linename[0x1000];     /* Variable name for a scanline */
   char linedata[0x1000];     /* Assigned value for a scanline */
   unsigned char *p;          /* Writable pointer into profile data */
   int height;       /* Height of the profile */
   int width;        /* Width of the profile */
   int row;          /* Iterator: current row */
   int col;          /* Iterator: current column */
   int lsz;          /* Length of a scanline, in chars */

   /* Allocate a new profile */   
   profile = (sc_tank_profile *)malloc(sizeof(sc_tank_profile));
   if(profile == NULL) return(false);

   /* Initialise profile data members */
   profile->next        = NULL;
   profile->radius      = 0;
   profile->turretradius= 0;
   profile->shelfsize   = 5;
   profile->efficiency  = 100;
   profile->hardness    = 100;
   profile->mobile      = false;
   strcopyb(profile->name, reg_get_var_name(rv), sizeof(profile->name));

   /* Determine the radius of the profile. Note, if the radius wasn't given, 
      then radius will remain zero, and the trap below will catch it.  So we
      don't need to check return value here, yay. */
   reg_get_integer(r, rv, "radius",      &profile->radius);
   reg_get_integer(r, rv, "turretRadius",&profile->turretradius);
   reg_get_integer(r, rv, "shelfSize",   &profile->shelfsize);
   reg_get_integer(r, rv, "efficiency",  &profile->efficiency);
   reg_get_integer(r, rv, "hardness",    &profile->hardness);
   reg_get_boolean(r, rv, "mobile",      &profile->mobile);

   /* Update name of profile if immobile */
   if(!profile->mobile) {
      strconcatb(profile->name, " (I)", sizeof(profile->name));
   }
   
   /* Calculate and check width, height of the profile. */
   width  = 1 + profile->radius * 2;
   height = 1 + profile->radius;
   if(width <= 0 || height <= 0) {
      fprintf(stderr, "Tank \"%s\" has requires a positive radius, I saw %d\n", profile->name, profile->radius);
      free(profile);
      return(false);
   } /* Sanity check */
   if(profile->turretradius <= profile->radius) {
      fprintf(stderr, "Tank \"%s\" has an absurdly small (<= radius) turretRadius, I saw %d\n",
              profile->name, profile->turretradius);
      free(profile);
      return(false);
   } /* Sanity check */

   /* Allocate the profile data */   
   profile->data = (unsigned char *)malloc(sizeof(char) * width * height);
   if(profile->data == NULL) {
      free(profile);
      return(false);
   }

   /* Read in the profile data from the regvar given. */
   for(p = profile->data, row = 0; row < height; ++row) {
      /* Compute the name of the current scanline */
      sbprintf(linename, sizeof(linename), "row%02d", row);
      /* Attempt to read the scanline. If this call fails, then we assume
         the scanline is entirely blank. Otherwise, compute the length. */
      if(reg_get_string(r, rv, linename, linedata, sizeof(linedata))) {
         lsz = strblenn(linedata, sizeof(linedata));
      } else {
         lsz = 0;
      }
      /* Read data from the scanline. If we run out of characters, we will
         assume the remaining characters encoded for `blank' or clear. */
      for(col = 0; col < width; ++col, ++p) {
         /* Check if we ran off the scanline */
         if(col < lsz) {
            /* read character from scanline */
            if(linedata[col] == ' ' || linedata[col] == '.') {
               *p = SC_TANK_PROFILE_CLEAR;
            } else {
               *p = SC_TANK_PROFILE_SOLID;
            } /* Data read from scanline */
         } else {
            /* Assume clear */
            *p = SC_TANK_PROFILE_CLEAR;
         } /* Scanline check */ 
      } /* Reading the scanline */
   } /* Reading each row... */

   /* Verify that the tank profile is valid. */
   if(!_sc_tank_profile_verify(profile)) {
      free(profile->data);
      free(profile);
      return(false);
   }

   /* Add this profile to the end of the linked list. */
   while(*plist != NULL) plist = &((*plist)->next);
   *plist = profile;
   return(true);

}



bool sc_tank_profile_add(sc_tank_profile **plist, const char *datafile) {
/* sc_tank_profile_add
   Adds a new set of tank profiles to the linked list given, by reading data
   from the datafile given.  On success, this function returns true (even if
   the datafile contained no new tank profiles).  */

   reg_var *rv;      /* Regvar for each block */
   reg *r;           /* Registry structure */

   /* Sanity checks */
   if(plist == NULL || datafile == NULL) return(false);
   
   /* Compute filename and initialise registry */
   r = reg_new(datafile);
   if(r == NULL) {
      fprintf(stderr, "%s: core dump imminent. Critical alloc error, aborting.\n", datafile);
      return(false);
   }
   reg_class_add(r, "tank_profile_class");
   reg_class_register_vars(r, "tank_profile_class", _reg_tank_profile_class);

   /* Attempt to load the registry file containing tank profiles. */
   if(!reg_load(r)) {
      fprintf(stderr, "%s: core dump imminent. Cannot locate datafile \"%s\", aborting.\n", datafile, datafile);
      return(false);
   }

   /* Get the first variable in the file. */
   rv = reg_get_block_head(reg_get_top(r));
   while(rv != NULL) {
      /* Process this variable. */
      if(strequal(reg_get_var_class(rv), "tank_profile_class")) {
         if(!_sc_tank_profile_new(plist, r, rv)) {
            fprintf(stderr, "%s: core dump imminent. tank profile %s failed, aborting.\n", datafile, reg_get_var_name(rv));
            return(false);
         } /* Failed to create profile? */
      } else {
         fprintf(stderr, "%s: warning: \"%s\" isn't a valid class in this context, for variable \"%s\".\n", datafile, reg_get_var_class(rv), reg_get_var_name(rv));
      } /* Is regvar a valid tank profile? */
      rv = reg_get_next_var(rv);
   }

   /* Release the registry and exit. */
   reg_free(&r);
   return(true);

}



void sc_tank_profile_free(sc_tank_profile **profile) {
/* sc_tank_profile_free
   Release a chain of tank profiles. */

   sc_tank_profile *cur;

   if(profile == NULL) return;
   while(*profile != NULL) {
      cur = *profile;
      *profile = cur->next;
      free(cur->data);
      free(cur);
   }

}



const sc_tank_profile *sc_tank_profile_lookup(const sc_tank_profile *plist, int index) {
/* sc_tank_profile_lookup
   Return the index'th element in list plist, or NULL on error.  */
   
   if(index < 0) return(NULL);
   while(index > 0 && plist != NULL) {
      plist = plist->next;
      --index;
   }
   return(plist);

}



int sc_tank_profile_index_of(const sc_tank_profile *plist, const sc_tank_profile *profile) {
/* sc_tank_profile_index_of 
   Returns the index of profile in the list, -1 if it doesn't appear. */
   
   int index;
   index = 0;
   while(plist != NULL && plist != profile) {
      ++index;
      plist = plist->next;
   }
   if(plist == NULL) return(-1);
   return(index);
   
}



int sc_tank_profile_size(const sc_tank_profile *plist) {
/* sc_tank_profile_size
   Return the number of elements in this profile list.  */
   
   int count;
   count = 0;
   while(plist != NULL) {
      ++count;
      plist = plist->next;
   }
   return(count);
   
}
