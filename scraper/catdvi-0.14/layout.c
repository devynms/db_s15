/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
   Copyright (C) 2000-02 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "util.h"
#include "page2.h"
#include "outenc.h"
#include "linebuf.h"
#include "density.h"
#include "canvas.h"
#include "vlist.h"
#include "fontinfo.h"

/* page margins */
static sint32 left_margin, right_margin;
static sint32 top_margin, bottom_margin;

/* other page metrics */
int linecount = 0;
uint32 boxcount = 0;
double average_box_width = 0;
double average_box_totheight = 0;

/* layout parameters */
sint32 max_col_width, max_row_height;
double min_col_density, min_line_density;
scdf_t col_density, line_density;
scdf_t * x2col, * y2line;


/* some obvious stuff about rectangles */
typedef struct rect_t rect_t;
struct rect_t {
    sint32 xmin;
    sint32 xmax;
    sint32 ymin;
    sint32 ymax;
};

int intervals_intersect(
    sint32 a, sint32 b,
    sint32 c, sint32 d
);

int rect_intersects(const rect_t * this, const rect_t * that);

int rect_contains_point(const rect_t * this, sint32 x, sint32 y);

/* create rect */
void rect_set(
    rect_t * this,
    sint32 xmin,
    sint32 xmax,
    sint32 ymin,
    sint32 ymax
);

void rect_set_empty(rect_t * this);

int rect_is_empty(const rect_t * this);

/* *this = smallest rect containing *this and *that */
void rect_union_with(rect_t * this, const rect_t * that);


/* a page_word is a sequence of contiguous box_t which should remain
 * contiguous in printout.
 */
typedef struct page_word_t page_word_t;
struct page_word_t {
    list_node_t * alpha;    /* node of first box in word */
    list_node_t * omega;    /* node of last box in word */
    linebuf_t * text;
    rect_t brect;   	    /* bounding rectangle: the smallest rectangle
    	    	    	     * containing the part _avove the baseline_
			     * of all boxes in the word
			     */
    sint32 ceiling; 	    /* stuff with y <= ceiling should appear top of
    	    	    	     * our row in printout.
			     */
    sint32 right_wall;	    /* stuff with x >= right_wall should appear right
    	    	    	     * from our rightmost glyph in printout.
			     */
    sint32 right_corridor;  /* stuff on the same row with x >= right_corridor
    	    	    	     * should appear trailing_spaces columns right from
			     * our rightmost glyph in printout.
			     */
    int output_width;
    int trailing_spaces;    /* usually 1, somtetimes 0 */
    int row;
    int column;
};

/* The page has one global list of page_words */
vlist_t page_words;

/* constructor */
static void page_word_init(
    page_word_t * this,
    list_node_t * alpha,
    list_node_t * omega,
    int trailing_spaces
);

/* destructor */
static void page_word_done(page_word_t * this);

/* Try to resolve collisions (i.e. situations where the bounding rects of
 * two words intersect).
 */
static void page_words_collide(vitor_t vi, vitor_t vj);

/* Split *this before where; i.e. :
 *   - create a new page_word containing the boxes from where to this->omega
 *     on the heap ("right half")
 *   - shrink *this to the boxes from this->alpha to where->prev ("left half")
 *   - return pointer to new page_word
 * Both the left and right half are brought into shape by constructor calls,
 * so any information about ceilings, right walls, etc. gets lost.
 */
static page_word_t * page_word_split_at(
    page_word_t * this,
    struct list_node_t * where
);


/* Estimate number of spaces from distance between boxes */
static int decide_space_count(
    struct list_node_t * prev_p,
    struct list_node_t * p
);

/* Look at the midpoints of two boxes. If either midpoint lies in the other
 * box, the boxes compare "equal". Otherwise, we compare x of midpoints.
 */
sint32 box_sloppy_x_cmp(const struct box_t * p1, const struct box_t * p2);


static void page_collect_statistics(void);

