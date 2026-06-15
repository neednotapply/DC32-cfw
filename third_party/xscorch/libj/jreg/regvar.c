/* $Header: /fridge/cvs/xscorch/libj/jreg/regvar.c,v 1.10 2009-04-26 17:39:28 jacob Exp $ */
/*

   libj - regvar.c            Copyright(c) 2000-2003 Justin David Smith
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



/***  Variables   ***/



reg_var_info *reg_var_info_new(reg_var_info **reg, const char *name,
                               reg_type type, const reg_class *klass) {
/* reg_var_info_new */

   reg_var_info *new;

   if(reg == NULL) return(NULL);

   new = (reg_var_info *)malloc(sizeof(reg_var_info));
   if(new == NULL) return(NULL);

   strcopyb(new->name,  name, REG_SYMBOL_SIZE);
   new->klass = klass;
   new->type = type;
   new->next = *reg;

   /* Prepend the new variable definition */
   *reg = new;
   return(new);

}



reg_type reg_var_info_lookup(const reg_class *bc, const char *name) {
/* reg_var_info_lookup */

   const reg_var_info *reg;

   if(bc == NULL || name == NULL) return(REG_ANY);
   reg = bc->vars;
   while(reg != NULL) {
      if(strequal(reg->name, name)) return(reg->type);
      reg = reg->next;
   }
   return(REG_ANY);

}



const reg_class *reg_var_info_lookup_class(const reg_class *bc, const char *name) {
/* reg_var_info_lookup_class */

   const reg_var_info *reg;

   if(bc == NULL || name == NULL) return(NULL);
   reg = bc->vars;
   while(reg != NULL) {
      if(strequal(reg->name, name)) return(reg->klass);
      reg = reg->next;
   }
   return(NULL);

}



static void _reg_value_free(reg_type type, reg_value *value) {
/* reg_value_free */

   switch(type) {
      case REG_STRING:
         free(value->string);
         value->string = NULL;
         break;

      case REG_BLOCK:
         reg_block_free(&value->block);
         break;

      case REG_INTEGER:
      case REG_BOOLEAN:
      case REG_DOUBLEV:
      case REG_ANY:
         break;
   }
}



void reg_var_info_release(reg_var_info **reg) {
/* reg_var_info_release */

   reg_var_info *cur;

   if(reg == NULL || *reg == NULL) return;
   while(*reg != NULL) {
      cur = *reg;
      *reg = cur->next;
      free(cur);
   }

}



static void _reg_var_clear(reg_var *var) {
/* reg_var_clear */

   if(var == NULL) return;
   _reg_value_free(var->type, &var->value);

}



reg_var *reg_var_lookup(reg_var *vars, const char *name) {
/* reg_var_lookup
   Checks to see if the specified variable is already defined.  If it
   is, then we will return it here.  */

   /* Make sure all pointers given are valid */
   if(name == NULL) return(NULL);

   /* Search for the appropriate variable */
   while(vars != NULL) {
      if(strequal(vars->name, name)) {
         /* Found a matching variable */
         return(vars);
      }
      /* Try the next variable... */
      vars = vars->next;
   } /* Searching... */

   /* No match found */
   return(NULL);

}



static reg_var *_reg_var_add(const reg *r, reg_var **vars, const char *name, reg_type type) {
/* reg_var_add
   Adds a new variable definition to the end of the linked list given.
   The variable will be uninitialised, but its type field will be set.
   This function may fail on an allocation error, or if name/type is
   invalid, etc...  */

   reg_var *var;       /* Newly constructed variable */
   reg_var *cur;       /* Pointer into linked list... */

   /* Make sure all pointers given are valid */
   if(name == NULL) return(NULL);
   if(vars == NULL) return(NULL);

   /* Make sure the name is valid, and not duplicated */
   if(*name == '\0') {
      reg_error(r, "Variable name is empty");
      return(NULL);
   } /* Name empty? */

   /* Create the new variable */
   var = (reg_var *)malloc(sizeof(reg_var));
   if(var == NULL) return(NULL);

   /* Initialise the data members */
   strcopyb(var->name, name, REG_SYMBOL_SIZE);
   var->type         = type;
   var->value.string = NULL;
   var->next         = NULL;

   /* Insert this variable to the list */
   if(*vars == NULL) {
      /* First variable in the list */
      *vars = var;
   } else {
      /* Search for end of list */
      cur = *vars;
      while(cur->next != NULL) cur = cur->next;
      cur->next = var;
   }

   /* Return the constructed variable */
   return(var);

}



static reg_var *_reg_var_set(const reg *r, reg_var *b, const char *name, reg_type type) {
/* reg_var_set */

   reg_var *var;       /* Variable created or to set */
   reg_type regtype;   /* Variable type required by vars */
   reg_block *blk;     /* Block data */

   /* Make sure type agrees with vars (if applicable) */
   blk = b->value.block;
   regtype = reg_var_info_lookup(blk->klass, name);
   if(regtype != REG_ANY && regtype != type) {
      /* Sorry, types don't match */
      reg_error1(r, "Type must match vars for \"%s\"", name);
      return(NULL);
   } /* Does type agree with vars? */

   /* Check if the variable has already been defined */
   var = reg_var_lookup(blk->vars, name);
   if(var != NULL) {
      /* Variable already exists */
      _reg_var_clear(var);
      var->type = type;
   } else {
      /* Variable did not exist; create it */
      var = _reg_var_add(r, &blk->vars, name, type);
   }

   /* Return the variable */
   return(var);

}



