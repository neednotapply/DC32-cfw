/* $Header: /fridge/cvs/xscorch/libj/jreg/regio.c,v 1.9 2009-04-26 17:39:27 jacob Exp $ */
/*

   libj - regio.c             Copyright(c) 2000-2003 Justin David Smith
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
static void _reg_save_block(reg *r, const reg_var *b, int indent);



/***  Line processing   ***/



static bool _sc_line_is_assignment(char *buf, char *var, char *val) {
/* sc_line_is_assignment */

   char *p;

   p = escaped_scan(buf, '=');
   if(p == NULL) return(false);

   strcopyb(var, buf, p - buf + 1);
   strcopyb(val, p + 1, REG_BUFFER);
   trim(var);
   trim(val);
   return(true);

}



static bool _sc_line_is_block_begin(const reg *r, char *buf, char *name, char *klass) {
/* sc_line_is_block_begin */

   char *p;

   p = escaped_scan(buf, '{');
   if(p == NULL) return(false);

   *p++ = '\0';
   trim(p);
   if(*p != '\0') {
      reg_error_line(r, "Garbage after '{' (warning only)");
   }

   if(!_sc_line_is_assignment(buf, name, klass)) {
      reg_error_line(r, "Block needs a name and a class");
      return(false);
   }

   return(true);

}



static bool _sc_line_is_block_end(const reg *r, char *buf) {
/* sc_line_is_block_end */

   char *p;

   p = escaped_scan(buf, '}');
   if(p == NULL) return(false);

   *p++ = '\0';
   trim(p);
   if(*p != '\0') {
      reg_error_line(r, "Garbage after '}' (warning only)");
   }

   trim(buf);
   if(*buf != '\0') {
      reg_error_line(r, "Garbage before '}' (warning only)");
   }

   return(true);

}



/***  File block readers   ***/



static reg_var *_reg_load_block(reg *r, const char *name, const char *klass) {

   bool exit;
   int startline;
   reg_var *block;
   reg_var *subblock;
   char buf[REG_BUFFER];
   char var[REG_BUFFER];
   char val[REG_BUFFER];

   startline = r->line;
   block = reg_var_new_block(r, name, reg_class_lookup(r->classes, klass));
   if(block == NULL) return(NULL);

   exit = false;
   while(!exit && fgets(buf, REG_BUFFER, r->handle) != NULL) {
      ++r->line;
      escaped_chop(buf, ';');
      trim(buf);

      if(_sc_line_is_block_begin(r, buf, var, val)) {
         subblock = _reg_load_block(r, var, val);
         if(subblock == NULL) {
            reg_error_line2(r, "   in block \"%s\"(%d)", name, startline);
            reg_var_free(&block);
            return(false);
         }
         if(!reg_var_merge_block(r, block, var, &subblock)) {
            reg_error_line3(r, "Failed to add subblock \"%s\" to block \"%s\"(%d) (continuable error)",
                            var, name, startline);
         }

      } else if(_sc_line_is_block_end(r, buf)) {
         exit = true;

      } else if(_sc_line_is_assignment(buf, var, val)) {
         if(!reg_var_set_by_value(r, block, var, val)) {
            reg_error_line3(r, "Failed to add variable \"%s\" to block \"%s\"(%d) (continuable error)",
                            var, name, startline);
         }

      } else if(*buf == '\0') {
         /* Do nothing */

      } else {
         /* Invalid line */
         reg_error_line2(r, "Parse error in block \"%s\"(%d)", name, startline);
         reg_var_free(&block);
         return(false);

      } /* What type of line is this? */

   } /* Reading lines from the file ... */

   if(!exit) {
      reg_error_line2(r, "Premature end-of-block \"%s\"(%d)", name, startline);
      reg_var_free(&block);
      return(false);
   }

   /* Return this block */
   return(block);

}



static bool _reg_load_top(reg *r) {

   reg_var *top;
   reg_var *subblock;
   char buf[REG_BUFFER];
   char var[REG_BUFFER];
   char val[REG_BUFFER];

   top = r->top;

   while(fgets(buf, REG_BUFFER, r->handle) != NULL) {
      ++r->line;
      escaped_chop(buf, ';');
      trim(buf);

      if(_sc_line_is_block_begin(r, buf, var, val)) {
         subblock = _reg_load_block(r, var, val);
         if(subblock == NULL) {
            reg_error_line1(r, "   in block \"%s\"", top->name);
            return(false);
         }
         if(!reg_var_merge_block(r, top, var, &subblock)) {
            reg_error_line2(r, "Failed to add subblock \"%s\" to top block \"%s\" (continuable error)",
                            var, top->name);
         }

      } else if(_sc_line_is_block_end(r, buf)) {
         reg_error_line1(r, "Cannot exit top block, \"%s\" (continuable error)", top->name);

      } else if(_sc_line_is_assignment(buf, var, val)) {
         if(!reg_var_set_by_value(r, top, var, val)) {
            reg_error_line2(r, "Failed to add variable \"%s\" to block \"%s\" (continuable error)",
                            var, top->name);
         }

      } else if(*buf == '\0') {
         /* Do nothing */

      } else {
         /* Invalid line */
         reg_error_line1(r, "Parse error in block \"%s\"", top->name);
         return(false);

      } /* What type of line is this? */

   } /* Reading lines from the file ... */

   return(true);

}



