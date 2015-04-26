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

#include <assert.h>
#include <stdlib.h>
#include "page2.h"
#include "util.h"
#include "glyphops.h"
#include "layout.h"

/* Return value like with strcmp and friends. */
static int compare_boxes(struct box_t a, struct box_t b)
{
        if (a.y > b.y) return 1;
        if (a.y < b.y) return -1;
        assert(a.y == b.y);
        if (a.x > b.x) return 1;
        if (a.x < b.x) return -1;
        assert(a.x == b.x);
        return 0;
}

struct point_t {
    sint32 x;
    sint32 y;
};

static int point_in_box(const struct point_t * point, const struct box_t * box)
{
    assert(box);
    assert(point);
    return(
    	box->x      	    	<= point->x &&
	(box->x + box->width) 	>= point->x &&
	(box->y - box->height) 	<= point->y &&
	box->y      	    	>= point->y
    );
}

/* Set to 1 when catdvi is invoked with the --sequential option */
int page_sequential = 0;

/* Set to 1 when catdvi is invoked with the --list-page-numbers option */
int page_list_numbers = 0;

/* List support variables.  Note that most insertions happen near the
   previous insertion, so it's beneficial to keep a pointer to the
   last inserted node. */
struct list_node_t * list_head;
struct list_node_t * list_tail;
struct list_node_t * list_latest; /* Node that was inserted last. */

/* flags to avoid the page_adjust_*() loops over the whole page when
 * there's nothing to adjust.
 */
static int page_has_diacritics;
static int page_has_texmext;
static int page_has_radicals;

static void swap_nodes(struct list_node_t * one, struct list_node_t * other)
{
        assert(one != 0);
        assert(other != 0);
        assert(one->next == other);
        assert(one == other->prev);

        one->next = other->next;
        other->prev = one->prev;
        other->next = one;
        one->prev = other;
        if (one->next == 0) {
                list_tail = one;
        } else {
                one->next->prev = one;
        }
        if (other->prev == 0) {
                list_head = other;
        } else {
                other->prev->next = other;
        }

        assert(other->next == one);
        assert(other == one->prev);
}

/* move list_latest into its correct position in the list */
static void bubble(void)
{
        while (1) {
                assert(list_latest != 0);

                /* If it is in correct position now, end here. */
                if ((list_latest->prev == 0
                     || compare_boxes(list_latest->prev->b, list_latest->b) <= 0)
                    && (list_latest->next == 0
                        || compare_boxes(list_latest->b, list_latest->next->b) <= 0)) {
                        return;
                }

                /* If it is too left, we must bubble it one position to the right. */
                if (list_latest->next != 0
                    && compare_boxes(list_latest->b, list_latest->next->b) > 0) {
                        swap_nodes(list_latest, list_latest->next);
                        continue;
                }

                /* If it is too right, we must bubble it one position to the left. */
                if (list_latest->prev != 0
                    && compare_boxes(list_latest->prev->b, list_latest->b) > 0) {
                        assert(list_latest->prev->next == list_latest);
                        swap_nodes(list_latest->prev, list_latest);
                        continue;
                }
                NOTREACHED;
        }
        NOTREACHED;
}


/* move some random node into its correct position in the list */
static void bubble_node(struct list_node_t * p)
{
    list_latest = p;
    bubble();
}


static void insert_list(struct box_t box)
{
        struct list_node_t * new;

        new = malloc(sizeof(struct list_node_t));
        if (new == 0) enomem();
        new->b = box;

        if (list_head == 0 || list_tail == 0 || list_latest == 0) {
                assert(list_head == 0);
                assert(list_tail == 0);
                assert(list_latest == 0);

                new->next = new->prev = 0;

                list_head = list_tail = list_latest = new;
                return;
        }

        /* The list is nonempty. */

        assert(list_head != 0);
        assert(list_tail != 0);
        assert(list_latest != 0);

        /* The insertion algorithm is this: insert the node after the
           latest insertion, and then bubble it into its correct
           position.  */

        new->next = list_latest->next;
        list_latest->next = new;
        new->prev = list_latest;
        list_latest = new;
        if (new->next == 0) {
                list_tail = new;
        } else {
                new->next->prev = new;
        }

        assert(list_latest->prev->next == list_latest);
        assert(list_latest->next == 0 || list_latest->next->prev == list_latest);

    	/* sort by position except in sequential mode */
        if (!page_sequential) bubble();
}


