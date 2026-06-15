/* $Header: /fridge/cvs/xscorch/libj/jstr/libjstr.h,v 1.18 2009-04-26 17:39:29 jacob Exp $ */
/*

   libj - jstr.h                 Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Main header file for jstr library


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
#ifndef __libj_str_included
#define __libj_str_included


/* Definitions and datatypes */
#include <stdio.h>
#include <string.h>
#include <libj.h>
#if !LIBJ_LIBC_STRING
   #if LIBJ_ALLOW_LIBC_STRING
      #define  LIBJ_ALLOW_LIBC_MEMORY    1
   #endif /* String requires Memory */
   #include <libjmem.h>
#endif


/* Time functions */
#if HAVE_GETTIMEOFDAY
#  if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#    include <time.h>
#  else
#    if HAVE_SYS_TIME_H
#      include <sys/time.h>
#    else
#      include <time.h>
#    endif
#  endif
#endif


/* Rabin-Karp configuration structure */
typedef struct _rkstate {
   sizea  rk_i;            /* Rabin-Karp incrementor/decrementor */
   sdword rk_t;            /* Rabin-Karp "t" accumulator */
   sdword rk_p;            /* Rabin-Karp "p" accumulator */
   sdword rk_h;            /* Rabin-Karp "h" multiplier */
   char *rk_source;        /* Rabin-Karp source buffer */
   const char *rk_pattern; /* Rabin-Karp search pattern */
   sizea  rk_dlen;         /* Rabin-Karp destination string len */
} rkstate;


/* String pairs */
typedef struct _string_pair {
   char *first;
   char *second;
} string_pair;
#define  pair_car(p)    ((p)->first)
#define  pair_cdr(p)    ((p)->second)


/* Definitions useful for string manipulation */
#define SEP    '|'      /* Default separator character */
#define COMM   '#'      /* Default comment character */


/* Boolean characters */
#define BOOLU(c)        ((c) ? 'T' : 'F')
#define BOOLL(c)        ((c) ? 't' : 'f')
#define BOOL(c)         BOOLU(c)


/* Alphabetic and numeric characters; case verification */
#define LOWER(c)        (((c) >= 'a' && (c) <= 'z'))
#define UPPER(c)        (((c) >= 'A' && (c) <= 'Z'))
#define DIGIT(c)        (((c) >= '0' && (c) <= '9'))
#define ALPHA(c)        (LOWER(c) || UPPER(c))
#define WHITESPACE(c)   (((c) == ' ' || (c) == '\t' || (c) == '\n'))


/* Disable LIBC functions if requested */
#if (!LIBJ_ALLOW_LIBC_STRING)
   #undef      strcmp
   #define     strcmp      __dont_use_strcmp
   #undef      strncmp
   #define     strncmp     __dont_use_strncmp
   #undef      strcasecmp
   #define     strcasecmp  __dont_use_strcasecmp
   #undef      strncasecmp
   #define     strncasecmp __dont_use_strncasecmp
   #undef      strcpy
   #define     strcpy      __dont_use_strcpy
   #undef      strncpy
   #define     strncpy     __dont_use_strncpy
   #undef      strcat
   #define     strcat      __dont_use_strcat
   #undef      strncat
   #define     strncat     __dont_use_strncat
   #undef      strlen
   #define     strlen      __dont_use_strlen
   #undef      strnlen
   #define     strnlen     __dont_use_strnlen
   #undef      sprintf
   #define     sprintf     __dont_use_sprintf
   #undef      snprintf
   #define     snprintf    __dont_use_snprintf
   #undef      strstr
   #define     strstr      __dont_use_strstr
#endif /* LIBJ_ALLOW_LIBC_STRING */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* A note on convention and suffices.  Functions which accept size args
   may follow one of two conventions (denoted by a suffix):
      'n':  size specifies the number of characters, EXCLUDE '\0'.
            Often used in functions which count but not copy.
      'b':  size specifies actual size of the buffer.  The number of chars
            (if applicable) is usually the size - 1, and the function (if
            copying) will impose a '\0' byte in the last position. Use the
            'b' version of the function if you are giving a BUFFER SIZE.
   Suffix 'p' is used when we do not copy, but rather pass around direct
   pointers.
 */


/*** Argument String Processing ***/
sizea getnumargs(const char *s, char sep);
char *getarg(char *d, const char *s, sizea argnum, char sep);
char *getargb(char *d, const char *s, sizea argnum, char sep, sizea maxlen);
char *getargn(char *d, const char *s, sizea argnum, char sep, sizea n);
sizea getargp(char **d, char *s, sizea argnum, char sep);
sizea getargp_trim(char **d, char *s, sizea argnum, char sep);


/*** Case-adjustment, etc. ***/
char *unescape(char *s);
char *unescape_quoted(char *s);
char *escapeb(char *s, sizea size);
char *escapen(char *s, sizea n);
char *escaped_scan(char *src, char ch);
char *escaped_chop(char *src, char ch);
char *strlower(char *s);
char *strupper(char *s);
char *strproper(char *s);


