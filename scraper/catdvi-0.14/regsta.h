/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>

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

#ifndef REGSTA_H
#define REGSTA_H

#include "bytesex.h"

/* Handle the registers and the stack. */

#define REGISTER_H 0
#define REGISTER_V 1
#define REGISTER_W 2
#define REGISTER_X 3
#define REGISTER_Y 4
#define REGISTER_Z 5

/* Set registers to zero and initialize an empty stack with the given
   maximum depth. */
void init_regs_stack(int depth);

/* Get the value of one of the registers (h, v, w, x, y, z). */

sint32 get_reg(int reg);

/* Set the value of one of the registers (h, v, w, x, y, z). */
void set_reg(int reg, sint32);

/* Add to the value of one of the registers. */
void add_reg(int reg, sint32 a);

/* Push the registers on the stack. */
void push_regs(void);

/* Pop the registers off the stack. */
void pop_regs(void);

/* For debug purposes, dump registers to the standard error stream, if
   level is right. */
void dump_regs(int level);

#endif /* REGSTA_H */