/* These two are set by command line options */
struct pageref_t page_start_output = {0, 0, 0, PRF_PHYSICAL};
struct pageref_t page_last_output = {SINT32_MAX, 0, 0, PRF_PHYSICAL};

static struct pageref_t current_page = {0, 0, 0, PRF_INVALID};


void page_begin(sint32 count0)
{
        /* If necessary, delete the previous page's data structure. */
        if (list_head != 0 || list_tail != 0 || list_latest != 0) {
                struct list_node_t * p;

                assert(list_head != 0);
                assert(list_tail != 0);
                assert(list_latest != 0);
                
                p = list_head;
                while (p != 0) {
                        struct list_node_t * next;

                        if (p == list_head)   list_head = 0;
                        if (p == list_latest) list_latest = 0;
                        if (p == list_tail)   list_tail = 0;

                        next = p->next;
                        free(p);
                        p = next;
                }
        }
        assert(list_head == 0);
        assert(list_tail == 0);
        assert(list_latest == 0);

    	/* reset per page flags */
	page_has_diacritics = 0;
	page_has_texmext = 0;
	page_has_radicals = 0;

	/* keep track of page numbering */
	if(current_page.flavour == PRF_INVALID) {
	    /* here comes the very first page of the document */
	    current_page.physical = 1;
	    current_page.count0 = count0;
	    if (count0 < 0) current_page.chapter = 1;
	    else current_page.chapter = 0;
	    	/* number chapters from 0 to accomodate frontmatter */
	    current_page.flavour = PRF_COMPLETE;
	}
	else {
	    /* just another ordinary page */
    	    current_page.physical += 1;
	    if (pageref_count0_cmp(current_page.count0, count0) >= 0) {
		/* count0 has not increased, so start new chapter */
		current_page.chapter += 1;
	    }
	    current_page.count0 = count0;
	}
}


void page_set_glyph(
    font_t font, glyph_t glyph,
    sint32 width, sint32 height, sint32 depth, sint32 axis_height,
    sint32 x, sint32 y
)
{
    struct box_t b;
    enum glyph_hint_t hint;

    b.x = x;
    b.y = y;
    b.width = width;
    b.height = height;
    b.depth = depth;
    b.glyph = glyph;
    b.font = font;
    b.axis_height = axis_height;
    b.flags = 0;
    b.axis = 0;

    hint = glyph_get_hint(glyph);

    if(hint & GH_DIACRITIC) {
	b.flags |= BF_SWIMMING | BF_DIACRITIC ;
	page_has_diacritics = 1;
    }

    if(hint & GH_EXTENSIBLE_RECIPE) {
	b.flags |= BF_SWIMMING;
	page_has_texmext = 1;
    }
    else if(hint & GH_ON_AXIS) {
	b.flags |= BF_SWIMMING | BF_ON_AXIS | BF_HAS_AXIS ;
	b.axis = y + (-height + depth)/2;
	page_has_texmext = 1;
    }
    else if(hint & GH_RADICAL) {
	b.flags |= BF_SWIMMING | BF_RADICAL ;
	page_has_radicals = 1;
    }
    else if(axis_height > 0) {
	b.flags |= BF_HAS_AXIS;
	b.axis = y - axis_height;
    }

    insert_list(b);
}


static void page_pair_accenting(
    struct list_node_t * pbase,
    struct list_node_t * pdiacritic,
    int direction
);

void page_end(void)
{
    pmesg(50, "BEGIN page_end\n");

    if (
	pageref_cmp(&page_start_output, &current_page) > 0 ||
	pageref_cmp(&page_last_output, &current_page) < 0
    ) {
	if(msglevel >= 80) {
	    pmesg(80, "skipping page by user request:\n  ");
	    pageref_print(&current_page, stderr);
	}
	pmesg(50, "END page_end\n");
	return;
    }

    if(page_list_numbers) {
	pageref_print(&current_page, stdout);
	pmesg(50, "END page_end\n");
	return;
    }

    if(page_sequential) page_print_sequential();
    else page_print_formatted();
    
    pmesg(50, "END page_end\n");
    return;

}