/* Make an initial pass at cutting the page into words, based on the distance
 * between adjacent boxes. This will be refined in a later pass.
 */
static void page_cut_words(void);


/* If the bounding rectangles of two words on the page collide, we have
 * likely hit a word breaking problem (example: math subscripts are
 * too thin to cause a word break between adjacent variables). If a
 * collission occurs, we call a function that tries to resolve it
 * by introducing additional word breaks.
 */
static void page_check_word_collisions(void);

/* The idea: words with x coordinates close to each other and different
 * y coordinates should appear on different rows in printout.
 */
static void page_find_ceilings(void);

/* If two words end up on the same line in printout, then the left one
 * needs to put its trailing_spaces before the right one starts.
 * Obviously, this has to be called _after_ we know our rows.
 */
static void page_find_right_corridors(void);

/* If the y intervals of the bounding rectangles of two words intersect,
 * then the left one should end in printout before the right one starts.
 * This should be called _after_ page_find_right_corridors() because we want
 * to ensure right_wall <= right_corridor .
 */
static void page_find_right_walls(void);

/* Determine on which row in printout words end up. The main vehicle here
 * is a line density function; we force it to have integral >=1 on the
 * interval [word->ceiling, word->brect->ymax].
 * The words are then positioned so that their baseline is in the correct row.
 */
static void page_determine_lines(void);

/* Determine on which column in printout words end up. The main vehicle here
 * is a column density function; we force it to have large enough integrals
 * on the intervals [word->brect->xmin, word->right_wall] resp.
 * [word->brect->xmin, word->right_wall] to accomodate word->output_width
 * resp. word->output_width + word->trailing_spaces columns.
 * The words are then positioned so that their left boundary is in the correct
 * column.
 */
static void page_determine_cols(void);


void page_print_formatted(void)
{
    canv_t canvas;
    vitor_t pwi;

    pmesg(50, "BEGIN page_print_formatted\n");

    /* Life is much easier if we don't have to care for the case
     * of an empty page all the time.
     */
    if(list_head == NULL) {
	puts("\f\n");
	pmesg(80, "This page is empty.\n");
	pmesg(50, "END page_print_formatted\n");
	return;
    }

    page_adjust_diacritics();
    page_adjust_texmext();
    page_adjust_radicals();

    page_collect_statistics();

    max_col_width = (sint32) (4.0 * average_box_width);
    max_row_height = (sint32) (3.0 * average_box_totheight);
    min_col_density = 1.0 / max_col_width;
    min_line_density = 1.0 / max_row_height;
    /*
     * The minimal densities should ensure that large amounts of white space
     * do not completely get lost. The factors 3.0, 4.0 have been determined
     * by rules of thumb and experiments.
     * The (unverified) expectation is that a printout column will be
     * about average_box_width DVI units wide and a printout row will be
     * about 1.5 * average_box_totheight DVI units high.
     */
    scdf_init(&col_density, left_margin, right_margin, min_col_density);
	/* col_density measures the column density at every point between
	 * left and right margin which is required to do proper positioning
	 * of every word.
	 */
    scdf_init(&line_density, top_margin, bottom_margin, min_line_density);
	/* Similar for line_density and lines */

    vlist_init(&page_words);
    page_cut_words();
    page_check_word_collisions();

    page_find_ceilings();
    page_determine_lines();

    page_find_right_corridors();
    page_find_right_walls();
    page_determine_cols();

    /* Now print out everything */
    canv_init(
    	&canvas,
	(sint32) scdf_eval(x2col, right_margin) + 1,
	(sint32) scdf_eval(y2line, bottom_margin) + 1
    );
    for(
    	pwi = vlist_begin(&page_words);
	pwi != vlist_end(&page_words);
	pwi = pwi->next
    ) {
	page_word_t * pw;

	pw = vitor2ptr(pwi, page_word_t);
	canv_put_linebuf(
	    &canvas,
	    pw->text,
	    pw->column,
	    pw->row,
	    pw->output_width
	);
	pw->text = NULL;    /* text is now owned by the canvas */
    }
    canv_write(&canvas, stdout);
    puts("\f\n");

    /* clean up */

    canv_done(&canvas);

    for(
    	pwi = vlist_begin(&page_words);
	pwi != vlist_end(&page_words);
	pwi = pwi->next
    ) {
    	page_word_done(pwi->data);
	free(pwi->data);
    }
    vlist_done(&page_words);

    scdf_done(y2line); free(y2line);
    scdf_done(x2col); free(x2col);
    scdf_done(&col_density);
    scdf_done(&line_density);

    pmesg(50, "END page_print_formatted\n");
}


