/* $Header: /fridge/cvs/xscorch/libj/jreg/reg.c,v 1.10 2009-04-26 17:39:27 jacob Exp $ */
/*

   libj - reg.c               Copyright(c) 2000-2003 Justin David Smith
   justins (a) chaos2.org     http://chaos2.org/

   Config file processing


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

   This file is part of LIBJ.

*/
#include <_reg.h>



/* Waiting for robotnik */



reg *reg_new(const char *filename) {
/* reg_new
   Create a new registry structure, with the associated filename.
   The filename is used by reg_load() and reg_save() functions,
   and may be modified later.  */

   reg *r;        /* Newly constructed reg variable */
   reg_class *tc; /* Class registry for top class. */

   /* Create the new object */
   r = (reg *)malloc(sizeof(reg));
   if(r == NULL) return(NULL);

   /* Initialise class registry */
   r->classes = NULL;
   reg_class_add(r, REG_TOP);
   tc = reg_class_lookup(r->classes, REG_TOP);

   /* Initialise toplevel block */
   r->top = reg_var_new_block(r, REG_TOP, tc);
   if(r->top == NULL) {
      /* Failed to allocate toplevel */
      free(r);
      return(false);
   } /* Do we have toplevel block? */

   /* Initialise associated filename */
   r->filename = NULL;
   reg_set_name(r, filename);

   /* Return the register structure */
   return(r);

}



void reg_free(reg **r) {
/* reg_free
   Releases a registry, and all associated data.  */

   /* release the registry structure */
   if(r == NULL || *r == NULL) return;
   reg_var_free(&(*r)->top);
   reg_class_release(&(*r)->classes);
   free((*r)->filename);
   free(*r);
   *r = NULL;

}



static inline int _reg_text_to_index(const char *text, const char **list) {
/* reg_text_to_index
   Determine which index the specified text string occurs at.
   <list> is a NULL-terminated list of strings.  */

   int index = 0;
   while(list[index] != NULL) {
      if(strequal(text, list[index])) return(index);
      ++index;
   }
   return(-1);

}



static inline int _reg_value_to_index(unsigned int value, const char **list,
                                      const unsigned int *values) {
/* reg_value_to_index
   Determine which index the specified value lives at.  <list>
   is required since it has the NULL terminator; although it
   is the <values> list we compare <value> to.  */

   int index = 0;
   while(list[index] != NULL) {
      if(value == values[index]) return(index);
      ++index;
   }
   return(-1);

}



int reg_get_integer_from_values(reg *r, reg_var *v, const char *path, int defvalue,
                                const char **names, const unsigned int *values) {
/* reg_get_integer_from_values
   Read the variable <path>, relative to <v>, and attempt to translate
   its string content to an enumerated type value.  If the value cannot
   be obtained (variable undefined, not a string, etc) then <defvalue>
   will be returned.  The enumerated types and corresponding names are
   given as arguments to this function.

   If <path> == NULL, then <v> is read.  */

   char buf[REG_BUFFER];   /* Temporary read-buffer */
   int index;              /* Index to matching name-value pair */

   /* Attempt to retrieve the string */
   if(reg_get_string(r, v, path, buf, REG_BUFFER)) {
      /* We have a string to work with, yay.  Does it
         have a corresponding entry in the values list? */
      trim(buf);
      index = _reg_text_to_index(buf, names);
      if(index >= 0) {
         /* Assign the new default value */
         defvalue = values[index];
      } /* Text matches? */
   } /* Is variable a valid string? */

   /* Return the enumerated value */
   return(defvalue);

}



int reg_get_bitmask_from_values(reg *r, reg_var *v, const char *path, int defvalue,
                                const char **names, const unsigned int *values) {
/* reg_get_bitmask_from_values
   Read the variable <path>, relative to <v>, and attempt to translate its
   string content to a bitmask value.  If the value cannot be obtained
   (variable undefined, not a string, etc) then <defvalue> will be returned.
   The enumerated types and corresponding names are given as arguments to
   this function.  If <path> == NULL, then <v> is read.  */

   char buf[REG_BUFFER];   /* Temporary read-buffer */
   char *ps;               /* Start of `current' bit */
   char *p;                /* Start of `next' bit */
   bool negate;            /* True if this bit negated */
   int  index;             /* Matching name-value pair */

   /* Attempt to retrieve the string */
   if(reg_get_string(r, v, path, buf, REG_BUFFER)) {
      /* Trim-o-rama, and loop while the string isn't empty */
      trim(buf);
      p = buf;
      while(*p != '\0') {
         /* Look for a comma or end-of-string */
         ps = p;
         while(*p != '\0' && *p != ',') ++p;

         /* If we hit a comma, there is another bit to parse */
         if(*p == ',') *(p++) = '\0';

         /* Look for a negation flag (if present) */
         trim(ps);
         negate = (*buf == '!');
         if(negate) trim(++ps);

         /* Attempt to find bitvalue corresponding to text */
         index = _reg_text_to_index(ps, names);
         if(index >= 0) {
            /* We have a matching bitvalue; mix it into default */
            index = values[index];
            if(negate) defvalue = defvalue & (~index);
            else defvalue = defvalue | index;
         } /* Found a matching bitvalue? */

      } /* Looping through named bits */
   } /* Was variable a valid string? */

   /* Return the newly constructed bitfield */
   return(defvalue);

}



