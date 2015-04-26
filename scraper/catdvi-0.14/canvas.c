/* catdvi - get text from DVI files
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

#include <stdlib.h>
#include <assert.h>
#include "util.h"
#include "canvas.h"
#include "glyphenm.h"
#include "outenc.h"


typedef enum canv_celltype_t {
    CCT_UNUSED,
    CCT_BLACK,
    CCT_LINEDRAW,
    CCT_LINEBUF_CONT,
    CCT_GLYPH,
    CCT_LINEBUF_START
} canv_celltype_t;
    /* The order matters. */

typedef enum canv_linedraw_t {
    CLD_RIGHT	= 1 << 1,
    CLD_UP  	= 1 << 2,
    CLD_LEFT	= 1 << 3,
    CLD_DOWN	= 1 << 4
} canv_linedraw_t;

typedef union canv_celldata_t {
    /* */   	    	    /* CCT_UNUSED */
    /* */   	    	    /* CCT_BLACK */
    canv_linedraw_t draw;   /* CCT_LINEDRAW */
    glyph_t glyph;  	    /* CCT_GLYPH */
    linebuf_t * lb; 	    /* CCT_LINEBUF_START, CCT_LINEBUF_CONT */
} canv_celldata_t;

struct canv_cell_t {
    canv_celltype_t type;
    canv_celldata_t data;
};

struct canv_line_t {
    canv_cell_t * cells;
    sint32 cols_used;
};

#define canv_cell(this, x, y) ((this)->lines[(y)].cells + (x))

#define CANV_CHECK_X(this, x) {assert(0 <= (x)); assert((x) < (this)->ncols);}
#define CANV_CHECK_Y(this, y) {assert(0 <= (y)); assert((y) < (this)->nlines);}


/* Claim cell at (x, y) for use as celltype whatfor.
 * Returns pointer to cell if succesful, NULL if cell already used
 * for something equally or more important.
 */
static canv_cell_t * canv_allocate_cell(
    canv_t * this,
    sint32 x,
    sint32 y,
    canv_celltype_t whatfor
);

/* Add line drawings draw to cell at (x, y).
 * Returns pointer to cell if succesful, NULL if cell already used
 * for something more important.
 */
static canv_cell_t * canv_linedraw_to_cell(
    canv_t * this,
    sint32 x,
    sint32 y,
    canv_linedraw_t draw
);


void canv_init(canv_t * this, sint32 ncols, sint32 nlines)
{
    sint32 i, j;
    canv_line_t * l;

    assert(ncols > 0);
    assert(nlines > 0);
    this->ncols = ncols;
    this->nlines = nlines;
    
    this->cells = xmalloc(ncols * nlines * sizeof(canv_cell_t));
    this->lines = xmalloc(nlines * sizeof(canv_line_t));
    
    for(i = 0; i < nlines; ++i) {
    	l = this->lines + i;
    	l->cols_used = 0;
	l->cells = this->cells + i * ncols;
	for(j = 0; j < ncols; ++j) {
	    l->cells[j].type = CCT_UNUSED;
	}
    }
}

void canv_done(canv_t * this)
{
    sint32 i, j;
    canv_cell_t * c;

    for(i = 0; i < this->nlines; ++i) {
    	for(j = 0; j < this->ncols; ++j) {
	    c = canv_cell(this, j, i);
	    if(c->type == CCT_LINEBUF_START) {
	    	linebuf_done(c->data.lb);
	    	free(c->data.lb);
	    }
	}
    }

    free(this->lines);
    free(this->cells);
}

void canv_put_linebuf(
    canv_t * this,
    linebuf_t * lb,
    sint32 xleft,
    sint32 y,
    sint32 width
)
{
    canv_cell_t * cell;
    sint32 x;

    CANV_CHECK_X(this, xleft);
    CANV_CHECK_Y(this, y);

    if(width <= 0) {
    	/* This shouldn't happen, but is no reason to bomb out. */
	linebuf_done(lb);
	free(lb);
	pmesg(
	    10,
	    "Attempt to put <= 0 (%ld) width linebuf at (%ld, %ld) !\n",
	    width,
	    xleft,
	    y
	);
    	return;
    }

    cell = canv_allocate_cell(this, xleft, y, CCT_LINEBUF_START);
    if(cell == NULL) {
    	/* If we can't get it started, we just forget the whole thing. */
	linebuf_done(lb);
	free(lb);
    	return;
    }
    cell->data.lb = lb;

    for(x = xleft + 1; x < xleft + width; ++x) {
	cell = canv_allocate_cell(this, x, y, CCT_LINEBUF_CONT);
	if(cell != NULL) cell->data.lb = lb;
    }

}

void canv_put_glyph(canv_t * this, glyph_t glyph, sint32 x, sint32 y)
{
    canv_cell_t * cell;

    CANV_CHECK_X(this, x);
    CANV_CHECK_Y(this, y);

    cell = canv_allocate_cell(this, x, y, CCT_GLYPH);
    if(cell != NULL) cell->data.glyph = glyph;
}

void canv_put_hline(canv_t * this, sint32 xleft, sint32 y, sint32 xright)
{
    sint32 x;

    CANV_CHECK_X(this, xleft);
    CANV_CHECK_Y(this, y);
    CANV_CHECK_X(this, xright);
    assert(xleft <= xright);

    canv_linedraw_to_cell(this, xleft, y, CLD_RIGHT);
    for(x = xleft + 1; x < xright; ++x) {
    	canv_linedraw_to_cell(this, x, y, CLD_LEFT | CLD_RIGHT);
    }
    canv_linedraw_to_cell(this, xright, y, CLD_LEFT);
    /* If xleft == xright, we have the cell drawn with CLD_LEFT | CLD_RIGHT.
     * This is intentional.
     */
}