void page_print_sequential(void)
{
    struct list_node_t * p;
    struct box_t prev_box, curr_box;

    sint32 delta_x = 0, delta_y = 0;
    sint32 dist_x = 0;
    sint32 epsilon_x = 0, epsilon_y = 0;

    /* begin-of-page flag */
    int bop = 1;
    /* begin-of-line flag */
    int bol = 1;
        
    int spaces, spaces_by_prev, spaces_by_curr;
    struct linebuf_t lb;    

    pmesg(50, "BEGIN page_print_sequential\n");

    page_adjust_diacritics();

    linebuf_init(&lb, 0);
    prev_box.width = 1;
    prev_box.height = 1;
    
    for (p = list_head; p != 0; p = p->next) {

        pmesg(80, "node: X = %li, Y = %li, glyph = %li (%c), font = %li\n",
              p->b.x, p->b.y, p->b.glyph, (unsigned char) p->b.glyph, p->b.font);

    	curr_box = p->b;
        if (curr_box.width == 0) curr_box.width = prev_box.width;
        if (curr_box.height == 0) curr_box.height = prev_box.height;

	if (!bop) {
    	    delta_x = curr_box.x - prev_box.x;
	    delta_y = curr_box.y - prev_box.y;
	    dist_x = delta_x - prev_box.width;

    	    /* (arithmetic mean of current and previous) / 20 */
	    epsilon_x = (curr_box.width + prev_box.width + 39) / 40;
	    epsilon_y = (curr_box.height + prev_box.height + 39) / 40;

            /* check for new line */
	    if (
		/* new line */
		delta_y >= 22*epsilon_y ||
		/* new column */
		delta_y <= -60*epsilon_y ||
		/* weird step back */
		dist_x <= -100*epsilon_x
	    ) {
        	pmesg(80, "end of line\n");
		outenc_write(stdout, &lb);
		linebuf_clear(&lb);
		puts("");
		bol = 1;
            }
	    
	    /* check for word breaks, but not at the beginning of a line */
	    if ((dist_x > 0) && (!bol)) {
		spaces_by_prev =
		    font_w_to_space(prev_box.font, dist_x);
		spaces_by_curr =
		    font_w_to_space(curr_box.font, dist_x);
    		spaces = (spaces_by_prev + spaces_by_curr + 1) / 2;

        	if (spaces > 0) {
                    pmesg(80, "setting space\n");
                    linebuf_putg(&lb, ' ');
        	}
	    }
		
	} /* end if (!bop) */

        linebuf_putg(&lb, curr_box.glyph);
	prev_box = curr_box;
	bop = 0;
	bol = 0;
    }
    /* flush line buffer and end page */
    outenc_write(stdout, &lb);
    puts("\n\f\n");
    
    linebuf_done(&lb);
    
    pmesg(50, "END page_print_sequential\n");
}

static int decide_space_count(
    struct list_node_t * prev_p,
    struct list_node_t * p
)
{
        font_t prev_font, curr_font;
        sint32 prev_x, prev_width;
        sint32 curr_x;
        sint32 delta;
        int (* const f2spc)(sint32, sint32) = font_w_to_space;

        prev_font = prev_p->b.font;
        prev_x = prev_p->b.x;
        prev_width = prev_p->b.width;

        curr_font = p->b.font;
        curr_x = p->b.x;

        delta = curr_x - (prev_x + prev_width);
        return (1 + f2spc(curr_font, delta) + f2spc(prev_font, delta)) / 2;
}