bool reg_set_string_from_values(reg *r, reg_var *v, const char *path, int value,
                                const char **names, const unsigned int *values) {
/* reg_set_string_from_values
   Write the enumerated value <value>, as its corresponding string to
   the variable <path> (relative to <v>).  The enumerated types and
   corresponding names are also given.

   If <path> == NULL, then modifications occur directly on <v>.  */

   int index;              /* Index to matching name-value pair */

   /* Attempt to get index matching up with value */
   index = _reg_value_to_index(value, names, values);
   if(index >= 0) {
      /* We have it; try to set this as a string value. */
      return(reg_set_string(r, v, path, names[index]));
   } /* Match found? */

   /* No matches; cannot handle this case */
   return(false);

}



bool reg_set_bitmask_from_values(reg *r, reg_var *v, const char *path, int value,
                                 const char **names, const unsigned int *values) {
/* reg_set_bitmask_from_values
   Write the bitmask <value>, as its corresponding list of strings to the
   variable <path> (relative to <v>).  Enumerated types and corresponding
   names are also given.  If <path> == NULL, then modifications occur
   directly on <v>.  */

   char buf[REG_BUFFER];   /* Temp variable to construct value in */
   bool needcomma;         /* True if need preceding comma */
   int i;                  /* Index of matching element */

   *buf = '\0';
   needcomma = false;

   i = 0;
   while(names[i] != NULL) {
      if((values[i] & value) == values[i]) {
         value = value & (~values[i]);
         if(needcomma) strconcatb(buf, ", ", sizeof(buf));
         strconcatb(buf, names[i], sizeof(buf));
         needcomma = true;
      }
      ++i;
   }

   if(value != 0) return(false);

   return(reg_set_string(r, v, path, buf));

}



bool reg_get_integer(reg *r, reg_var *v, const char *path, int *value) {
/* reg_get_integer
   Reads the integer value <path>, relative to <v>.  True is returned
   on success.  If <path> == NULL, then <v> is read.  */

   reg_var *var;

   var = reg_block_resolve(r, v, path);
   if(var == NULL || var->type != REG_INTEGER) return(false);

   if(value != NULL) *value = var->value.integer;
   return(true);

}



bool reg_get_doublev(reg *r, reg_var *v, const char *path, double *value) {
/* reg_get_doublev
   Reads the double value <path>, relative to <v>.  True is returned
   on success.  If <path> == NULL, then <v> is read.  */

   reg_var *var;

   var = reg_block_resolve(r, v, path);
   if(var == NULL || var->type != REG_DOUBLEV) return(false);

   if(value != NULL) *value = var->value.doublev;
   return(true);

}



bool reg_get_boolean(reg *r, reg_var *v, const char *path, bool *value) {
/* reg_get_boolean
   Reads the boolean value <path>, relative to <v>.  True is returned
   on success.  If <path> == NULL, then <v> is read.  */

   reg_var *var;

   var = reg_block_resolve(r, v, path);
   if(var == NULL || var->type != REG_BOOLEAN) return(false);

   if(value != NULL) *value = var->value.boolean.value;
   return(true);

}



bool reg_get_string(reg *r, reg_var *v, const char *path, char *value, int size) {
/* reg_get_string
   Reads the string value <path>, relative to <v>.  The value is copied
   to the buffer indicated in <value>, of size <size>. True is returned
   on success.  If <path> == NULL, then <v> is read.  */

   reg_var *var;

   var = reg_block_resolve(r, v, path);
   if(var == NULL || var->type != REG_STRING) {
      return(false);
   }

   if(value != NULL && size > 0) {
      strcopyb(value, var->value.string, size);
   }
   return(true);

}



bool reg_set_integer(reg *r, reg_var *v, const char *path, int value) {
/* reg_set_integer
   Writes the integer <value> into the variable <path>, relative to <v>.
   True is returned on success.  If <path> == NULL, then modifications
   occur directly on <v>.  */

   char varname[REG_SYMBOL_SIZE];
   reg_var *block;

   block = reg_block_resolve_container(r, v, path, varname);
   if(block == NULL) return(false);

   return(reg_var_set_integer(r, block, varname, value));

}