void canv_put_vline(canv_t * this, sint32 x, sint32 ybottom, sint32 ytop)
{
    sint32 y;

    CANV_CHECK_X(this, x);
    CANV_CHECK_Y(this, ybottom);
    CANV_CHECK_Y(this, ytop);
    assert(ybottom >= ytop);

    canv_linedraw_to_cell(this, x, ytop, CLD_DOWN);
    for(y = ytop + 1; y < ybottom; ++y) {
    	canv_linedraw_to_cell(this, x, y, CLD_UP | CLD_DOWN);
    }
    canv_linedraw_to_cell(this, x, ybottom, CLD_UP);
    /* If ytop == ybottom, we have the cell drawn with CLD_UP | CLD_DOWN.
     * This is intentional.
     */
}

void canv_put_black_box(
    canv_t * this,
    sint32 xleft,
    sint32 ybottom,
    sint32 xright,
    sint32 ytop
)
{
    sint32 x, y;

    CANV_CHECK_X(this, xleft);
    CANV_CHECK_Y(this, ybottom);
    CANV_CHECK_X(this, xright);
    CANV_CHECK_Y(this, ytop);
    assert(xleft <= xright);
    assert(ybottom >= ytop);

    for(y = ytop; y <= ybottom; ++y) {
    	for(x = xleft; x <=  xright; ++x) {
	    canv_allocate_cell(this, x, y, CCT_BLACK);
	    	/* If the cell is already black this call fails, but the
		 * cell is then black anyway.
		 */
	}
    }
}


static const glyph_t linedraw_glyphs[16] = {
    GLYPH_UNI_space,
    GLYPH_UNI_bdrawlightrt,
    GLYPH_UNI_bdrawlightup,
    GLYPH_UNI_bdrawlightuprt,
    GLYPH_UNI_bdrawlightlf,
    GLYPH_UNI_bdrawlighthorz,
    GLYPH_UNI_bdrawlightuplf,
    GLYPH_UNI_bdrawlightuphorz,
    GLYPH_UNI_bdrawlightdn,
    GLYPH_UNI_bdrawlightdnrt,
    GLYPH_UNI_bdrawlightvert,
    GLYPH_UNI_bdrawlightvertrt,
    GLYPH_UNI_bdrawlightdnlf,
    GLYPH_UNI_bdrawlightdnhorz,
    GLYPH_UNI_bdrawlightvertlf,
    GLYPH_UNI_bdrawlightverthorz
};

void canv_write(const canv_t * this, FILE * f)
{
    linebuf_t lb;
    canv_line_t * pl;
    canv_cell_t * pc;
    sint32 x, y;

    linebuf_init(&lb, this->ncols);
    	/* lb's initial size should be about right for most lines */
    for(y = 0; y < this->nlines; ++y) {
    	pl = this->lines + y;
	assert(pl->cols_used <= this->ncols);	/* consistency check */

    	linebuf_clear(&lb);
	pc = pl->cells;
	for(x = 0; x < pl->cols_used; ++x) {
	    switch(pc[x].type) {
		case CCT_UNUSED:
		    linebuf_putg(&lb, GLYPH_UNI_space);
		    /*linebuf_putg(&lb, GLYPH_asterisk);*/
		    break;
		case CCT_BLACK:
		    linebuf_putg(&lb, GLYPH_block);
		    break;
		case CCT_LINEDRAW:
		    linebuf_putg(&lb, linedraw_glyphs[pc[x].data.draw]);
		    break;
		case CCT_LINEBUF_CONT:
		    break;
		case CCT_GLYPH:
		    linebuf_putg(&lb, pc[x].data.glyph);
		    break;
		case CCT_LINEBUF_START:
		    linebuf_append(&lb, pc[x].data.lb);
		    break;
		default:
		    NOTREACHED;
	    }
	} /* for x */
	outenc_write(f, &lb);
	putc('\n', f);
    } /* for y */
    
    linebuf_done(&lb);
}

static canv_cell_t * canv_allocate_cell(
    canv_t * this,
    sint32 x,
    sint32 y,
    canv_celltype_t whatfor
)
{
    canv_cell_t * res;
    canv_line_t * l;

    res = canv_cell(this, x, y);
    if(res->type >= whatfor) {
    	pmesg(
	    80,
	    "cell at (%ld, %ld) already used for %d, cannot use for %d\n",
	    x,
	    y,
	    res->type,
	    whatfor
	);
	return NULL;
    }
    res->type = whatfor;

    l = this->lines + y;
    if(l->cols_used < x + 1) l->cols_used = x + 1;
    return res;
}

static canv_cell_t * canv_linedraw_to_cell(
    canv_t * this,
    sint32 x,
    sint32 y,
    canv_linedraw_t draw
)
{
    canv_cell_t * res;

    res = canv_cell(this, x, y);
    if(res->type == CCT_LINEDRAW) {
    	/* already in use for line drawing */
    	res->data.draw |= draw;
    }
    else {
    	/* try to claim for line drawing */
    	res = canv_allocate_cell(this, x, y, CCT_LINEDRAW);
	if(res != NULL) res->data.draw = draw;
    }
    return res;
}