void page_adjust_diacritics(void)
{
    int trouble = 0;
    int direction = 0;
    
    struct list_node_t *p, *s, *q, *pb;
    struct box_t dia_box;
    struct point_t dia_center;
        
    pmesg(50, "BEGIN page_adjust_diacritics\n");
    
    if(!page_has_diacritics) {
    	pmesg(80, "nothing to do\n");
    	pmesg(50, "END page_adjust_diacritics\n");
	return;
    }

    for (p = list_head; p != 0; p = s) {
    	s = p->next;
	    /* p-> is a moving target */
	
	if (!(p->b.flags & BF_DIACRITIC)) continue;

	trouble = 0;
	pb = 0;

    	dia_box = p->b;
	dia_center.x = dia_box.x + dia_box.width / 2;
	dia_center.y = dia_box.y - dia_box.height / 2;
    	
	/* Search corresponding base box. There are complex overlayed
	 * constructions we can't handle. Therefore, we try to find all
	 * possible candidates and abort when more than one is found.
	 */
	 
	/* search backwards */
	for(q = p->prev; q != 0; q = q->prev) {
	    if (abs(dia_box.y - q->b.y) > 3*(dia_box.height + q->b.height))
	    	/* too far off to find anything useful */
	    	break;
	    
	    if (point_in_box(&dia_center, &q->b)) {
	    	/* I got you babe */
    	    	if(pb != 0) {
		    /* we are not alone...too bad */
		    trouble = 1;
		    break;
		}
		pb = q;
		direction = -1;
	    }
	}
	
	if(trouble) {
	    pmesg(
	    	80,
		"page_adjust_diacritics: trouble with diacritic %#lx\n",
		dia_box.glyph
	    );
	    continue;
	}
	
	/* search forward */
	for(q = p->next; q != 0; q = q->next) {
	    if (abs(dia_box.y - q->b.y) > 3*(dia_box.height + q->b.height))
	    	/* too far off to find anything useful */
	    	break;
	    
	    if (point_in_box(&dia_center, &q->b)) {
	    	/* I got you babe */
    	    	if(pb != 0) {
		    /* we are not alone...too bad */
		    trouble = 1;
		    break;
		}
		pb = q;
		direction = 1;
	    }
	}
	
	if(trouble) {
	    pmesg(
	    	80,
		"page_adjust_diacritics: trouble with diacritic %#lx\n",
		dia_box.glyph
	    );
	    continue;
	}
	if(pb == 0) {
	    pmesg(
	    	80,
		"page_adjust_diacritics: no base glyph for diacritic %#lx\n",
		dia_box.glyph
	    );
	    /* a lone diacritic. assume it just belongs where it is */
	    p->b.flags &= ~(BF_SWIMMING | BF_DIACRITIC);
	    continue;
	}
	
	/* Found the one and only one. Let's get closer. */
	page_pair_accenting(pb, p, direction);
    }

    pmesg(50, "END page_adjust_diacritics\n");
}


static void page_pair_accenting(
    struct list_node_t * pbase,
    struct list_node_t * pdiacritic,
    int direction
)
{
    glyph_t dcv;

    assert(pdiacritic->b.flags & BF_DIACRITIC);
    assert(abs(direction) == 1);
    
    pmesg(
	80,
	"detected accenting: base=%#lx, diacritic=%#lx\n",
	pbase->b.glyph,
	pdiacritic->b.glyph
    );

    dcv = diacritic_combining_variant(pdiacritic->b.glyph);
    
    if(dcv) {
    	/* OK this one can be handeled within the unicode framework */
	
	/* move diacritic right after base glyph */
	if(direction == -1) {
	    /* base before diacritic */
	    while(pdiacritic->prev != pbase) {
		swap_nodes(pdiacritic->prev, pdiacritic);
	    }
	}
	else {
	    /* base after diacritic */
	    while(pdiacritic->prev != pbase) {
		swap_nodes(pdiacritic, pdiacritic->next);
		    /* note swap_nodes() is order sensitive */
	    }
	}
	
    	/* Make the combining diacritic occupy the same space as the
	 * accented glyph would. That's the only way to statisfy all
	 * ordering and spacing assumptions in this program. And nearly
	 * consequent.
	 */
	pdiacritic->b.width = pbase->b.width;
	pdiacritic->b.height = max(
	    pbase->b.height,
	    pdiacritic->b.height + pbase->b.y - pdiacritic->b.y
	);
	pdiacritic->b.x = pbase->b.x;
	pdiacritic->b.y = pbase->b.y;
	pdiacritic->b.glyph = dcv;
	pdiacritic->b.flags &= ~(BF_SWIMMING | BF_DIACRITIC);
    }
    else {
    	/* A strange guy. Won't combine. Just put him in line so he can't
	 * clobber up things. Like in real life.
	 */
	pdiacritic->b.height =
	    pdiacritic->b.height + pbase->b.y - pdiacritic->b.y;
	pdiacritic->b.y = pbase->b.y;
	pdiacritic->b.flags &= ~(BF_SWIMMING | BF_DIACRITIC);

	/* keep the list ordered */
	if(!page_sequential) bubble_node(pdiacritic);
    }

}