/*************** page_word_t method implementations ****************/
void page_word_init(
    page_word_t * this,
    list_node_t * alpha,
    list_node_t * omega,
    int trailing_spaces
)
{
    list_node_t * p, * q;
    rect_t r;

    this->alpha = alpha;
    this->omega = omega;
    this->text = xmalloc(sizeof(linebuf_t));
    linebuf_init(this->text, 20);
    	/* 20 seems a good tradeoff between memory usage and
	 * avoiding reallocs.
	 */
    rect_set_empty(&(this->brect));
    this->ceiling = SINT32_MIN;
    	/* The top line has no ceiling -- it fits automatically */
    this->right_wall = right_margin;
    this->right_corridor = right_margin;
    /* this->output_width will be set below */
    this->trailing_spaces = trailing_spaces;
    this->row = -1; 	/* will be filled in later */
    this->column = -1;	/* will be filled in later */

    q = omega->next;
    for(p = alpha; p != q; p = p->next) {
    	/* add p->b to bounding rect */
    	rect_set(
	    &r,
	    p->b.x,
	    p->b.x + p->b.width,
	    p->b.y - p->b.height,
	    p->b.y
	);
	rect_union_with(&(this->brect), &r);

    	/* add p->b.glyph to text */
    	linebuf_putg(this->text, p->b.glyph);
    }

    this->output_width = outenc_get_width(this->text);
}


void page_word_done(page_word_t * this)
{
    this->alpha = NULL;
    this->omega = NULL;
    /* this->text will be NULL after it has been passed to canvas */
    if(this->text != NULL) {
	linebuf_done(this->text);
	free(this->text);
	this->text = NULL;
    }
}

static void page_words_collide(vitor_t vi, vitor_t vj)
{
    page_word_t * wp, * wq;
    list_node_t * bp, * bq;
    sint32 c;

    pmesg(60, "BEGIN page_words_collide\n");

    wp = vi->data;
    wq = vj->data;

    bp = wp->alpha;
    bq = wq->alpha;
    if(box_sloppy_x_cmp(&bp->b, &bq->b) > 0) {
    	/* We want to assume that *wp starts left from *wq */
	pmesg(60, "calling myself with exchanged arguments\n");
    	page_words_collide(vj, vi);
    	pmesg(60, "END page_words_collide\n");
	return;
    }

    /* Skip all letters of *wp left of *wq. We allow a certain margin overlap
     * to accomodate italics correction, kerning, etc.
     */
    while(c = box_sloppy_x_cmp(&bp->b, &bq->b), c < 0) {
    	if(bp == wp->omega) {
	    /* We're at the end of *wp, so either really no collision
	     * at all, or only the last box collides. Anyway, nothing
	     * to do.
	     */
	    pmesg(60, "nothing to do\n");
    	    pmesg(60, "END page_words_collide\n");
	    return;
	}
    	bp = bp->next;
    }

    if(c == 0) {
	/* Ouch, full crash */
	pmesg(60, "unresolvable collision\n");
    	pmesg(60, "END page_words_collide\n");
	return;
    }

    /* We've moved bp across the beginning of *wq */
    pmesg(60, "resolving collision by splitting word\n");
    vlist_insert_after(&page_words, vi, page_word_split_at(wp, bp));

    /* Check for further collisions between *wq and the right piece of *wp */    
    pmesg(60, "checking for further collisions\n");
    page_words_collide(vj, vi->next);
    pmesg(60, "END page_words_collide\n");
}

