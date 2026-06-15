/* $Header: /fridge/cvs/xscorch/libj/jreg/_reg.h,v 1.10 2009-04-26 17:39:26 jacob Exp $ */
/*

   libj - _reg.h              Copyright(c) 2000-2003 Justin David Smith
   justins (a) chaos2.org     http://chaos2.org/

   File reader/writer/parser, variable registry


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
#ifndef __reg_int_h_included
#define __reg_int_h_included


/* Includes */
#include <libjreg.h>
#include <libjstr.h>


/* Error reporting assistance */
#define reg_error_line(f, fmt) \
   if((f) != NULL) fprintf(stderr, "%s:%d: error: " fmt "\n", (f)->filename, (f)->line)
#define reg_error_line1(f, fmt, arg1) \
   if((f) != NULL) fprintf(stderr, "%s:%d: error: " fmt "\n", (f)->filename, (f)->line, (arg1))
#define reg_error_line2(f, fmt, arg1, arg2) \
   if((f) != NULL) fprintf(stderr, "%s:%d: error: " fmt "\n", (f)->filename, (f)->line, (arg1), (arg2))
#define reg_error_line3(f, fmt, arg1, arg2, arg3) \
   if((f) != NULL) fprintf(stderr, "%s:%d: error: " fmt "\n", (f)->filename, (f)->line, (arg1), (arg2), (arg3))
#define reg_error(f, fmt) \
   if((f) != NULL) fprintf(stderr, "%s: error: " fmt "\n", (f)->filename)
#define reg_error1(f, fmt, arg1) \
   if((f) != NULL) fprintf(stderr, "%s: error: " fmt "\n", (f)->filename, (arg1))
#define reg_error2(f, fmt, arg1, arg2) \
   if((f) != NULL) fprintf(stderr, "%s: error: " fmt "\n", (f)->filename, (arg1), (arg2))


/* Variable registry */
reg_var_info *reg_var_info_new(reg_var_info **varlist, const char *name,
                               reg_type type, const reg_class *klass);
reg_type     reg_var_info_lookup(const reg_class *bc, const char *name);
void         reg_var_info_release(reg_var_info **reg);


/* Variables */
void    reg_var_free(reg_var **v);
bool    reg_var_set_integer(const reg *r,  reg_var *v, const char *name, int value);
bool    reg_var_set_doublev(const reg *r,  reg_var *v, const char *name, double value);
bool    reg_var_set_boolean(const reg *r,  reg_var *v, const char *name, bool value, reg_format_bool format);
bool    reg_var_set_string(const reg *r,   reg_var *v, const char *name, const char *value);
bool    reg_var_set_block(const reg *r,    reg_var *v, const char *name, reg_var **block);
bool    reg_var_set_by_value(const reg *r, reg_var *v, const char *name, char *value);
bool    reg_var_merge_block(const reg *r,  reg_var *v, const char *name, reg_var **block);
reg_var *reg_var_new_block(const reg *r, const char *name, const reg_class *klass);
reg_var *reg_var_lookup(reg_var *v, const char *name);


/* Block classes */
reg_class *reg_class_lookup(reg_class *classes, const char *name);
void       reg_class_release(reg_class **classes);


/* Blocks */
reg_block *reg_block_new(const char *name, const reg_class *klass);
void       reg_block_free(reg_block **block);


/* Variable resolution */
reg_var *  reg_block_resolve(reg *r, reg_var *v, const char *path);
reg_var *  reg_block_resolve_container(reg *r, reg_var *v, const char *path, char *varname);


#endif /* __reg_int_h_included */