/*** CGI Functions ***/
bool  getassign(char *var, char *val, const char *s);
bool  getassignbb(char *var, char *val, const char *s, sizea lvar, sizea lval);
bool  getassignbn(char *var, char *val, const char *s, sizea lvar, sizea nval);
bool  getassignnb(char *var, char *val, const char *s, sizea nvar, sizea lval);
bool  getassignnn(char *var, char *val, const char *s, sizea nvar, sizea nval);
#define  getassignb(var, val, s, lvar, lval)    getassignbb((var), (val), (s), (lvar), (lval))
#define  getassignn(var, val, s, lvar, lval)    getassignnn((var), (val), (s), (lvar), (lval))
char *unescapeCGI(char *s);
char *escapeCGI(char *s);
sizea getnumargsCGI(const char *s);
char *getargCGI(char *d, const char *s, sizea argnum);
char *getargCGIb(char *d, const char *s, sizea argnum, sizea bufsize);
char *getargCGIn(char *d, const char *s, sizea argnum, sizea nchars);


/*** String comparison routines ***/
/* For comparison, n is the maximum number of characters to compare
   against.  If the first n chars agree, it is a total match. */
int   strcomp(const char *a, const char *b);
int   strcompn(const char *a, const char *b, sizea nchars);
bool  strequal(const char *a, const char *b);
bool  strequaln(const char *a, const char *b, sizea nchars);
#define  streql(a, b)         strequal((a), (b))
#define  streqln(a, b, n)     strequaln((a), (b), (n))
char *rkstrpat(rkstate *rk, char *s, const char *pat);
char *rkstrnext(rkstate *rk);
char *kmpstrpat(char *s, const char *pat);
char *strscan(char *s, const char *pat);
char *strscan_list(char *s, const char *const*patlist, sizea *index);
bool strsimilar(const char *A, const char *B);

/* Shorthands */
#define  jstrcmp(a, b)        strcomp((a), (b))
#define  jstrncmp(a, b, n)    strcompn((a), (b), (n))
#define  jstreql(a, b)        strequal((a), (b))
#define  jstrneql(a, b, n)    strequal((a), (b), (n))
#define  jstrscan(s, pat)     strscan((s), (pat))


/*** Console (formatting) string functions ***/
char *cpyslenn(char *d, const char *s, sizea size);
void  putslenn(const char *s, sizea size);
char *cpysrlenn(char *d, const char *s, sizea size);
void  putsrlenn(const char *s, sizea size);


/*** Sprintf services ***/
char *sbprintf(char *dest, sizea size, const char *fmt, ...);
char *sbprintf_concat(char *dest, sizea size, const char *fmt, ...);


/*** String copy routines ***/
char *strcopyb(char *d, const char *s, sizea bufsize);
char *strcopyn(char *d, const char *s, sizea nchars);
char *strcopynb(char *d, const char *s, sizea nchars, sizea bufsize); /* #chars to append/size of buffer */
#define  strcopybn(d, s, bufsize, nchars)  strcopynb((d), (s), (nchars), (bufsize))
char *strcopy(char *d, const char *s);
char *strconcatb(char *d, const char *s, sizea bufsize);    /* Size of dest buffer! */
char *strconcatn(char *d, const char *s, sizea nchars);     /* Number of chars to append! */
char *strconcatnb(char *d, const char *s, sizea nchars, sizea bufsize); /* #chars to append/size of buffer */
#define  strconcatbn(d, s, bufsize, nchars)  strconcatnb((d), (s), (nchars), (bufsize))
char *strconcat(char *d, const char *s);

/* Shorthands */
#define  jstrcpy(d, s)        strcopy((d), (s))
#define  jstrbcpy(d, s, n)    strcopyb((d), (s), (n))
#define  jstrncpy(d, s, n)    strcopyn((d), (s), (n))
#define  jstrcat(d, s)        strconcat((d), (s))
#define  jstrbcat(d, s, n)    strconcatb((d), (s), (n))
#define  jstrncat(d, s, n)    strconcatn((d), (s), (n))
#define  jstrnbcat(d, s, n, b) strconcatnb((d), (s), (n), (b))


/*** Special string creation routines ***/
char *strdupl(const char *s);


/*** String information functions ***/
sizea strlenn(const char *s);
sizea strnlenn(const char *s, sizea maxchars);
sizea strblenn(const char *s, sizea size);
sizea strnumwords(const char *s);
sizea strnumlines(const char *s);


/*** File I/O ***/
sizea fcount(const char *fname);


/*** Numerical functions ***/
sdword strtoint(const char *s, int *succ);
double strtofloat(const char *s, int *succ);
char * inttostr(char *d, sdword num, sizea digits);
char * inttohex(char *d, udword num, sizea digits);
bool   getbool(bool *result, const char *s);


/*** String Replacement Functions ***/
sizea strreplaceb(char *s, const char *src, const char *dst, sizea bufsize);
sizea strreplacen(char *s, const char *src, const char *dst, sizea nchars);


/*** Time functions ***/
#if HAVE_GETTIMEOFDAY
bool decodetime(struct tm *t, const char *s);
char *encodetime(char *d);
#endif


/*** String trimming ***/
char *trimcomment(char *s, char c);
char *trimbracecomment(char *s, char c1, char c2);
char *trim(char *s);
char *rtrim(char *s);
char *ltrim(char *s);


/*** Unicode functions ***/
char *unitochar(char *d, const wchar *s);
wchar *chartouni(wchar *d, const char *s);
char *unitocharc(char *d, const wchar *s, sizea maxlen);
wchar *chartounic(wchar *d, const char *s, sizea maxlen);


#ifdef __cplusplus /* Close the extern */
};
#endif /* __cplusplus */


#endif /* Header included */
