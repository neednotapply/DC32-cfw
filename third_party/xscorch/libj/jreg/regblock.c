/* $Header: /fridge/cvs/xscorch/libj/jreg/regblock.c,v 1.11 2009-04-26 17:39:27 jacob Exp $ */
/*

   libj - regblock.c          Copyright(c) 2000-2003 Justin David Smith
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



reg_class *reg_class_new(reg_class **classes, const char *name) {
/* reg_class_new
   Creates a new block class object.  Each block class holds the
   list of variables and their data types, for the given class.
   The new class is prepended to the <classes> linked list.  */

   reg_class *bc;

   if(classes == NULL || name == NULL) return(NULL);

   bc = (reg_class *)malloc(sizeof(reg_class));
   if(bc == NULL) return(NULL);

   strcopyb(bc->name, name, REG_SYMBOL_SIZE);
   bc->vars = NULL;
   bc->next = *classes;
   *classes = bc;
   return(bc);

}



reg_class *reg_class_lookup(reg_class *classes, const char *name) {
/* reg_class_lookup
   Look for the block class with given name.  */

   while(classes != NULL) {
      if(strequal(classes->name, name)) return(classes);
      classes = classes->next;
   }
   return(NULL);

}



void reg_class_release(reg_class **classes) {
/* reg_class_release
   Release the list of block classes.  */

   reg_class *cur;

   if(classes == NULL) return;
   while(*classes != NULL) {
      cur = *classes;
      *classes = cur->next;
      reg_var_info_release(&cur->vars);
      free(cur);
   }

}



reg_block *reg_block_new(const char *name, const reg_class *klass) {
/* reg_block_new
   Creates a new subblock, of the specified name and class.  */

   reg_block *b;

   b = (reg_block *)malloc(sizeof(reg_block));
   if(b == NULL) return(NULL);

   strcopyb(b->name, name, REG_SYMBOL_SIZE);
   b->klass = klass;
   b->vars  = NULL;

   return(b);

}



void reg_block_free(reg_block **b) {
/* reg_block_free
   Releases an entire block.  */

   if(b == NULL || *b == NULL) return;

   while((*b)->vars != NULL) {
      reg_var_free(&(*b)->vars);
   }

   free(*b);
   *b = NULL;

}



reg_var *reg_block_resolve(reg *r, reg_var *v, const char *path) {
/* reg_block_resolve
   Resolve the specified pathname, relative to <v>.  If <path> == NULL,
   then <v> itself is returned.  Otherwise, the <path> is resolved,
   relative to <v>; if an error occurs along the way, then NULL will
   be returned.  Note, if <v> is not a block, and <path> is not NULL,
   then an error occurs.  */

   char firstelem[REG_SYMBOL_SIZE];
   reg_var *match;
   const char *p;

   if(v == NULL) v = r->top;
   if(v == NULL) return(NULL);
   if(path == NULL) return(v);
   if(v->type != REG_BLOCK) return(NULL);

   p = path;
   while(*p != '\0' && *p != '/') ++p;
   strcopynb(firstelem, path, p - path, REG_SYMBOL_SIZE);

   match = reg_var_lookup(v->value.block->vars, firstelem);
   if(match == NULL) return(NULL);
   if(*p == '\0') return(match);
   return(reg_block_resolve(r, match, p + 1));

}