static page_word_t * page_word_split_at(
    page_word_t * this,
    struct list_node_t * where
)
{
    page_word_t * righthalf;
    list_node_t * alpha;

    /* No splitting with one half empty, please. */
    assert(where != this->alpha);
    assert(where != this->omega->next);

    /* create right half */
    righthalf = xmalloc(sizeof(page_word_t));
    page_word_init(righthalf, where, this->omega, this->trailing_spaces); 

    /* reinit left half (ourselves) */
    alpha = this->alpha;
    page_word_done(this);
    page_word_init(this, alpha, where->prev, 0);

    return righthalf;
}


sint32 box_sloppy_x_cmp(const struct box_t * p1, const struct box_t * p2)
{
    sint32 m1, m2;  /* midpoints */

    m1 = p1->x + p1->width / 2;
    m2 = p2->x + p2->width / 2;

    /* If the x midpoint of one box is in the x interval of the other,
     * the two are considered close together (and compare "equal").
     * Otherwise, the x midpoints are compared.
     */
    if(p1->x <= m2 && m2 <= p1->x + p1->width) return 0;
    if(p2->x <= m1 && m1 <= p2->x + p2->width) return 0;
    return m1 - m2;
}


/*************** page layout passes ********************************/
void page_collect_statistics(void)
{
    struct list_node_t * p;
    sint32 prev_y;

    pmesg(80, "BEGIN page_collect_statistics\n");

    left_margin = SINT32_MAX;
    right_margin = SINT32_MIN;
    top_margin = SINT32_MAX;
    bottom_margin = SINT32_MIN;

    average_box_width = 0;
    average_box_totheight = 0;
    linecount = 0;
    boxcount = 0;

    prev_y = list_head->b.y;
    for (p = list_head; p != 0; p = p->next) {
    	sint32 x, y, width, height, depth;

	x = p->b.x;
	y = p->b.y;
	width = p->b.width;
	height = p->b.height;
	depth = p->b.depth;

	left_margin = min(left_margin, x);
	right_margin = max(right_margin, x + width);
	top_margin = min(top_margin, y - height);
	bottom_margin = max(bottom_margin, y + depth);

        if (y > prev_y) {
            prev_y = y;
            linecount++;
        }

	average_box_width += width;
	average_box_totheight += (height + depth);
	++boxcount;
    }
    ++linecount; /* The last line hasn't been counted */
    average_box_width = average_box_width / boxcount;
    average_box_totheight = average_box_totheight / boxcount;
    
    pmesg(80, "left margin: %ld, right margin: %ld.\n", \
	left_margin, right_margin);
    pmesg(80, "top margin: %ld, bottom margin: %ld.\n", \
	top_margin, bottom_margin);

    pmesg(80, "END page_collect_statistics\n");
}


void page_cut_words(void)
{
    struct list_node_t * p;

    int bop, eop, bol, eol, bow, eow;
    	/* {begin,end} of {page, line, word} flags */

    pmesg(50, "BEGIN page_cut_words\n");

    bop = 1;
    bol = 1;
    bow = 1;
    for (p = list_head; p != NULL; p = p->next) {
    	sint32 y;
	list_node_t * curr_alpha;

        pmesg(80, "node: X = %li, Y = %li, glyph = %li (%c), font = %li\n",
              p->b.x, p->b.y, p->b.glyph, (unsigned char) p->b.glyph, p->b.font);

    	y = p->b.y;
    	eop = (p->next == NULL);
	eol = eop || (p->next->b.y != y);
    	eow = eol || (decide_space_count(p, p->next) > 0);

    	if(bow) {
    	    curr_alpha = p;
	}

	if(eow) {
	    int trailing_space;
	    page_word_t * new_word;
	    
	    if(eol) {
		trailing_space = 0;
	    }
	    else {
		trailing_space = 1;
	    }

    	    /* Make up a page_word_t from the collected data and insert
	     * it into the page_words list
	     */
	    new_word = xmalloc(sizeof(page_word_t));
	    page_word_init(new_word, curr_alpha, p, trailing_space);
	    vlist_push_back(&page_words, new_word);
	}

    	if(eol) {
            pmesg(80, "end of line\n");
	}

    	bop = 0;
	bol = eol;
    	bow = eow;

    } /* for p */

    pmesg(50, "END page_cut_words\n");

}


