/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * CPU specific code for a core tile with an unknown cpu
 * - hence fairly empty......
 */

#include <common.h>
#include <command.h>

static void cache_flush (void);

int cpu_init (void)
{
	/*
	 * setup up stacks if necessary
	 */
#ifdef CONFIG_USE_IRQ
	DECLARE_GLOBAL_DATA_PTR;

	IRQ_STACK_START = _armboot_start - CFG_MALLOC_LEN - CFG_GBL_DATA_SIZE - 4;
	FIQ_STACK_START = IRQ_STACK_START - CONFIG_STACKSIZE_IRQ;
#endif
	return 0;
}

int cleanup_before_linux (void)
{
	/*
	 * this function is called just before we call linux
	 * it prepares the processor for linux
	 *
	 * we turn off caches etc ...
	 */

	disable_interrupts ();

	/* Since the CT has unknown processor we do not support
	 * cache operations
	 */

	/* turn off I/D-cache */
	icache_disable();
	dcache_disable();
	cache_flush();

	return (0);
}

int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	extern void reset_cpu (ulong addr);

	disable_interrupts ();
	reset_cpu (0);
	/*NOTREACHED*/
	return (0);
}

/* read coprocessor 15, register #1 (control register) */
static unsigned long read_p15_c1 (void)
{
	unsigned long value;

	__asm__ __volatile__(
		"mrc	p15, 0, %0, c1, c0, 0   @ read control reg\n"
		: "=r" (value)
		:
		: "memory");
	return value;
}

/* write to coprocessor 15, register #1 (control register) */
static void write_p15_c1 (unsigned long value)
{
	__asm__ __volatile__(
		"mcr	p15, 0, %0, c1, c0, 0   @ write it back\n"
		:
		: "r" (value)
		: "memory");

	read_p15_c1();
}

static void cp_delay (void)
{
	volatile int i;

	/* delay at least 2 nops  */
	for (i = 0; i < 100; i++)
		__asm__ __volatile__("nop\n");
}

/* See also ARM Ref. Man. */
#define C1_MMU			(0x1 << 0)	/* mmu off/on */
#define C1_ALIGN		(0x1 << 1)	/* alignment faults off/on */
#define C1_DC			(0x1 << 2)	/* dcache off/on */
#define C1_WB			(0x1 << 3)	/* merging write buffer on/off */
#define C1_BIG_ENDIAN	(0x1 << 7)	/* big endian off/on */
#define C1_SYS_PROT		(0x1 << 8)	/* system protection */
#define C1_ROM_PROT		(0x1 << 9)	/* ROM protection */
#define C1_IC			(0x1 << 12)	/* icache off/on */
#define C1_HIGH_VECTORS	(0x1 << 13)	/* location of vectors: low/high */
#define RESERVED_1		(0xf << 3)	/* must be 111b for R/W */

void icache_enable (void)
{
	ulong reg;

	reg = read_p15_c1 ();	/* get control reg. */
	cp_delay ();
	write_p15_c1 (reg | C1_IC);
}

void icache_disable (void)
{
	ulong reg;

	reg = read_p15_c1 ();
	cp_delay ();
	write_p15_c1 (reg & ~C1_IC);
}

/* return "disabled" */
int icache_status (void)
{
	return (read_p15_c1 () & C1_IC) != 0;
}

void dcache_enable (void)
{
	ulong reg;

	reg = read_p15_c1 ();
	cp_delay ();
	write_p15_c1 (reg | C1_DC);
}

void dcache_disable (void)
{
	ulong reg;

	reg = read_p15_c1 ();
	cp_delay ();
	write_p15_c1 (reg & ~C1_DC);
}

/* return "disabled", always off */
int dcache_status (void)
{
	return (read_p15_c1 () & C1_DC) != 0;
}

/* flush I/D-cache */
static void cache_flush (void)
{
	/* invalidate both caches and flush btb */
	asm ("mcr p15, 0, %0, c7, c7, 0": :"r" (0));
	/* mem barrier to sync things */
	asm ("mcr p15, 0, %0, c7, c10, 4": :"r" (0));
}

