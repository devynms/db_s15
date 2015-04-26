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

#include "density.h"
#include <assert.h>
#include <stdlib.h>
#include "util.h"   /* for enomem() and xmalloc() */
#include <stdio.h>  /* used by scdf_dump only */
#include <errno.h>

/* Insert a step at x, with value f to the right. where is the nearest
 * existing step to the left.
 * Returns address of the newly created step.
 */
scdf_step_t * scdf_insert_after(
    scdf_t * this,
    scdf_step_t * where,
    scdf_domain_t x,
    scdf_range_t  f
);


/* Delete the step _after_ (i.e. _right from_ ) where */
void scdf_delete_after(scdf_t * this, scdf_step_t * where);


/* Let scdf.curr point to the step in which x lies. Returns new curr. */
scdf_step_t * scdf_set_curr_to_x(scdf_t * this, scdf_domain_t x);

/**************************************************************************
 **************************************************************************/


void scdf_init(
    scdf_t * this,
    scdf_domain_t xmin,
    scdf_domain_t xmax,
    scdf_range_t f
)
{
    scdf_step_t * pa, * pb;
    
    assert(xmin < xmax);
    
    this->xmin = xmin;
    this->xmax = xmax;
    
    pa = malloc(sizeof(scdf_step_t));
    if(pa == NULL) enomem();
    pb = malloc(sizeof(scdf_step_t));
    if(pb == NULL) enomem();
    
    pa->x = xmin;
    pa->f = f;
    pa->next = pb;
    pb->x = xmax;
    pb->f = f;
    pb->next = NULL;
    
    this->curr = this->head = pa;
}

void scdf_done(scdf_t * this)
{
    scdf_step_t * p0, * p1;
    
    for(p0 = this->head; p0 != NULL; p0 = p1) {
    	p1 = p0->next;
	free(p0);
    }
    this->curr = this->head = NULL;
    this->xmin = this->xmax = 0;
}


scdf_step_t * scdf_insert_after(
    scdf_t * this,
    scdf_step_t * where,
    scdf_domain_t x,
    scdf_range_t  f
)
{
    scdf_step_t * p;
    
    assert(where->next != NULL);
    	/* Can't insert after the end of the interval */
    assert(where->x < x ); assert(x < where->next->x);
    
    p = malloc(sizeof(scdf_step_t));
    if(p == NULL) enomem();
    
    p->x = x;
    p->f = f;
    p->next = where->next;
    where->next = p;
    
    return p;
}


void scdf_delete_after(scdf_t * this, scdf_step_t * where)
{
    scdf_step_t * p;
    
    p = where->next;
    assert(p != NULL);
    	/* Can't delete after the end of the interval */
    assert(p->next != NULL);
    	/* Don't delete the "step" at xmax */

    where->next = p->next;
    if(this->curr == p) this->curr = where;
    	/* don't leave dangling curr */
    free(p);
}


scdf_step_t * scdf_set_curr_to_x(scdf_t * this, scdf_domain_t x)
{
    scdf_step_t * p0, * p1;
    
    assert(this->xmin <= x); assert(x <= this->xmax);
    
    p0 = this->curr;
    if(p0->x > x) p0 = this->head;
    	/* too far to the right, start at the left */

    while(p1 = p0->next, p1 != NULL && p1->x <= x) p0 = p1;

    this->curr = p0;
    return p0;
}


void scdf_force_min_value(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_domain_t x1,
    scdf_range_t fmin
)
{
    scdf_step_t * p0, * p1;
    
    assert(this->xmin <= x0); assert(x0 < x1);  assert(x1 <= this->xmax);
    
    p0 = scdf_set_curr_to_x(this, x0);
    p1 = p0->next;
    
    for( ; x0 < x1; x0 = p1->x, p0 = p1, p1 = p1->next) {
    
    	if(p1->x > x1) p1 = scdf_insert_after(this, p0, x1, p0->f);
	    /* Avoid complications that would arise in the last iteration
	     * if x1 wasn't a step boundary by forcing it to be one.
	     * A final scdf_normalize() will clean up this crap.
	     */
	
	if(fmin <= p0->f) continue;
	    /* the nothing-to-do case: f is already large enough */

    	if(p0->x < x0) {
	    /* If we change the function value in the middle of a step
	     * we have to subdivide. The newly created step is automatically
	     * skipped by the for loop iteration code.
	     */
	    scdf_insert_after(this, p0, x0, fmin);
	    continue;
	}
	
	/* Now we know p0->x == x0 && x1 <= p1->x && p0->f < fmin ,
	 * i.e. we have a step which is completely contained in [x0, x1]
	 * and its value is too small.
	 */
	p0->f = fmin;
    }
    
    this->curr = p0;
    	/* p0 points to the step containing x1 now - this is where the
	 * next call will most probably start.
	 */
}


