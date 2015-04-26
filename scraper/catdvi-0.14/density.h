/* catdvi - get text from DVI files
   Copyright (C) 2000-01 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

#ifndef DENSITY_H
#define DENSITY_H

/* Implements a staircase (i.e. piecewise constant) density function
 * on an interval [xmin, xmax].
 *
 * The domain can be integral or "real" (float, double), the range should
 * be "real".
 *
 * The implementation is NOT numerically sophisticated, so don't expect
 * miracles.
 */


/* There's no need to use these two typdefs in an application. They
 * are here for logical clarity and easy customization and can be changed
 * as needed.
 */
#include "bytesex.h"	/* for sint32 */
typedef sint32 scdf_domain_t;
typedef double scdf_range_t;

/* convenience defs - c++ does this automatically */
#ifndef __cplusplus
typedef struct scdf_t scdf_t;
typedef struct scdf_step_t scdf_step_t;
#endif

/* The function is stored as a (singly) linked list of steps. Its value
 * f(x) is step.f for x in the half-open interval [step.x, step.next->x) .
 * For technical reasons, we keep a last step with last.x = xmax and
 * last.next = NULL. last.f is not important since any value f(xmax) will
 * give the same integral of f.
 * 
 * Typical applications will traverse [xmin, xmax) as a union of subintervals
 * [x0, x1) from left to right. We try to keep this direction efficient.
 */

struct scdf_step_t {
    scdf_domain_t x;
    scdf_range_t f;
    scdf_step_t * next;
};

struct scdf_t {
    scdf_domain_t xmin;
    scdf_domain_t xmax;
    scdf_step_t * head;
    scdf_step_t * curr;
};


void scdf_init(
    scdf_t * this,
    scdf_domain_t xmin,
    scdf_domain_t xmax,
    scdf_range_t f  	/* The initial (constant) value of f - usually 0 */
);


void scdf_done(scdf_t * this);

/* Join neighbouring steps with the same f. This should be done at the
 * end of a sequence of operations traversing [xmin, xmax] .
 */
void scdf_normalize(scdf_t * this);


/* Force the density function to have at least value fmin in the interval
 * [x0, x1). Mathematically: let g have value fmin on [x0, x1) and value
 * (-infinity) elsewhere, then f is replaced by the pointwise maximum of
 * f and g.
 */
void scdf_force_min_value(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_domain_t x1,
    scdf_range_t fmin
);


/* Force f to have at least integral Jmin on [x0, x1]. This is currently
 * done by first checking if the integral is large enough anyway, and
 * forcing f to have value at least Jmin / (x1 - x0) if not. More
 * sophisticated (keeping f smaller in some cases) methods are possible.
 * However, some experiments with real world data for the intended application
 * (catdvi) have shown that:
 *   - Methods that tend to evenly distribute the density (like the
 *     one implemented here) do in almost all cases yield  better results
 *     (both in terms of appearance of output and of shorter lines) than
 *     an exact "additive" method which gives rather uneven distributions.
 *   - Replacing Jmin / (x1 - x0) by a quantity deviating at most 1/128
 *     from the minimal possible value gains 1-3 columns for some lines
 *     and nothing most of the time.
 * Since the currently implemented method is fast and seems to be nearly
 * optimal for typical catdvi input, we'll probably stick with it.
 */
void scdf_force_min_integral(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_domain_t x1,
    scdf_range_t Jmin
);


/* Find the value of f at x */
scdf_range_t scdf_eval(scdf_t * this, scdf_domain_t x);


/* Compute the integral of f on [x0, x1] */
scdf_range_t scdf_integral(scdf_t * this, scdf_domain_t x0, scdf_domain_t x1);


/* Solve the equation "integral of f on [x0, x1] = J" for x1;
 * set errno = EDOM if this is not possible.
 *
 * The algorithm used has to do a conversion from scdf_range_t to
 * scdf_domain_t, which is done by casting a _positive_ value of
 * type scdf_range_t to scdf_domain_t. This should result in rounding
 * the return value towards (-infinity) in cases where loss of precision
 * occurs.
 */
scdf_domain_t scdf_solve_integral_for_x1(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_range_t J
);

/* Create new staircase function
 * F(x) = floor(integral(f(t), t = f.xmin..x))
 * on the heap; return pointer to it. Abort if OOM.
 * F obviously has the same domain of definition as f.
 */
scdf_t * scdf_floor_of_integral(scdf_t * this);

/* Dump a textual representation of f to stderr */
void scdf_dump(scdf_t * this);

#endif