static void page_check_word_collisions(void)
{
    vitor_t vi, nvi, vj, llvi;
    page_word_t * p, * np, * q;
    int eop, eol;

    pmesg(50, "BEGIN page_check_word_collisions\n");

    llvi = vlist_rend(&page_words);

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = nvi
    ) {
	p = vi->data;
	nvi = vi->next;
	np = nvi->data;

    	eop = (nvi == vlist_end(&page_words));
	eol = eop || np->brect.ymax != p->brect.ymax;

	/* We only need to look upwards, a collision will then always be caught
	 * by the lower one of the colliding words.
	 * Collisions between words on the same baseline cannot occur
	 * (by design of the basic word break algorithm) and shouldn't
	 * result in new word breaks anyway.
	 *
	 * We keep an iterator to the last word in the previous line (upwards)
	 * to avoid repeated skipping backwards over preceding words
	 * on the same baseline.
	 */
    	for(vj = llvi; vj != vlist_rend(&page_words); vj = vj->prev) {
	    q = vj->data;
    	    if(q->brect.ymax < p->brect.ymin) break;
	    	/* We are too far up to find any more collisions. Note
		 * this happens _immediately_ in the standard (one column
		 * text without intersecting lines) case.
		 */
	    if(!intervals_intersect(
	    	p->brect.xmin, p->brect.xmax,
		q->brect.xmin, q->brect.xmax
	    )) continue;
	    page_words_collide(vi, vj);
	    	/* We do _not_ break if q->brect.xmax < p->brect.xmin
		 * because there may be more than one line with
		 * possible collisions.
		 */
	}

	if(eol) llvi = vi;
    }

    pmesg(50, "END page_check_word_collisions\n");
}


static void page_find_ceilings(void)
{
    vitor_t vi, nvi, vj, llvi;
    page_word_t * p, * np, * q;
    int eop, eol;

    pmesg(50, "BEGIN page_find_ceilings\n");

    llvi = vlist_rend(&page_words);

    /* Our policy is the following:
     * a. Stuff that has its baseline above our head (read p->brect.ymin) must
     *    end up on a higher line.
     * b. Stuff that is near us in x direction and has its baseline above
     *    ours must end up on a higher line.
     */

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = nvi
    ) {
    	sint32 xmin, xmax;  /* the x interval we're interested in */
    	sint32 ymin;        /* we need not care for stuff higher that that */

	p = vi->data;
    	nvi = vi->next;
	np = nvi->data;

    	xmin = p->brect.xmin - 2 * average_box_width;
    	xmax = p->brect.xmax + 2 * average_box_width;
    	ymin = p->brect.ymax - max_row_height;

    	eop = (nvi == vlist_end(&page_words));
	eol = eop || np->brect.ymax != p->brect.ymax;

	/* We only need to look upwards, and no further than max_row_hight
	 *
	 * We keep an iterator to the last word in the previous line (upwards)
	 * to avoid repeated skipping backwards over preceding words
	 * on the same baseline.
	 * Since the list is traversed bottom-to-top, we can stop looking
	 * as soon as we've found the first ceiling candidate.
	 */
    	for(vj = llvi; vj != vlist_rend(&page_words); vj = vj->prev) {
	    q = vj->data;
    	    if(q->brect.ymax < ymin) {
	    	/* stuff so high will end up on another line automatically */
	    	break;
	    }
    	    if(q->brect.ymax < p->brect.ymin) {
	    	p->ceiling = q->brect.ymax;
	    	break;
    	    }
	    if(intervals_intersect(
	    	xmin, xmax,
		q->brect.xmin, q->brect.xmax
	    )) {
    	    	p->ceiling = q->brect.ymax;
		break;
	    }
	}

	if(eol) llvi = vi;
    }

    pmesg(50, "END page_find_ceilings\n");
}