/************************************************************************
 * the hairy texmext stuff
 ************************************************************************/

typedef struct interval32_t interval32_t;
struct interval32_t {
    sint32 a;
    sint32 b;
};

static int interval32_contains(const interval32_t * this, sint32 x)
{
    int res;
    
    res = (this->a <= x) && (x <= this->b);
    
    pmesg(
    	150,
	"interval32_contains: %ld %s [%ld, %ld]\n",
	x,
	res ? "in" : "not in",
	this->a,
	this->b
    );

    return res;
}


typedef struct searchinfo_t searchinfo_t;
struct searchinfo_t {
    interval32_t searched_y;
    interval32_t x;
    interval32_t y;
    interval32_t top;
    interval32_t axis;
    int require_axis;
    int test_start;
    struct list_node_t * start;
    struct list_node_t * first_found;
};

static int page_match_box(
    const struct searchinfo_t * quest,
    const struct list_node_t * candidate
)
{
    int matches;
    const struct box_t * cb;
    
    cb = &(candidate->b);

    /* The candidate's x, y and top ( == y - height) values must match the ones
     * we're searching for. If candidate has a known math axis, it must match.
     * require_axis says whether we require it to have a known math axis.
     */
    matches =
    	interval32_contains(&(quest->x), cb->x) &&
    	interval32_contains(&(quest->y), cb->y) &&
	interval32_contains(&(quest->top), cb->y - cb->height) &&
    	(
	    (cb->flags & BF_HAS_AXIS) ?
	    	interval32_contains(&(quest->axis), cb->axis) :
	    	!quest->require_axis
	);  /* yes these parentheses _are_ neccessary */

    return(matches);
}

/* Return values:
 * 0 - no box matches the given criteria
 * 1 - some boxes match the given criteria, and all of them have the same y
 * 2 - some boxes match the given criteria, but different y values occur.
 *
 * If some boxes match, quest->first_found will point to guess what.
 */
static int page_grb_unique_y(
    struct searchinfo_t * quest
)
{
    interval32_t * range;
    struct list_node_t * p;
    struct list_node_t * start_up, * start_down;
    sint32 y;
    int have_y = 0;


    range = &(quest->searched_y);
    assert(interval32_contains(range, quest->start->b.y));
    start_up = quest->test_start ? quest->start : quest->start->prev;
    start_down = quest->start->next;
    
    quest->first_found = NULL;
    
    for(p = start_up; p != NULL; p = p->prev) {
    	if(!interval32_contains(range, p->b.y)) break;
	if(p->b.flags & BF_SWIMMING) continue;
	if(page_match_box(quest, p)) {
	    if(have_y) {
	    	if(p->b.y != y) {
		    return 2;
		}
	    }
	    else {
	    	quest->first_found = p;
		y = p->b.y;
		have_y = 1;
	    }
	}
    }

    for(p = start_down; p != NULL; p = p->next) {
    	if(!interval32_contains(range, p->b.y)) break;
	if(p->b.flags & BF_SWIMMING) continue;
	if(page_match_box(quest, p)) {
	    if(have_y) {
	    	if(p->b.y != y) {
		    return 2;
		}
	    }
	    else {
	    	quest->first_found = p;
		y = p->b.y;
		have_y = 1;
	    }
	}
    }

    return have_y;
}


enum srestrict_t {
    SRS_MIN_VAL = -1,	/* always first */
    SRS_REQUIRE_AXIS,
    SRS_NARROW,
    SRS_VERY_NARROW,
    SRS_MOREMATH_SIDE,
    SRS_LESSMATH_SIDE,
    SRS_MAX_VAL     	/* always last */
};

