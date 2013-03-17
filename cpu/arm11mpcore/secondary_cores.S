/*
 * (C) Copyright 2011, Cavium Networks
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


#include <config.h>
#include <version.h>

#include <cns3000.h>

#ifdef CONFIG_CNS3000
	//#define CNS3XXX_EARLY_DEBUG
	#ifdef CNS3XXX_EARLY_DEBUG
		#define CNS3XXX_EARLY_DEBUG_GPIO
		#ifdef CNS3XXX_EARLY_DEBUG_GPIO
			#define CNS3XXX_EARLY_DEBUG_GPIO_A_MASK 0x000000E0 /* GPIOA 5,6, and 7*/
			#define CNS3XXX_EARLY_DEBUG_GPIO_CPU	(0x1<<5) 
			#define CNS3XXX_EARLY_DEBUG_GPIO_DRAM	(0x1<<6) 
			#define CNS3XXX_EARLY_DEBUG_GPIO_DTEST	(0x1<<7) 
		#endif
	#endif
#endif

.globl secondary_cores
secondary_cores:
	mov	r12, lr

	/* Have we already remapped. */
	ldr	r8, =CNS3000_VEGA_MISC_BASE
	ldr	r6, [r8, #0x00]
	and	r6, r6, #0x01	

	tst	r6, #0x01
	beq	2f
	mov	pc, #0x10000000
2:
	/* Enable software interrupt */
	ldr	r5, =CNS3000_TC11MP_GIC_CPU_BASE
	mov	r6, #0x01
	str	r6, [r5, #GIC_CPU_CTRL_OFFSET]
	mov	r6, #0xF0
	str	r6, [r5, #GIC_CPU_PRIMASK_OFFSET]
1:
	/* Set WFI */
	mov r2, #0
	mcr	p15, 0, r2, c7, c0, 4

	ldr	r5, =CNS3000_VEGA_MISC_BASE
	ldr	r6, [r5, #0x0600]
	ldr	r5, =0x35678855
	cmp	r6, r5
	movne	pc, r6

	b	1b

	mov	lr, r12
	mov	pc, lr


