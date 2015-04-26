/*
   Copyright (C) 1999 J.H.M. Dassen (Ray) <jdassen@wi.LeidenUniv.nl>
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
   Copyright (C) 2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define lengthof(array) (sizeof(array) / sizeof(array[0]))
/* the number of elements in an array (a real one, NOT a pointer) */

extern int msglevel;
extern int suppress_warnings;
extern int num_warnings;

#ifdef __GNUC__
#  define FORMAT(x) __attribute__((format x))
#else 
#  define FORMAT(x) /* nothing */
#endif

void warning(const char * format, ...) FORMAT((printf, 1, 2));
/* print an unsurpressable warning */

void panic(const char * format, ...) FORMAT((printf, 1, 2));
/* print a message, and exit */


#if defined(NDEBUG) && defined(__GNUC__)
/* gcc's cpp has extensions; it allows for macros with a variable number of
 *    arguments. We use this extension here to preprocess eassert away. */
#define eassert(int expr, format, args...) ((void)0)
#else
void eassert(int expr, const char * format, ...) FORMAT((printf, 2, 3));
/* Like assert(), but with a descriptive message */
#endif


#if defined(NDEBUG) && defined(__GNUC__)
/* gcc's cpp has extensions; it allows for macros with a variable number of
   arguments. We use this extension here to preprocess pmesg away. */
#define pmesg(level, format, args...) ((void)0)
#else
void pmesg(int level, const char * format, ...) FORMAT((printf, 2, 3));
/* print a message, if it is considered significant enough.
      Adapted from [K&R2], p. 174 */
#endif


extern const char * const util_notreached_format;
#define NOTREACHED panic(util_notreached_format, __FILE__, __LINE__)


extern const char * const util_oom_format;
#define enomem( ) panic(util_oom_format, __FILE__, __LINE__)


/* xmalloc(size) tries to malloc() size bytes of memory and panics with a
 * diagnostic message if it didn't get them. There are situations when it is
 * convenient to ask for zero bytes of memory (variable length arrays in .tfm
 * data which may have length 0). ISO C allows malloc(0) to return NULL, and
 * on some systems (e.g. AIX 4.2) it really does, so we must not panic
 * in this case.
 *
 * xmalloc() is implemented as a macro. Memory checkers like to #define
 * it themselves, and we don't want to stop them.
 */
#ifndef xmalloc
#define xmalloc(size) util_xmalloc(size, __FILE__, __LINE__)
#endif
extern void * util_xmalloc(size_t size, const char * file, int line);

/* Simulate malloc(0) == NULL behaviour with GNU libc.
 * Useful for debugging purposes only.
 * Does not work with C libraries different from GNU libc !
 */
#ifdef USE_MALLOC0
extern void use_malloc0(void);
#endif

/* Conver a string in-place to upper case */
void strupcase(char * s);

/* Return a malloc'ed upper-case copy of a string, NULL if out of memory. */
char * strupcasedup(char const * s);

/* Simple shell-style pattern matching. Wildcards are:
 * '*': matches any string (including empty string)
 * '?': matches any single character
 * '#': matches any decimal digit (convenient for TeX font names)
 * Returns true if matching, 0 otherwise.
 */
int patmatch(char const * pat, char const * s);


#endif /* UTIL_H */