bool reg_var_set_integer(const reg *r, reg_var *b, const char *name, int value) {
/* reg_var_set_integer
   Adds a new integer variable.  */

   reg_var *var;       /* Newly constructed variable */

   /* Create the variable and initialise its value */
   var = _reg_var_set(r, b, name, REG_INTEGER);
   if(var == NULL) return(false);
   var->value.integer = value;
   return(true);

}



bool reg_var_set_doublev(const reg *r, reg_var *b, const char *name, double value) {
/* reg_var_set_doublev
   Adds a new floating-point variable.  */

   reg_var *var;       /* Newly constructed variable */

   /* Create the variable and initialise its value */
   var = _reg_var_set(r, b, name, REG_DOUBLEV);
   if(var == NULL) return(false);
   var->value.doublev = value;
   return(true);

}



bool reg_var_set_boolean(const reg *r, reg_var *b, const char *name, bool value, reg_format_bool format) {
/* reg_var_set_boolean
   Adds a new boolean variable (with associated format).  */

   reg_var *var;       /* Newly constructed variable */

   /* Create the variable and initialise its value */
   var = _reg_var_set(r, b, name, REG_BOOLEAN);
   if(var == NULL) return(false);
   var->value.boolean.value  = value;
   var->value.boolean.format = format;
   return(true);

}



bool reg_var_set_string(const reg *r, reg_var *b, const char *name, const char *value) {
/* reg_var_set_string
   Adds a new string variable.  */

   reg_var *var;       /* Newly constructed variable */

   /* Construct the variable */
   var = _reg_var_set(r, b, name, REG_STRING);
   if(var == NULL) return(false);

   /* Copy the new string into the variable */
   if(value == NULL) {
      var->value.string = NULL;
   } else {
      /* Source string was not NULL */
      var->value.string = (char *)malloc(strlenn(value) + 1);
      if(var->value.string != NULL) {
         strcopy(var->value.string, value);
      } /* Did malloc() succeed? */
   } /* Was source value nonnull? */
   return(true);

}



bool reg_var_set_block(const reg *r, reg_var *b, const char *name, reg_var **block) {
/* reg_var_set_block
   Adds a new block variable.  */

   reg_var *var;       /* Newly constructed variable */
   const reg_class *klass;  /* Block class in vars */
   reg_block *blk;     /* Original block object */
   reg_block *newblk;  /* New block object to assign */

   /* Assert that b,  is a block. */
   if(b == NULL || b->type != REG_BLOCK) return(false);
   if(block == NULL || *block == NULL || (*block)->type != REG_BLOCK) return(false);
   blk = b->value.block;
   newblk = (*block)->value.block;

   /* Make sure class agrees with vars (if applicable) */
   klass = reg_var_info_lookup_class(blk->klass, name);
   if(klass != NULL && newblk->klass != klass) {
      /* Sorry, types don't match */
      reg_error1(r, "Object class must match vars for \"%s\"", name);
      return(false);
   } /* Does type agree with vars? */

   /* Create the variable and assign the block to it */
   var = _reg_var_set(r, b, name, REG_BLOCK);
   if(var == NULL) return(false);
   var->value.block = newblk;

   /* Delete the original */
   (*block)->value.block = NULL;
   reg_var_free(block);
   return(true);

}



bool reg_var_merge_block(const reg *r, reg_var *b, const char *name, reg_var **block) {
/* reg_var_merge_block
   Merges the given block into an existing block (if available).  If
   necessary, a new block will be created.  Variables in <block> will
   override the old vars in <b> that were set.  WARNING: This function may
   destroy the block pointer given, after it is finished.  */

   reg_var *var;       /* Newly constructed variable */
   reg_var **newlist;  /* New variable list to merge */
   reg_block *blk;     /* Original block object */
   reg_block *newblk;  /* New block object to merge */
   bool advance;       /* Advance pointer? */

   /* Make sure pointers are valid */
   if(b == NULL || block == NULL || *block == NULL) return(false);

   /* Make sure b, block are valid blocks */
   if(b->type != REG_BLOCK || (*block)->type != REG_BLOCK) return(false);
   blk = b->value.block;
   newblk = (*block)->value.block;

   /* Search for an existing block */
   var = reg_var_lookup(blk->vars, name);
   if(var == NULL || var->type != REG_BLOCK || var->value.block->klass != newblk->klass) {
      /* No variable by that name found, or it wasn't a block */
      return(reg_var_set_block(r, b, name, block));
   } /* Does variable exist? Is it a block? */

   /* Variable is a block; we must merge given block into it */
   newlist = &newblk->vars;
   while(*newlist != NULL) {
      advance = true;
      switch((*newlist)->type) {
         case REG_INTEGER:
            if(!reg_var_set_integer(r, var, (*newlist)->name, (*newlist)->value.integer)) return(false);
            break;

         case REG_DOUBLEV:
            if(!reg_var_set_doublev(r, var, (*newlist)->name, (*newlist)->value.doublev)) return(false);
            break;

         case REG_BOOLEAN:
            if(!reg_var_set_boolean(r, var, (*newlist)->name, (*newlist)->value.boolean.value, (*newlist)->value.boolean.format)) return(false);
            break;

         case REG_STRING:
            if(!reg_var_set_string(r, var, (*newlist)->name, (*newlist)->value.string)) return(false);
            break;

         case REG_BLOCK:
            if(!reg_var_merge_block(r, var, (*newlist)->name, newlist)) return(false);
            advance = false;
            break;

         case REG_ANY:
            break;
      }
      if(advance) newlist = &((*newlist)->next);
   } /* Merging ... */

   /* Destroy the block */
   reg_var_free(block);

   /* Return success */
   return(true);

}