static void page_find_right_corridors(void)
{
    vitor_t vi, nvi, vj, llvi;
    page_word_t * p, * np, * q;
    int eop, eol;

    pmesg(50, "BEGIN page_find_right_corridors\n");

    llvi = vlist_rend(&page_words);

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = nvi
    ) {
	p = vi->data;
    	nvi = vi->next;
	np = nvi->data;

    	eop = (nvi == vlist_end(&page_words));
	eol = eop || np->brect.ymax != p->brect.ymax;

    	/* first we look directly right */
	if(!eol) {
	    p->right_corridor = min(p->right_corridor, np->brect.xmin);
	}
	
	/* Now we look upwards (left and right).
	 *
	 * We keep an iterator to the last word in the previous line (upwards)
	 * to avoid repeated skipping backwards over preceding words
	 * on the same baseline.
	 */
    	for(vj = llvi; vj != vlist_rend(&page_words); vj = vj->prev) {
	    q = vj->data;
    	    if(q->row < p->row) break;
	    	/* We need not look further up */
	    if(q->brect.xmin < p->brect.xmin) {
	    	q->right_corridor = min(q->right_corridor, p->brect.xmin);
	    }
	    else if(p->brect.xmin < q->brect.xmin) {
	    	p->right_corridor = min(p->right_corridor, q->brect.xmin);
	    }
	}

	if(eol) llvi = vi;
    }

    pmesg(50, "END page_find_right_corridors\n");
}


static void page_find_right_walls(void)
{
    vitor_t vi, nvi, vj, llvi;
    page_word_t * p, * np, * q;
    int eop, eol;

    pmesg(50, "BEGIN page_find_right_walls\n");

    llvi = vlist_rend(&page_words);

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = nvi
    ) {
	p = vi->data;
    	nvi = vi->next;
	np = nvi->data;

    	eop = (nvi == vlist_end(&page_words));
	eol = eop || np->brect.ymax != p->brect.ymax;

    	/* This has to be done somewhere. The most natural place is here. */
    	p->right_wall = min(p->right_wall, p->right_corridor);

    	/* We need not look directly right, we've already done so to
	 * find right_corridor. But if we are at the end of the line,
	 * we want stuff on other lines starting clearly right from where we
	 * end to appear right of us.
	 */
    	if(eol) {
	    p->right_wall = min(
	    	p->right_wall,
		p->brect.xmax + average_box_width
	    );
	    /* FIXME: is average_box_width the right amount of slack? Or
	     * use no slack at all ?
	     */
	}

	/* Now we look upwards (left and right).
	 *
	 * We keep an iterator to the last word in the previous line (upwards)
	 * to avoid repeated skipping backwards over preceding words
	 * on the same baseline.
	 */
    	for(vj = llvi; vj != vlist_rend(&page_words); vj = vj->prev) {
	    q = vj->data;
    	    if(q->brect.ymax < p->brect.ymin) break;
	    	/* We need not look further up */
	    if(box_sloppy_x_cmp(&(q->omega->b), &(p->alpha->b)) < 0) {
    	    	/* *q ends left from *p */
	    	q->right_wall = min(q->right_wall, p->brect.xmin);
	    }
	    else if(box_sloppy_x_cmp(&(p->omega->b), &(q->alpha->b)) < 0) {
    	    	/* *q starts right from *p */
	    	p->right_wall = min(p->right_wall, q->brect.xmin);
	    }
	    /* FIXME: what about crashing words? Should we do something here?
	     * And is box_sloppy_x_cmp() really the right criterion?
	     */
	}

	if(eol) llvi = vi;
    }

    pmesg(50, "END page_find_right_walls\n");
}