static int srestrict_conflicts[SRS_MAX_VAL] = {
    0,      	    	    /* SRS_REQUIRE_AXIS */
    1 << SRS_VERY_NARROW,   /* SRS_NARROW */
    1 << SRS_NARROW,	    /* SRS_VERY_NARROW */
    1 << SRS_MOREMATH_SIDE, /* SRS_MOREMATH_SIDE */
    1 << SRS_LESSMATH_SIDE  /* SRS_LESSMATH_SIDE */
};


/* Recurse through all subsets of the set of search restrictions.
 * Return values:
 * 0 - No box conforms to the given set of restrictions. Cut that branch.
 * 1 - The given restrictions or a further refinement have lead to a unique
 *     y value.
 * 2 - There were boxes conforming to the given set of restrictions, but
 *     the refinements I've tried haven't lead to a unique y value.
 */
static int page_grb_recursion(
    struct searchinfo_t * quest,
    struct list_node_t * swimmer,
    enum srestrict_t try_restrict,
    int srestricts
)
{
    struct searchinfo_t newquest;
    int res;
    struct box_t * pb;
    sint32 d;
    
    static char indents[] = "                 ";
    char * indent;
    
    indent = indents + lengthof(indents) + try_restrict - SRS_MAX_VAL;
    
    pmesg(130, "%sBEGIN page_grb_recursion\n", indent);
    pmesg(150, "%spage_grb_recursion: try_restrict=%d\n", indent, try_restrict);

    if(try_restrict < 0) {
    	/* The set of applicable search restrictions is already settled.
	 * Really DO something.
	 */
	res = page_grb_unique_y(quest);
        pmesg(
	    150,
	    "%spage_grb_recursion: srestricts=%#4x, unique_y result %d\n",
	    indent,
	    srestricts,
	    res
	);
    	pmesg(130, "%sEND page_grb_recursion\n", indent);
	return res;
    }

    /* Try without imposing a new restriction first */
    res = page_grb_recursion(quest, swimmer, try_restrict - 1, srestricts);
    if(res <= 1) {
    	pmesg(130, "%sEND page_grb_recursion\n", indent);
    	return res;
    }
    
    /* This hasn't been restrictive enough, try adding "our" restriction. */
    
    if(srestrict_conflicts[try_restrict] & srestricts) {
    	/* We can't add "our" restriction - conflict. But our caller could
	 * try others.
	 */
    	pmesg(150, "%spage_grb_recursion: tried conflicting restrictions\n", indent);
    	pmesg(130, "%sEND page_grb_recursion\n", indent);
	return(2);
    }

    pb = &(swimmer->b);
    d = ((pb->height + pb->depth) + pb->width) / 20;
    	/* (arithmetic mean of total height and width of swimmer)/10 */
    newquest = *quest;
    switch(try_restrict) {
    	case SRS_REQUIRE_AXIS:
	    newquest.require_axis = 1;
	    break;
	case SRS_NARROW:
	    newquest.x.a = max(newquest.x.a, pb->x - 30*d);
	    newquest.x.b = min(newquest.x.b, pb->x + 40*d);
	    	/* 10 for the swimmer itself */
	    break;
	case SRS_MOREMATH_SIDE:
	    if(glyph_get_hint(pb->glyph) & GH_MOREMATH_LEFT) {
	    	newquest.x.b = pb->x + pb->width;
	    }
	    else {
	    	newquest.x.a = pb->x;
	    }
	    break;
	case SRS_LESSMATH_SIDE:
	    if(glyph_get_hint(pb->glyph) & GH_MOREMATH_LEFT) {
	    	newquest.x.a = pb->x;
	    }
	    else {
	    	newquest.x.b = pb->x + pb->width;
	    }
	    break;
	case SRS_VERY_NARROW:
	    newquest.x.a = max(newquest.x.a, pb->x - 15*d);
	    newquest.x.b = min(newquest.x.b, pb->x + 25*d);
	    	/* 10 for the swimmer itself */
	    break;
	default:
	    NOTREACHED;
    }

    res = page_grb_recursion(
    	&newquest,
	swimmer,
	try_restrict - 1,
	srestricts | (1 << try_restrict)
    );

    quest->first_found = newquest.first_found;
    pmesg(130, "%sEND page_grb_recursion\n", indent);
    if(res == 1) return 1;
    else return 2;

}


