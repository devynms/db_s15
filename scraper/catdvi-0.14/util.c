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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "util.h"

int msglevel = 0; /* the higher, the more messages... */
int suppress_warnings = 0; /* flag */
int num_warnings = 0; /* count */

void warning(const char * format, ...)
{
	va_list args;

        ++num_warnings;

        va_start(args, format);
        if (!suppress_warnings) vfprintf(stderr, format, args);
        va_end(args);
}

void panic(const char * format, ...)
{
	va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
	exit(EXIT_FAILURE);
}

#if defined(NDEBUG) && defined(__GNUC__)
/* Nothing. eassert has been "defined away" in util.h already. */
#else
void eassert(int expr, const char * format, ...) {
#ifdef NDEBUG
	/* Empty body, so a good compiler will optimise calls away */
#else
	va_list args;

	if (!expr) {
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		exit(EXIT_FAILURE);
	}
#endif /* NDEBUG */
}
#endif /* NDEBUG && __GNUC__ */


#if defined(NDEBUG) && defined(__GNUC__)
/* Nothing. pmesg has been "defined away" in util.h already. */
#else
void pmesg(int level, const char * format, ...) {
#ifdef NDEBUG
	/* Empty body, so a good compiler will optimise calls
	   to pmesg away */
#else
        va_list args;

        if (level>msglevel)
                return;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
#endif /* NDEBUG */
#endif /* NDEBUG && __GNUC__ */
}

const char * const util_notreached_format =
    "Please report a bug.  Include in the report "
    "the following information,\n"
    "exactly as it is written here: "
    "\"%s:%i was reached\".\n"
    "Please include in your report as much detail "
    "as possible about what you\n"
    "were doing at the "
    "time you saw this message.  Thank you.\n";



const char * const util_oom_format = "Out of memory (near %s line %d)\n";

void * util_xmalloc(size_t size, const char * file, int line)
{
    void * res;

    res = malloc(size);
    if(res == NULL && size != 0) panic(util_oom_format, file, line);
    return res;
}


/* Simulate malloc(0) == NULL behaviour with GNU libc.
 * Useful for debugging purposes only.
 * Does not work with C libraries different from GNU libc!
 */
#ifdef USE_MALLOC0
#include <malloc.h>

typedef void * mallhook_func(size_t, const void *);
mallhook_func * old_mallhook;

/* our replacement malloc */
void * malloc0(size_t size, const void * caller);

void * malloc0(size_t size, const void * caller)
{
    void * result;

    if(size == 0) return NULL;

    /* restore old hook */
    __malloc_hook = old_mallhook;

    /* call real malloc */
    result = malloc(size);

    /* save old hook again because it may have changed itself */
    old_mallhook = __malloc_hook;

    /* hook in again */
    __malloc_hook = &malloc0;

    return result;
}

void use_malloc0(void)
{
    /* save old hook */
    old_mallhook = __malloc_hook;
    
    /* set to our one */
    __malloc_hook = malloc0;
}
#endif /* USE_MALLOC0 */


void strupcase(char * s)
{
        register char * p;

        for (p = s; *p !=0; ++p) {
                *p = islower(*p) ?
                        toupper(*p) :
                        *p;
        }
}

char * strupcasedup(char const * s)
{
    char * su;

    su = malloc(strlen(s) + 1);
    if (su == NULL) return NULL;
    strcpy(su, s);
    	/* We don't use strdup because it's not part of ISO C. */
    strupcase(su);
    
    return su;
}

int patmatch(char const * pat, char const * s)
{
    for( ; *pat != 0; ++pat, ++s) {
	switch(*pat) {
    	    case '*':
		++pat;
		if(*pat == 0) return 1;
	    	    /* speedup in frequent case that pattern ends with '*' */
		do {
	    	    if(patmatch(pat, s)) return 1;
		} while(*s++ != 0);
		return 0;
	    case '?':
		if(*s == 0) return 0;
		break;
#if 0
    	    /* Possibly useful extension: '+' matches 0 or 1 characters.
	     * Or even interchange '+', '?' ? Disabeled for now.
	     */
	    case '+':
	    	++pat;
	    	return ((*s != 0) && patmatch(pat, s+1)) || patmatch(pat, s);
#endif
	    case '#':
		if(!isdigit(*s)) return 0;
		break;
#if 0
    	    /* Possibly useful extension: '\\' protects next char from
	     * wildcard interpretation. Disabeled for now.
	     */
	    case '\\':
	    	++pat;
		assert(*pat != 0);
		/* fall through to default */
#endif
	    default:
		if(*pat != *s) return 0;
	    	    /* this covers premature end of s, too */
	} /* switch */
    } /* for */
    return (*s == 0);
}