/***  Block saving functions  ***/



static inline void _reg_indent(FILE *f, int indent) {

   while(indent-- > 0) fputc(' ', f);

}



static void _reg_save_variable(reg *r, const reg_var *var, int indent) {

   char buf[REG_BUFFER];

   switch(var->type) {
      case REG_INTEGER:
         _reg_indent(r->handle, indent);
         fprintf(r->handle, "%-16s = %d;\n", var->name, var->value.integer);
         break;

      case REG_DOUBLEV:
         _reg_indent(r->handle, indent);
         fprintf(r->handle, "%-16s = %e;\n", var->name, var->value.doublev);
         break;

      case REG_BOOLEAN:
         _reg_indent(r->handle, indent);
         switch(var->value.boolean.format) {
            case REG_FORMAT_BOOL_TF:
               fprintf(r->handle, "%-16s = %s;\n", var->name, (var->value.boolean.value ? "True" : "False"));
               break;
            case REG_FORMAT_BOOL_YN:
               fprintf(r->handle, "%-16s = %s;\n", var->name, (var->value.boolean.value ? "Yes" : "No"));
               break;
            case REG_FORMAT_BOOL_OO:
               fprintf(r->handle, "%-16s = %s;\n", var->name, (var->value.boolean.value ? "On" : "Off"));
               break;
            case REG_FORMAT_BOOL_TNIL:
               fprintf(r->handle, "%-16s = %s;\n", var->name, (var->value.boolean.value ? "T" : "NIL"));
               break;
         }
         break;

      case REG_STRING:
         strcopyb(buf, var->value.string, REG_BUFFER);
         escapeb(buf, REG_BUFFER);

         _reg_indent(r->handle, indent);
         fprintf(r->handle, "%-16s = \"%s\";\n", var->name, buf);
         break;

      case REG_BLOCK:
         _reg_indent(r->handle, indent);
         fprintf(r->handle, "%s = %s {\n", var->name, var->value.block->klass == NULL ? "null" : var->value.block->klass->name);

         _reg_save_block(r, var, indent + 3);
         _reg_indent(r->handle, indent);
         fprintf(r->handle, "}; End of block %s\n", var->name);
         if(indent == 0) fprintf(r->handle, "\n\n");
         break;

      case REG_ANY:
         break;
   }

}



static void _reg_save_block(reg *r, const reg_var *b, int indent) {

   reg_var *var;

   var = b->value.block->vars;
   while(var != NULL) {
      _reg_save_variable(r, var, indent);
      var = var->next;
   }

}



/***  General interface    ***/



void reg_set_name(reg *r, const char *filename) {

   char *oldfilename;

   if(r == NULL || filename == NULL) return;

   oldfilename = r->filename;
   r->filename = (char *)malloc(strlenn(filename) + 1);
   if(r->filename == NULL) {
      r->filename = oldfilename;
      return;
   }

   /* We just allocated the buffer, so the size is always ok */
   strcopy(r->filename, filename);
   free(oldfilename);

}



bool reg_load(reg *r) {

   if(r == NULL) return(false);

   r->line = 0;
   r->handle = fopen(r->filename, "r");
   if(r->handle == NULL) {
      reg_error(r, "Cannot open file to load");
      return(false);
   }

   if(!_reg_load_top(r)) {
      reg_error(r, "Nonrecoverable error occurred");
      fclose(r->handle);
      return(false);
   }

   fclose(r->handle);
   return(true);

}



bool reg_save(reg *r) {

   if(r == NULL) return(false);

   r->handle = fopen(r->filename, "w");
   if(r->handle == NULL) {
      reg_error(r, "Cannot open file to save");
      return(false);
   }

   fprintf(r->handle, "; This is an automatically generated file\n\n\n");
   fprintf(r->handle, "; Warning: there is _no_ error checking on values in this file.\n");
   fprintf(r->handle, "; This would make for a rather convenient way to set invalid values.\n");
   fprintf(r->handle, "; Maybe it could even be exploited to enable hidden `features'.\n\n\n");
   _reg_save_block(r, r->top, 0);

   fprintf(r->handle, "; End of file\n");
   fclose(r->handle);
   return(true);

}