bool reg_set_doublev(reg *r, reg_var *v, const char *path, double value) {
/* reg_set_doublev
   Writes the double <value> into the variable <path>, relative to <v>.
   True is returned on success.  If <path> == NULL, then modifications
   occur directly on <v>.  */

   char varname[REG_SYMBOL_SIZE];
   reg_var *block;

   block = reg_block_resolve_container(r, v, path, varname);
   if(block == NULL) return(false);

   return(reg_var_set_doublev(r, block, varname, value));

}



bool reg_set_boolean_f(reg *r, reg_var *v, const char *path, bool value, reg_format_bool format) {
/* reg_set_boolean_f
   Writes the boolean <value> into the variable <path>, relative to <v>.
   True is returned on success.  If <path> == NULL, then modifications
   occur directly on <v>.  */

   char varname[REG_SYMBOL_SIZE];
   reg_var *block;

   block = reg_block_resolve_container(r, v, path, varname);
   if(block == NULL) return(false);

   return(reg_var_set_boolean(r, block, varname, value, format));

}



bool reg_set_boolean(reg *r, reg_var *v, const char *path, bool value) {
/* reg_set_boolean */

   return(reg_set_boolean_f(r, v, path, value, REG_FORMAT_BOOL_DEFAULT));

}



bool reg_set_string(reg *r, reg_var *v, const char *path, const char *value) {
/* reg_set_string
   Writes the NULL-terminated string buffer <value> into the variable
   <path>, relative to <v>.  True is returned on success.  In case
   <path> == NULL, the modifications will occur directly on <v>.  */

   char varname[REG_SYMBOL_SIZE];
   reg_var *block;

   block = reg_block_resolve_container(r, v, path, varname);
   if(block == NULL) return(false);

   return(reg_var_set_string(r, block, varname, value));

}



bool reg_set_block(reg *r, reg_var *v, const char *path, const char *klass) {
/* reg_set_block
   Constructs a new block with the class named in <klass>.  If <path>
   is NULL, then <r> becomes a new block.  If <klass> is null, then
   an NULL-classed object will be created.  */

   char varname[REG_SYMBOL_SIZE];
   reg_var *block;
   reg_var *tmp;

   block = reg_block_resolve_container(r, v, path, varname);
   if(block == NULL) return(false);

   tmp = reg_var_new_block(r, varname, reg_class_lookup(r->classes, klass));
   return(reg_var_set_block(r, block, varname, &tmp));

}



static bool _reg_set_var(reg *r, reg_var *v, const char *path,
                         const reg_var_data *data, bool overwrite) {
/* reg_set_var
   Sets the variable to the value based on the description in data.
   If overwrite is not true, then this function will not attempt to
   replace any existing value binding.  This function returns true
   if the assignment is successful, OR if overwrite = false and the
   value was already defined.  It returns false in other cases.

   The data contains a reg_value which is used to initialize the
   variable.  As such, it is able to handle multiple types of
   data.  This function can only define integers, floats, booleans
   and strings at this time.  It cannot bulk-define variables that
   represent subclasses yet, or variant variables.  */

   char thispath[REG_SYMBOL_SIZE];

   /* Construct the final name of the variable to define. */
   if(path == NULL) {
      strcopyb(thispath, data->name, sizeof(thispath));
   } else {
      sbprintf(thispath, sizeof(thispath), "%s/%s", path, data->name);
   }

   /* Figure out if we would be overwriting an existing var */
   if(!overwrite && reg_get_var(r, v, thispath) != NULL) {
      /* This is not considered an error case */
      return(true);
   }

   /* Assign the variable, depending on the type of data */
   switch(data->type) {
   case REG_INTEGER:
      return(reg_set_integer(r, v, thispath, data->value.integer));
   case REG_DOUBLEV:
      return(reg_set_doublev(r, v, thispath, data->value.doublev));
   case REG_BOOLEAN:
      return(reg_set_boolean_f(r, v, thispath, data->value.boolean.value,
                               data->value.boolean.format));
   case REG_STRING:
      return(reg_set_string(r, v, thispath, data->value.string));
   case REG_BLOCK:
   case REG_ANY:
      break;
   }

   /* If we're here, then something went wrong */
   return(false);

}