/* The fonts in "TeX math extension" encoding are somewhat different from
 * the others. The glyphs are centered on the "math axis" (about the height
 * of a minus sign above the baseline of the surrounding text/formula). The
 * y coordinate of the glyphs reference point is mostly meaningless (and in
 * Knuths cmex fonts the glyphs are "hanging down" from the reference
 * point). We know the math axis: its the arithmethic mean of (y - height)
 * and (y + depth). But we have to move the glyph to the _baseline_ for
 * proper text formatting, hence we need to deduce the baseline somehow.
 *
 * TeX gets the height of the math axis above the baseline from a parameter
 * named axis_height in the font metrics of the currently used math symbol
 * font. This parameter is only present in TFM files for fonts with "TeX math
 * symbols" encoding and it is not easy to be sure which axis height was
 * in effect when the math extension glyph was set, given only the DVI file.
 *
 * Therefore, we try another approach first: we look at the surrounding
 * boxes. If every box in some reasonable neighbourhood that is dissected
 * by the math axis has the same baseline, this must be the baseline of
 * the formula. Only if this approach fails we resort to guessing
 * axis_height by looking at the loaded math symbol fonts.
 */
static struct list_node_t * page_guess_reference_box(
    struct list_node_t * swimmer
)
{
    struct searchinfo_t quest;
    struct box_t * pb;
    
    pmesg(50, "BEGIN page_guess_reference_box\n");
    pmesg(
    	80,
	"page_guess_reference_box: y=%ld, axis=%ld\n",
	swimmer->b.y,
	swimmer->b.axis
    );

    pb = &(swimmer->b);
    
    assert(pb->flags & BF_SWIMMING);
    assert(pb->flags & BF_HAS_AXIS);

    quest.x.a = SINT32_MIN;
    quest.x.b =  SINT32_MAX;
    quest.y.a = pb->axis;
    quest.y.b = pb->y + pb->depth;
    quest.top.a = SINT32_MIN;
    quest.top.b = pb->axis;
    quest.axis.a = pb->axis - 2;
    quest.axis.b = pb->axis + 2; /* allow small rounding errors */
    quest.require_axis = 0;
    quest.test_start = 0;
    quest.start = swimmer;


    if(page_sequential) {
    	sint32 h;
	
    	h = pb->height + pb->depth;
	quest.searched_y.a = pb->axis - 3*h;
	quest.searched_y.b = pb->axis + 3*h;
    }
    else {
    	quest.searched_y.a = min(pb->axis, pb->y);
    	quest.searched_y.b = pb->y + pb->depth;
    }

    if(page_grb_recursion(&quest, swimmer, SRS_MAX_VAL - 1, 0) == 1) {
    	pmesg(50, "END page_guess_reference_box\n");
    	return quest.first_found;
    }
    else {
    	pmesg(50, "END page_guess_reference_box\n");
    	return NULL;
    }

}


void page_adjust_texmext(void)
{
    struct list_node_t * p, * s, * ref;
    sint32 new_y, delta;
        
    pmesg(50, "BEGIN page_adjust_texmext\n");

    if(!page_has_texmext) {
    	pmesg(80, "nothing to do\n");
    	pmesg(50, "END page_adjust_texmext\n");
	return;
    }

    for (p = list_head; p != 0; p = s) {
    	s = p->next;
	    /* p-> is a moving target */
	
	if (!(p->b.flags & BF_ON_AXIS)) continue;
	assert(p->b.flags & BF_HAS_AXIS);
	assert(p->b.flags & BF_SWIMMING);

    	pmesg(
	    80,
	    "page_adjust_texmext: glyph=%#6lx, y=%ld\n",
	    p->b.glyph,
	    p->b.y
	);
	
	if(p->b.axis_height > 0) {
	    ref = NULL;
	    pmesg(
	    	80,
		"page_adjust_texmext: known axis_height=%ld\n",
		p->b.axis_height
	    );
	}
	else {
    	    ref = page_guess_reference_box(p);
	    pmesg(
	    	80,
		"page_adjust_texmext:%s reference box found\n",
		ref ? "" : " no"
	    );
	}

	if(ref != NULL)  new_y = ref->b.y;
	else new_y = p->b.axis + abs(p->b.axis_height);
	pmesg(80, "page_adjust_texmext: new_y=%ld\n", new_y);
	
	delta = new_y - p->b.y;
	p->b.y = new_y;
	p->b.height += delta;
	p->b.depth -= delta;
	p->b.axis_height = p->b.y - p->b.axis;
	p->b.flags &= ~(BF_SWIMMING | BF_ON_AXIS);
	
	if(!page_sequential) bubble_node(p);

    }

    pmesg(50, "END page_adjust_texmext\n");
}