reg_var *reg_block_resolve_container(reg *r, reg_var *v, const char *path, char *varname) {
/* reg_block_resolve_container
   Resolve the container which holds the specified variable.
   Lookup rules are the same as for reg_block_resolve().  The
   final variable name may be copied into <varname> (which
   must be of size REG_SYMBOL_SIZE) if successful.

   Caution:  If <path> is NULL, then a container _cannot_ be
   returned for this variable.  This is a bug; I should be
   keeping a backpointer around, but I don't.  Oops.  */

   char firstelem[REG_SYMBOL_SIZE];
   reg_var *match;
   const char *p;

   if(v == NULL) v = r->top;
   if(v == NULL || path == NULL) return(NULL);
   if(v->type != REG_BLOCK) return(NULL);

   p = path;
   while(*p != '\0' && *p != '/') ++p;
   if(*p == '\0') {
      /* Found the container */
      if(varname != NULL) {
         strcopynb(varname, path, p - path, REG_SYMBOL_SIZE);
      }
      return(v);
   }

   strcopynb(firstelem, path, p - path, REG_SYMBOL_SIZE);
   match = reg_var_lookup(v->value.block->vars, firstelem);

   if(match == NULL) return(NULL);
   return(reg_block_resolve_container(r, match, p + 1, varname));

}



bool reg_class_add(reg *r, const char *classname) {
/* sc_file_class_add
   Add a new (empty) class to the registry.  */

   if(r == NULL || classname == NULL) return(false);
   return(reg_class_new(&r->classes, classname) != NULL);

}



bool reg_class_register_var(reg *r, const char *classname, const char *name,
                            reg_type type, const char *klass) {
/* reg_class_register_var
   Register a variable with name <name>, and specified <type> (and <klass>
   if a block), to the class entry <classname> in <r>.  NULL pointers are
   not tolerated by this function, except for <klass>.  If usedef is true,
   then the variable is registered with the indicated default value; other-
   wise, defvalue is ignored.  */

   reg_class *bc;

   if(r == NULL || classname == NULL || name == NULL) {
      return(false);
   }
   bc = reg_class_lookup(r->classes, classname);
   if(bc == NULL) {
      return(false);
   }
   if(reg_var_info_new(&bc->vars, name, type, reg_class_lookup(r->classes, klass)) == NULL) {
      return(false);
   }
   return(true);

}



bool reg_class_add_list(reg *r, const char **classnames) {
/* reg_class_add_list
   Register a list of empty classes.  <classnames> is a NULL-
   terminated list of names to register.  */

   if(classnames == NULL) return(false);
   while(*classnames != NULL) {
      if(!reg_class_add(r, *classnames)) return(false);
      ++classnames;
   }
   return(true);

}



bool reg_class_register_vars(reg *r, const char *classname, const reg_class_data *list) {
/* reg_class_register_vars */

   if(list == NULL) return(false);
   while(list->name != NULL) {
      if(!reg_class_register_var(r, classname, list->name, list->type, list->klass)) {
         return(false);
      }
      ++list;
   }
   return(true);

}



bool reg_class_register_default_vars(reg *r, const char *classname, const reg_class_default_data *list) {
/* reg_class_register_default_vars */

   if(list == NULL) return(false);
   while(list->class_info.name != NULL) {
      if(!reg_class_register_var(r, classname, list->class_info.name,
                                 list->class_info.type, list->class_info.klass)) {
         return(false);
      }
      ++list;
   }
   return(true);

}



bool reg_class_register_list(reg *r, const reg_class_list *list) {
/* reg_class_register_list */

   if(list == NULL) return(false);
   while(list->name != NULL) {
      if(!reg_class_add(r, list->name)) return(false);
      if(!reg_class_register_vars(r, list->name, list->vars)) return(false);
      ++list;
   }
   return(true);

}



bool reg_class_register_default_list(reg *r, const reg_class_default_list *list) {
/* reg_class_register_default_list */

   if(list == NULL) return(false);
   while(list->name != NULL) {
      if(!reg_class_add(r, list->name)) return(false);
      if(!reg_class_register_default_vars(r, list->name, list->vars)) return(false);
      ++list;
   }
   return(true);

}



reg_var *reg_get_block_head(reg_var *v) {

   if(v == NULL) return(NULL);
   if(v->type != REG_BLOCK) return(NULL);
   return(v->value.block->vars);

}



reg_var *reg_get_next_var(reg_var *v) {

   if(v == NULL) return(NULL);
   return(v->next);

}