static bool _reg_set_vars(reg *r, reg_var *v, const char *path,
                          const reg_var_data *data, bool overwrite) {
/* reg_set_vars
   This function takes a list of reg_var_data's in data, which is
   terminated by a record whose name field is NULL.  It assigns
   each of the names given, relative to the reg_var v.  If path
   is not NULL, then it is prepended to each name in turn to
   construct the final variable name (which itself is relative
   to v).  If v is NULL, then the topmost variable in the registry
   is used.

   For each entry in the data list, this sets the variable to the
   value based on the description in data.  If overwrite is not
   true, then this function will not attempt to replace any existing
   value binding.  This function returns true if ALL assignments are
   successful (an assignment is successful if the variable is either
   reassigned, or the variable was already bound and overwrite was
   false).  If any single assignemnt fails, then this function will
   attempt to assign the remainder of the variables in the data
   list, but will ultimately return false.

   Each entry in the data list contains a reg_value which is used to
   initialize the variable.  As such, it is able to handle multiple
   types of data.  This function can only define integers, floats,
   booleans and strings at this time.  It cannot bulk-define
   variables that represent subclasses yet, or variant variables.  */

   bool success = true;

   if(data == NULL) return(false);

   while(data->name != NULL) {
      success = _reg_set_var(r, v, path, data, overwrite) || success;
      ++data;
   }

   return(success);

}



bool reg_set_vars(reg *r, reg_var *v, const char *path, const reg_var_data *data) {
/* reg_set_vars
   This function takes a list of reg_var_data's in data, which is
   terminated by a record whose name field is NULL.  It assigns
   each of the names given, relative to the reg_var v.  If path
   is not NULL, then it is prepended to each name in turn to
   construct the final variable name (which itself is relative
   to v).  If v is NULL, then the topmost variable in the registry
   is used.

   For each entry in the data list, this sets the variable to the
   value based on the description in data.  This function will
   overwrite existing bindings in the registry.  This function
   returns true if ALL assignments are successful (an assignment is
   successful if the variable is actually reassigned).  If any
   single assignemnt fails, then this function will attempt to
   assign the remainder of the variables in the data list, but
   will ultimately return false.

   Each entry in the data list contains a reg_value which is used to
   initialize the variable.  As such, it is able to handle multiple
   types of data.  This function can only define integers, floats,
   booleans and strings at this time.  It cannot bulk-define
   variables that represent subclasses yet, or variant variables.  */

   return(_reg_set_vars(r, v, path, data, true));

}



bool reg_set_var_defaults(reg *r, reg_var *v, const char *path, const reg_var_data *data) {
/* reg_set_var_defaults
   This function takes a list of reg_var_data's in data, which is
   terminated by a record whose name field is NULL.  It assigns
   each of the names given, relative to the reg_var v.  If path
   is not NULL, then it is prepended to each name in turn to
   construct the final variable name (which itself is relative
   to v).  If v is NULL, then the topmost variable in the registry
   is used.

   For each entry in the data list, this sets the variable to the
   value based on the description in data.  This function will
   ONLY assign variables which are not already bound in the
   registry; as such, it is suitable for assigning "default"
   values to a class and its subclasses, once the class has
   already been loaded from disk.  This function returns true if
   ALL assignments are successful (an assignment is successful if
   the variable is either reassigned, or was already bound).  If
   any single assignemnt fails, then this function will attempt to
   assign the remainder of the variables in the data list, but
   will ultimately return false.

   Each entry in the data list contains a reg_value which is used to
   initialize the variable.  As such, it is able to handle multiple
   types of data.  This function can only define integers, floats,
   booleans and strings at this time.  It cannot bulk-define
   variables that represent subclasses yet, or variant variables.  */

   return(_reg_set_vars(r, v, path, data, false));

}



bool reg_set_var_class_defaults(reg *r, reg_var *v, const char *path, const reg_class_default_data *data) {
/* reg_set_var_defaults
   This function takes a list of reg_class_default_data's, which is
   terminated by a record whose name field is NULL.  It assigns
   each of the names given, relative to the reg_var v.  If path
   is not NULL, then it is prepended to each name in turn to
   construct the final variable name (which itself is relative
   to v).  If v is NULL, then the topmost variable in the registry
   is used.

   For each entry in the data list, this sets the variable to the
   value based on the description in data.  This function will
   ONLY assign variables which are not already bound in the
   registry; as such, it is suitable for assigning "default"
   values to a class and its subclasses, once the class has
   already been loaded from disk.  This function returns true if
   ALL assignments are successful (an assignment is successful if
   the variable is either reassigned, or was already bound).  If
   any single assignemnt fails, then this function will attempt to
   assign the remainder of the variables in the data list, but
   will ultimately return false.

   Each entry in the data list contains a reg_value which is used to
   initialize the variable.  As such, it is able to handle multiple
   types of data.  This function can only define integers, floats,
   booleans and strings at this time.  It cannot bulk-define
   variables that represent subclasses yet, or variant variables.  */

   bool success = true;

   if(data == NULL) return(false);

   while(data->default_info.name != NULL) {
      success = _reg_set_var(r, v, path, &data->default_info, false) || success;
      ++data;
   }

   return(success);

}
