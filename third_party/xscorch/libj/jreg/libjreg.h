/* $Header: /fridge/cvs/xscorch/libj/jreg/libjreg.h,v 1.9 2009-04-26 17:39:27 jacob Exp $ */
/*

   libj - libjreg.h              Copyright (C) 2000-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Main header file for registry library

   Note that some of the macros defined in here require ISO C99.  The old
   notation for initializing a field, using "name: value", is deprecated
   in favour of the new ".name = value" notation.


   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation, version 2 of the License ONLY.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this library; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

   This file is part of LIBJ.

*/
#ifndef __libjreg_h_included
#define __libjreg_h_included



/* Required includes */
#include <libj.h>
#include <stdio.h>



/* Useful definitions */
#define  REG_TOP  "__top"     /* Name of `top' variable */
#define  REG_NULL "__null"    /* Name of `null' class */
#define  REG_SYMBOL_SIZE  64  /* Max symbol size */
#define  REG_BUFFER   0x1000  /* I/O buffer size */



/* Variable types */
typedef enum _reg_type {
   REG_INTEGER,               /* Integer value  */
   REG_DOUBLEV,               /* Floating value */
   REG_BOOLEAN,               /* Boolean values */
   REG_STRING,                /* Character string */
   REG_BLOCK,                 /* Subblock */
   REG_ANY                    /* Any type */
} reg_type;



/* Format types */
typedef enum _reg_format_bool {
   REG_FORMAT_BOOL_TF,        /* True/False vals */
   REG_FORMAT_BOOL_YN,        /* Yes/No values */
   REG_FORMAT_BOOL_OO,        /* On/Off values */
   REG_FORMAT_BOOL_TNIL       /* T/NIL values */
} reg_format_bool;



/* Default formats */
#define  REG_FORMAT_BOOL_DEFAULT    REG_FORMAT_BOOL_TF



/* Values datatype */
typedef struct _reg_bool {
   bool value;                /* Value of this variable */
   reg_format_bool format;    /* Output format to use */
} reg_bool;

typedef union _reg_value {
   int integer;               /* Integer value */
   double doublev;            /* Double value */
   reg_bool boolean;          /* Boolean value */
   char *string;              /* String data (must be allocated) */
   struct _reg_block *block;  /* A subblock */
} reg_value;



/* Variables */
typedef struct _reg_var_info {
   char name[REG_SYMBOL_SIZE];      /* Variable name */
   reg_type type;                   /* Variable type */
   const struct _reg_class *klass;  /* Class (if block)? */
   struct _reg_var_info *next;      /* Next varinfo in list */
} reg_var_info;

typedef struct _reg_var {
   char name[REG_SYMBOL_SIZE];      /* Variable name */
   reg_type  type;                  /* Variable type */
   reg_value value;                 /* Value assigned */
   struct _reg_var *next;           /* Next var in chain */
} reg_var;



/* Subblocks */
typedef struct _reg_class {
   char name[REG_SYMBOL_SIZE];      /* Name for this class */
   reg_var_info *vars;              /* Variable registry */
   struct _reg_class *next;         /* Next block class */
} reg_class;

typedef struct _reg_block {
   char name[REG_SYMBOL_SIZE];      /* Block name */
   const reg_class *klass;          /* Block class info */
   reg_var *vars;                   /* List of variables */
} reg_block;



/* File structure */
typedef struct _reg {
   char *filename;                  /* Filename to read */
   reg_var *top;                    /* Toplevel parse tree */
   reg_class *classes;              /* Block classes avail */
   FILE *handle;                    /* File I/O handle */
   int line;                        /* Current line number (file) */
} reg;



/* Bulk registry */
typedef struct _reg_class_data {
   const char *name;                /* Variable name */
   reg_type    type;                /* Variable type */
   const char *klass;               /* Variable class */
} reg_class_data;

typedef struct _reg_class_list {
   const char *name;                /* Class name */
   const reg_class_data *vars;      /* Class variables */
} reg_class_list;



