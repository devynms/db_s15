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

#include <assert.h>
#include <stdlib.h>
#include "regsta.h"
#include "util.h"

#define NUM_REGS ((REGISTER_Z) + 1)

static size_t SP = 0; /* top of stack (index of first available slot) */
static size_t sdepth = 0; /* stack depth */
static sint32 * stack = 0;
static sint32 registers[NUM_REGS] = {0};
char const * regnames[NUM_REGS] = { "H", "V", "W", "X", "Y", "Z" };

void init_regs_stack(int depth)
{
        int i;
        assert(depth > 0);
        
        if (stack != 0) free(stack);

        for (i = 0; i < NUM_REGS; i++) registers[i] = 0;

        stack = malloc(depth * NUM_REGS * sizeof(sint32));
        if (stack == 0) panic("Out of memory!");
        SP = 0;
        sdepth = depth;
}

void push_regs(void)
{
        int i;
        assert(SP <= sdepth);
        if (SP == sdepth) panic("Stack overflow!");
        for (i = 0; i < NUM_REGS; i++) {
                stack[NUM_REGS * SP + i] = registers[i];
        }
        ++SP;
        dump_regs(90);
}

void pop_regs(void)
{
        int i;
        assert(SP <= sdepth);
        if (SP == 0) panic("Stack underflow!");
        --SP;
        for (i = 0; i < NUM_REGS; i++) {
                registers[i] = stack[NUM_REGS * SP + i];
        }
        dump_regs(90);
}

sint32 get_reg(int reg)
{
        assert(reg >= 0);
        assert(reg < NUM_REGS);
        pmesg(150, "GET_REG %s -> %li\n", regnames[reg], registers[reg]);
        return registers[reg];
}

void set_reg(int reg, sint32 a)
{
        assert(reg >= 0);
        assert(reg < NUM_REGS);
        pmesg(95, "SET_REG %s <- %li\n", regnames[reg], a);
        registers[reg] = a;
        dump_regs(90);
}

void add_reg(int reg, sint32 a)
{
        assert(reg >= 0);
        assert(reg < NUM_REGS);
        pmesg(95, "REGISTER %s <- %li + %li = ",
              regnames[reg], registers[reg], a);
        registers[reg] += a;
        pmesg(95, "%li\n", registers[reg]);
        dump_regs(90);
}

void dump_regs(int level)
{
        int i;

    	if(level > msglevel) return;
        pmesg(level, "REGISTER DUMP: ");
        for (i = 0; i < NUM_REGS; i++) {
                pmesg(level, "(%s %01li) ", regnames[i], registers[i]);
        }
        pmesg(level, "\n");
}