static struct list_node_t * page_find_lowest_box(struct searchinfo_t * quest)
{
    interval32_t * range;
    struct list_node_t * p, * low;
    struct list_node_t * start_up, * start_down;

    range = &(quest->searched_y);
    assert(interval32_contains(range, quest->start->b.y));
    start_up = quest->test_start ? quest->start : quest->start->prev;
    start_down = quest->start->next;
    
    quest->first_found = NULL;
    low = NULL;
    
    for(p = start_up; p != NULL; p = p->prev) {
    	if(!interval32_contains(range, p->b.y)) break;
	if(p->b.flags & BF_SWIMMING) continue;
	if(page_match_box(quest, p)) {
	    if(low == NULL || p->b.y > low->b.y) low = p;
	}
    }

    for(p = start_down; p != NULL; p = p->next) {
    	if(!interval32_contains(range, p->b.y)) break;
	if(p->b.flags & BF_SWIMMING) continue;
	if(page_match_box(quest, p)) {
	    if(low == NULL || p->b.y > low->b.y) low = p;
	}
    }

    quest->first_found = low;
    return low;
}


/* TeX's radical signs are hanging down from their reference point. We move
 * them to the baseline of the lowest glyph in the radicant (that's where
 * the bottom tip ought to be) or, if there's no radicant, to the y position
 * of the bottom tip.
 */
void page_adjust_radicals(void)
{
    struct list_node_t * p, * s, * lowest;
    sint32 new_y, delta;
    struct searchinfo_t quest;
        
    pmesg(50, "BEGIN page_adjust_radicals\n");

    if(!page_has_radicals) {
    	pmesg(80, "nothing to do\n");
    	pmesg(50, "END page_adjust_radicals\n");
	return;
    }

    for (p = list_head; p != 0; p = s) {
    	s = p->next;
	    /* p-> is a moving target */
	
	if (!(p->b.flags & BF_RADICAL)) continue;
	assert(p->b.flags & BF_SWIMMING);

    	pmesg(
	    80,
	    "page_adjust_radicals: glyph=%#6lx, y=%ld\n",
	    p->b.glyph,
	    p->b.y
	);

    	/* put the radical on the baseline of the lowest box in the radicant */
	quest.x.a = p->b.x;
	quest.x.b =  SINT32_MAX;
	    /* FIXME: Once we have rudimentary rules support, we can search
	     * for the rule forming the radicals top and take its length as
	     * the width of the radicant.
	     */
	quest.y.a = p->b.y - p->b.height;
	quest.y.b = p->b.y + p->b.depth;
	quest.top = quest.y;
	quest.axis.a = SINT32_MIN;
	quest.axis.b = SINT32_MAX;
	quest.require_axis = 0;
	quest.test_start = 0;
	quest.start = p;

	if(page_sequential) {
	    sint32 h;

    	    h = p->b.height + p->b.depth;
	    quest.searched_y.a = p->b.y - h;
	    quest.searched_y.b = p->b.y + 2*h;
	}
	else {
    	    quest.searched_y = quest.y;
	}

    	lowest = page_find_lowest_box(&quest);
	pmesg(
	    80,
	    "page_adjust_radicals:%s lowest box found\n",
	    lowest ? "" : " no"
	);

	if(lowest != NULL)  new_y = lowest->b.y;
	else new_y = p->b.y + p->b.depth;
	pmesg(80, "page_adjust_radicals: new_y=%ld\n", new_y);
	
	delta = new_y - p->b.y;
	p->b.y = new_y;
	p->b.height += delta;
	p->b.depth -= delta;
	p->b.flags &= ~(BF_SWIMMING | BF_RADICAL);
	
	if(!page_sequential) bubble_node(p);

    }

    pmesg(50, "END page_adjust_radicals\n");
}