/* Macros to simplify construction of reg_class_data */
#define  __REG_CLASS__(_name, _type, _klass) \
   { .name     = (_name),                 \
     .type     = (_type),                 \
     .klass    = (_klass)                 \
   }
#define  REG_CLASS_INTEGER(_name)         __REG_CLASS__(_name, REG_INTEGER, NULL)
#define  REG_CLASS_DOUBLEV(_name)         __REG_CLASS__(_name, REG_DOUBLEV, NULL)
#define  REG_CLASS_BOOLEAN(_name)         __REG_CLASS__(_name, REG_BOOLEAN, NULL)
#define  REG_CLASS_STRING(_name)          __REG_CLASS__(_name, REG_STRING,  NULL)
#define  REG_CLASS_BLOCK(_name, _klass)   __REG_CLASS__(_name, REG_BLOCK,   _klass)
#define  REG_CLASS_END                    __REG_CLASS__(NULL,  0,           NULL)

#define  REG_CLASS_BITFIELD(_name)        REG_CLASS_STRING(_name)

#define  REG_CLASS_LIST(_name, _vars) \
   { .name     = (_name),                 \
     .vars     = (_vars)                  \
   }
#define  REG_CLASS_LIST_END               REG_CLASS_LIST(NULL, NULL)



/* Bulk initialization */
typedef struct _reg_var_data {
   const char *name;                /* Variable name */
   reg_type    type;                /* Variable type */
   reg_value   value;               /* Variable value */
} reg_var_data;



/* Macros to simplify construction of reg_var_data */
#define  __REG_VAR__(_name, _type, _field, _value) \
   { .name     = (_name),                 \
     .type     = (_type),                 \
     .value    = { ._field = _value }     \
   }
#define  __REG_VAR_BOOL__(_value) \
   { .value    = (_value),                \
     .format   = REG_FORMAT_BOOL_DEFAULT  \
   }
#define  REG_VAR_INTEGER(_name, _value)   __REG_VAR__(_name, REG_INTEGER, integer, (_value))
#define  REG_VAR_DOUBLEV(_name, _value)   __REG_VAR__(_name, REG_DOUBLEV, doublev, (_value))
#define  REG_VAR_STRING(_name, _value)    __REG_VAR__(_name, REG_STRING,  string,  (_value))
#define  REG_VAR_BOOLEAN(_name, _value)   __REG_VAR__(_name, REG_BOOLEAN, boolean, __REG_VAR_BOOL__(_value))
#define  REG_VAR_END                      __REG_VAR__(NULL,  0,           integer, 0)



/* Unifying the class init and variable default data blocks */
typedef struct _reg_class_default_data {
   reg_class_data class_info;       /* Class information (variable type) */
   reg_var_data   default_info;     /* Default information (value info) */
} reg_class_default_data;

typedef struct _reg_class_default_list {
   const char *name;                   /* Class name */
   const reg_class_default_data *vars; /* Class variables */
} reg_class_default_list;



/* Macros to simplify construction of reg_class_data */
#define  __REG_CLASS_DEFAULT__(_class, _default) \
   { .class_info     =   _class,          \
     .default_info   = _default,          \
   }
#define  REG_CLASS_DEFAULT_INTEGER(_name, _value) \
   __REG_CLASS_DEFAULT__(REG_CLASS_INTEGER(_name), REG_VAR_INTEGER(_name, _value))
#define  REG_CLASS_DEFAULT_DOUBLEV(_name, _value) \
   __REG_CLASS_DEFAULT__(REG_CLASS_DOUBLEV(_name), REG_VAR_DOUBLEV(_name, _value))
#define  REG_CLASS_DEFAULT_BOOLEAN(_name, _value) \
   __REG_CLASS_DEFAULT__(REG_CLASS_BOOLEAN(_name), REG_VAR_BOOLEAN(_name, _value))
