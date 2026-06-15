/* $Header: /fridge/cvs/xscorch/libj/jstr/_str.h,v 1.8 2009-04-26 17:39:29 jacob Exp $ */
/*

   libj - _str.h                 Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Main internal header file for jstr library


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
#ifndef __libj__str_included
#define __libj__str_included


/* Include the public header */
#include <libjstr.h>


/* Functions for whitespace manipulation */
typedef struct _const_whitespace {
   const char *fnws;
   const char *lnws;
} const_whitespace;
typedef struct _whitespace {
   char *fnws;
   char *lnws;
} whitespace;
#define  SKIM_WHITESPACE(p)      do { while(WHITESPACE(*(p))) ++(p); } while(0)
#define  SKIM_WHITESPACE_T(p,t)  do { while(WHITESPACE(*(p)) && *(p) != (t)) ++(p); } while(0)
#define  SET_FIRST_NWS(ws,p)     do { (ws).fnws = (ws).lnws = (p); } while(0)
#define  SET_LAST_NWS(ws,p)      do { \
                                    while(*(p) != '\0') { \
                                       if(!WHITESPACE(*(p))) (ws).lnws = (p) + 1; \
                                       ++(p); \
                                    } \
                                 } while(0)
#define  SET_LAST_NWS_T(ws,p,t)  do { \
                                    while(*(p) != '\0' && *(p) != (t)) { \
                                       if(!WHITESPACE(*(p))) (ws).lnws = (p) + 1; \
                                       ++(p); \
                                    } \
                                 } while(0)
#define  NWS_SIZE(ws)            (unsigned)((ws).lnws - (ws).fnws)


/* Short macros for character buffer manipulation */
#if LIBJ_LIBC_STRING
   #define MEMEQL(s,d,z)   (!memcmp((s), (d), (z)))
   #define MEMCOMP(s,d,z)  (memcmp((s), (d), (z)))
   #define MEMCPY(d,s,z)   memcpy((d), (s), (z))
   #define MEMMOVE(d,s,z)  memmove((d), (s), (z))
   #define MEMSET(d,c,z)   memset((d), (c), (z))

   #define STREQL(s,d)     (!strcmp((s), (d)))
   #define STRCOMP(s,d)    (strcmp((s), (d)))
   #define STRLEN(s)       strlen(s)
   #define STRLENN(s)      (s == NULL ? 0 : strlen(s))
   #define STRNEQL(s,d,z)  (!strncmp((s), (d), (z)))
   #define STRNCOMP(s,d,z) (strncmp((s), (d), (z)))
   #define STRNCPY(d,s,z)  (strncpy((d), (s), (z)), *((d) + (z) - 1) = '\0')
   #define STRCPY(d,s)     strcpy((d), (s))
#else
   #define MEMEQL(s,d,z)   jmemeql((s), (d), (z))
   #define MEMCOMP(s,d,z)  jmemcmp((s), (d), (z))
   #define MEMCPY(d,s,z)   mcopy((d), (s), (z))
   #define MEMMOVE(d,s,z)  mmove((d), (s), (z))
   #define MEMSET(d,c,z)   mset((d), (c), (z))

   #define STREQL(s,d)     MEM_ARE_EQUAL(scmp((s), (d)))
   #define STRCOMP(s,d)    __memcomp((s), (d), scmp((s), (d)))
   #define STRLEN(s)       sscan_null(s)
   #define STRLENN(s)      (s == NULL ? 0 : sscan_null(s))
   #define STRNEQL(s,d,z)  MEM_ARE_EQUAL(scmp_size((s), (d), (z)))
   #define STRNCOMP(s,d,z) __memcomp((s), (d), scmp_size((s), (d), (z)))
   #define STRNCPY(d,s,z)  scopy_size((d), (s), (z))
   #define STRCPY(d,s)     scopy((d), (s))
#endif


#endif /* Header included */