void scdf_force_min_integral(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_domain_t x1,
    scdf_range_t Jmin
)
{
    if(Jmin > scdf_integral(this, x0, x1)) {
    	scdf_force_min_value(this, x0, x1, Jmin / (x1 - x0));
    }
}

void scdf_normalize(scdf_t * this)
{
    scdf_step_t * p0, * p1, * p2;
    
    p0 = this->head;
    p1 = p0->next;  	/* always != NULL */
    p2 = p1->next;  	/* could be NULL  */
    
    this->curr = this->head;
    	/* May point to free()d memory otherwise. Besides, we expect to
	 * be called after processing the whole interval of definition
	 * anyway.
	 */
    
    while(p2 != NULL) {
    	if(p0->f == p1->f) scdf_delete_after(this, p0);
	else p0 = p1;
	
	p1 = p2;
	p2 = p2->next;
    }
}


scdf_domain_t scdf_solve_integral_for_x1(
    scdf_t * this,
    scdf_domain_t x0,
    scdf_range_t J
)
{
    scdf_step_t * p0, * p1;
    scdf_range_t K;

    assert(this->xmin <= x0); assert(x0 < this->xmax);
    assert(J > 0);
    
    p0 = scdf_set_curr_to_x(this, x0);
    
    while(p1 = p0->next, p1 != NULL) {
    	K = p0->f * (p1->x - x0);
	if(K > J) {
	    return x0 + (scdf_domain_t) (J / p0->f);
	}
	
	J -= K;
	x0 = p1->x;
	p0 = p1;
    }
    errno = EDOM;
    return this->xmax;
}

scdf_range_t scdf_eval(scdf_t * this, scdf_domain_t x)
{
    return scdf_set_curr_to_x(this, x)->f;
}


scdf_range_t scdf_integral(scdf_t * this, scdf_domain_t x0, scdf_domain_t x1)
{
    scdf_step_t * p0, * p1;
    scdf_domain_t d;
    scdf_range_t J = 0;
    
    assert(this->xmin <= x0); assert(x0 < x1); assert(x1 <= this->xmax);
    
    p0 = scdf_set_curr_to_x(this, x0);
    p1 = p0->next;

    for( ; x0 < x1; x0 = p1->x, p0 = p1, p1 = p1->next) {
    
    	/* Sum it up step by step - and be careful with the last one. */
    	if(x1 < p1->x) d = x1 - x0;
	else d = p1->x - x0;
	J += d * p0->f;
    }
    
    return J;
}

void scdf_dump(scdf_t * this)
{
    scdf_step_t * p;
    
    p = this->head;
    fprintf(stderr, "scdf at %p {\n", (void *) this);
    for(p = this->head; p != NULL; p = p->next) {
    	fprintf(stderr, "\tf(%10.8g) = %10.8g\n", (double) p->x, (double) p->f);
	    /* should work with any type of domain and range */
    }
    fputs("}\n", stderr);
}


scdf_t * scdf_floor_of_integral(scdf_t * this)
{
    scdf_domain_t x0, x1;
    scdf_range_t f = 0;
    scdf_t * result;
    scdf_step_t * p;

    result = xmalloc(sizeof(scdf_t));
    scdf_init(result, this->xmin, this->xmax, 0);
    	/* we start with result identically 0 */
    p = result->head;
    x0 = this->xmin;

    for( ; ; ) {
    	/* Where should the next jump by 1 of result be? */
    	errno = 0;
	x1 = scdf_solve_integral_for_x1(this, x0, 1);

	if(errno == EDOM) {
    	    /* EDOM means not enough density right from x0 to give integral 1,
	     * i.e. no more jumps and we're done.
	     */
    	    break;
	}

    	/* OK, result should jump by 1 at x1 */
	f += 1;
	if(x1 == this->xmax) {
	    /* Jump is at the right margin, result already has a pseudo step
	     * there and we're done.
	     */
	    break;
	}
	
	/* Insert a step at x1 with value f */
    	p = scdf_insert_after(result, p, x1, f);

	x0 = x1;
    }

    /* The pseudo step at xmax still has to get its f value */
    p = scdf_set_curr_to_x(result, this->xmax);
    p->f = f;

    return result;
}