#define  REG_CLASS_DEFAULT_STRING(_name, _value) \
   __REG_CLASS_DEFAULT__(REG_CLASS_STRING(_name), REG_VAR_STRING(_name, _value))
#define  REG_CLASS_DEFAULT_BITFIELD(_name, _value) \
   __REG_CLASS_DEFAULT__(REG_CLASS_BITFIELD(_name), REG_VAR_STRING(_name, _value))
#define  REG_CLASS_DEFAULT_END \
   __REG_CLASS_DEFAULT__(REG_CLASS_END, REG_VAR_END)

#define  REG_CLASS_DEFAULT_LIST(_name, _vars) \
   REG_CLASS_LIST(_name, _vars)
#define  REG_CLASS_DEFAULT_LIST_END \
   REG_CLASS_LIST_END



/* Basic object maintenance */
reg *   reg_new(const char *name);
void    reg_free(reg **r);
void    reg_set_name(reg *f, const char *name);
bool    reg_load(reg *r);
bool    reg_save(reg *r);



/* Class add/Variable registry maintenance */
bool    reg_class_add(reg *r, const char *classname);
bool    reg_class_add_list(reg *r, const char **classnames);
bool    reg_class_register_var(reg *r, const char *classname, const char *name,
                               reg_type type, const char *klass);
bool    reg_class_register_vars(reg *r, const char *classname, const reg_class_data *list);
bool    reg_class_register_list(reg *r, const reg_class_list *list);
bool    reg_class_register_default_list(reg *r, const reg_class_default_list *list);



/* General variable information */
#define reg_get_top(r)  reg_get_var((r), NULL, NULL)
reg_var *   reg_get_var(reg *r, reg_var *v, const char *path);
reg_type    reg_get_var_type( const reg_var *v);
const char *reg_get_var_name( const reg_var *v);
const char *reg_get_var_class(const reg_var *v);



/* Variable reading */
bool    reg_get_integer(reg *r, reg_var *v, const char *path, int *value);
bool    reg_get_doublev(reg *r, reg_var *v, const char *path, double *value);
bool    reg_get_boolean(reg *r, reg_var *v, const char *path, bool *value);
bool    reg_get_string( reg *r, reg_var *v, const char *path, char *value, int size);
int     reg_get_integer_from_values(reg *r, reg_var *v, const char *path, int defvalue,
                                    const char **names, const unsigned int *values);
int     reg_get_bitmask_from_values(reg *r, reg_var *v, const char *path, int defvalue,
                                    const char **names, const unsigned int *values);



/* Iterative variable retrieval */
reg_var *reg_get_block_head(reg_var *v);
reg_var *reg_get_next_var(reg_var *v);



/* Variable set */
bool    reg_set_integer(reg *r, reg_var *v, const char *path, int value);
bool    reg_set_doublev(reg *r, reg_var *v, const char *path, double value);
bool    reg_set_boolean(reg *r, reg_var *v, const char *path, bool value);
bool    reg_set_boolean_f(reg *r, reg_var *v, const char *path, bool value,
                          reg_format_bool format);
bool    reg_set_string( reg *r, reg_var *v, const char *path, const char *value);
bool    reg_set_block(  reg *r, reg_var *v, const char *path, const char *klass);
bool    reg_set_string_from_values(reg *r, reg_var *v, const char *path, int value,
                                   const char **names, const unsigned int *values);
bool    reg_set_bitmask_from_values(reg *r, reg_var *v, const char *path, int value,
                                    const char **names, const unsigned int *values);



/* Bulk variable set */
bool    reg_set_vars(reg *r, reg_var *v, const char *path,
                     const reg_var_data *data);
bool    reg_set_var_defaults(reg *r, reg_var *v, const char *path,
                             const reg_var_data *data);
bool    reg_set_var_class_defaults(reg *r, reg_var *v, const char *path,
                                   const reg_class_default_data *data);



#endif /* __libjreg_h_included */