bool reg_var_set_by_value(const reg *r, reg_var *b, const char *name, char *value) {
/* reg_var_set_by_value
   Adds a new variable, based on the contents of the string value.  First,
   this function attempts to set an integer variable; failing that, it will
   try seting a double variable, and as a last resort it uses a string
   variable.  */

   char *p;                /* Pointer to store result of strto? */
   int valint;             /* Attempted conversion to integer */
   double valdbl;          /* Attempted conversion to double */

   /* Make sure value isn't NULL */
   if(value == NULL || *value == '\0') return(false);

   /* Check for True/False and other boolean keywords */
   if(strequal(value, "True")  || strequal(value, "true"))  return(reg_var_set_boolean(r, b, name, true, REG_FORMAT_BOOL_TF));
   if(strequal(value, "Yes")   || strequal(value, "yes"))   return(reg_var_set_boolean(r, b, name, true, REG_FORMAT_BOOL_YN));
   if(strequal(value, "On")    || strequal(value, "on"))    return(reg_var_set_boolean(r, b, name, true, REG_FORMAT_BOOL_OO));
   if(strequal(value, "T")     || strequal(value, "t"))     return(reg_var_set_boolean(r, b, name, true, REG_FORMAT_BOOL_TNIL));
   if(strequal(value, "False") || strequal(value, "false")) return(reg_var_set_boolean(r, b, name, false, REG_FORMAT_BOOL_TF));
   if(strequal(value, "No")    || strequal(value, "no"))    return(reg_var_set_boolean(r, b, name, false, REG_FORMAT_BOOL_YN));
   if(strequal(value, "Off")   || strequal(value, "off"))   return(reg_var_set_boolean(r, b, name, false, REG_FORMAT_BOOL_OO));
   if(strequal(value, "NIL")   || strequal(value, "nil"))   return(reg_var_set_boolean(r, b, name, false, REG_FORMAT_BOOL_TNIL));

   /* Attempt to convert to an integer first (most restrictive) */
   valint = strtol(value, &p, 0);
   if(*p == '\0') return(reg_var_set_integer(r, b, name, valint));

   /* Attempt to convert to a double */
   valdbl = strtod(value, &p);
   if(*p == '\0') return(reg_var_set_doublev(r, b, name, valdbl));

   /* Assume the value is a string if it has a quote. */
   if(*value == '\"') {
      unescape_quoted(value);
      return(reg_var_set_string(r, b, name, value));
   }

   /* Failure mode */
   reg_error2(r, "Malformed value for \"%s\": %s\n", name, value);
   return(false);

}



void reg_var_free(reg_var **vars) {
/* reg_var_free
   Removes the variable at the top of the list.  */

   reg_var *var;       /* Variable being deleted */

   /* Make sure there is something to delete */
   if(vars == NULL || *vars == NULL) return;

   /* Splice, and update pointers in the list */
   var = *vars;
   *vars = var->next;

   /* Release any memory associated with the variable */
   _reg_var_clear(var);
   free(var);

}



reg_var *reg_var_new_block(const reg *r, const char *name, const reg_class *klass) {

   reg_var *var;

   var = NULL;
   _reg_var_add(r, &var, name, REG_BLOCK);
   if(var != NULL) {
      var->value.block = reg_block_new(name, klass);
   }
   return(var);

}



reg_type reg_get_var_type(const reg_var *v) {

   if(v == NULL) return(REG_ANY);
   return(v->type);

}



const char *reg_get_var_name(const reg_var *v) {

   if(v == NULL) return(NULL);
   return(v->name);

}



const char *reg_get_var_class(const reg_var *v) {

   if(v == NULL) return(NULL);
   if(v->type != REG_BLOCK) return(NULL);
   if(v->value.block->klass == NULL) return(REG_NULL);
   return(v->value.block->klass->name);

}



reg_var *reg_get_var(reg *r, reg_var *v, const char *path) {

   return(reg_block_resolve(r, v, path));

}