static void page_determine_lines(void)
{
    vitor_t vi;
    page_word_t * p;

    pmesg(50, "BEGIN page_determine_lines\n");

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = vi->next
    ) {
    	p = vi->data;
    	pmesg(
	    100,
	    "x = %ld, y = %ld, ceiling = %ld\n",
	    p->brect.xmin,
	    p->brect.ymax,
	    p->ceiling
	);

	if(p->ceiling >= top_margin) {
	    scdf_force_min_integral(
	    	&line_density,
		p->ceiling,
		p->brect.ymax,
		1
	    );
	}
    }

    scdf_normalize(&line_density);
    y2line = scdf_floor_of_integral(&line_density);

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = vi->next
    ) {
    	p = vi->data;
	p->row = (sint32) scdf_eval(y2line, p->brect.ymax);
	pmesg(100, "word at row %i\n", p->row);
    }

    pmesg(50, "END page_determine_lines\n");
}


static void page_determine_cols(void)
{
    vitor_t vi, vj;
    page_word_t * p;
    int eop, eol;

    pmesg(50, "BEGIN page_determine_cols\n");

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = vj
    ) {
    	p = vi->data;
    	pmesg(
	    100,
	    "x = %ld, y = %ld, right_wall = %ld, right_corridor = %ld\n",
	    p->brect.xmin,
	    p->brect.ymax,
	    p->right_wall,
	    p->right_corridor
	);
    	assert(p->right_wall <= p->right_corridor);
    	assert(p->right_corridor <= right_margin);

	if(p->right_wall < p->right_corridor) {
	    scdf_force_min_integral(
		&col_density,
		p->brect.xmin,
		p->right_wall,
		p->output_width
	    );
	}
	if(p->right_wall == p->right_corridor || p->trailing_spaces > 0) {
    	    scdf_force_min_integral(
		&col_density,
		p->brect.xmin,
		p->right_corridor,
		p->output_width + p->trailing_spaces
	    );
	}

	vj = vi->next;
	eop = (vj == vlist_end(&page_words));
	eol = eop || (p->brect.ymax != vitor2ptr(vj, page_word_t)->brect.ymax);
	if(eol) scdf_normalize(&col_density);
    }

    x2col = scdf_floor_of_integral(&col_density);

    for(
    	vi = vlist_begin(&page_words);
	vi != vlist_end(&page_words);
	vi = vi->next
    ) {
    	p = vi->data;
	p->column = (sint32) scdf_eval(x2col, p->brect.xmin);
	pmesg(100, "word at column %i\n", p->column);
    }

    pmesg(50, "END page_determine_cols\n");
}


/***************** rect_t method implementations **********************/
int intervals_intersect(
    sint32 a, sint32 b,
    sint32 c, sint32 d
)
{
    if(a > b || c > d) return 0; /* One of the intervals is empty */
    return c <= b && a <= d;	/* yes, this works in ALL remaining cases :) */
}

int rect_intersects(const rect_t * this, const rect_t * that)
{
    return (
    	intervals_intersect(this->xmin, this->xmax, that->xmin, that->xmax)
	&& intervals_intersect(this->ymin, this->ymax, that->ymin, that->ymax)
    );
}

int rect_contains_point(const rect_t * this, sint32 x, sint32 y)
{
    return (
    	this->xmin <= x && x <= this->xmax
	&& this->ymin <= y && y <= this->ymax
    );
}

void rect_set(
    rect_t * this,
    sint32 xmin,
    sint32 xmax,
    sint32 ymin,
    sint32 ymax
)
{
    this->xmin = xmin;
    this->xmax = xmax;
    this->ymin = ymin;
    this->ymax = ymax;
}

void rect_set_empty(rect_t * this)
{
    rect_set(this, 1, 0, 1, 0);
}

int rect_is_empty(const rect_t * this)
{
    return this->xmin > this->xmax || this->ymin > this->ymax;
}

void rect_union_with(rect_t * this, const rect_t * that)
{
    if(rect_is_empty(that)) return;

    if(rect_is_empty(this)) *this = *that;
    else {
    	this->xmin = min(this->xmin, that->xmin);
    	this->xmax = max(this->xmax, that->xmax);
    	this->ymin = min(this->ymin, that->ymin);
    	this->ymax = max(this->ymax, that->ymax);
    }
}
