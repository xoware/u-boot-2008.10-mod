/*
 *  armboot - Startup Code for unknown CPU-core
 *
 *  Copyright (c) 2003  Texas Instruments
 *
 *  ----- Adapted for OMAP1610 OMAP730 from ARM925t code ------
 *
 *  Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 *  Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 *  Copyright (c) 2002	Gary Jennejohn <gj@denx.de>
 *  Copyright (c) 2003	Richard Woodruff <r-woodruff2@ti.com>
 *  Copyright (c) 2003	Kshitij <kshitij@ti.com>
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
/*
 *************************************************************************
 *
 * Jump vector table
 *
 *************************************************************************
 */

.globl _start
_start:
	b	start_code
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_not_used:
	.word not_used
_irq:
	.word irq
_fiq:
	.word fiq

	.balignl 16,0xdeadbeef

/*
 *************************************************************************
 *
 * Startup Code (called from the ARM reset exception vector)
 *
 * do important init only if we don't start from memory!
 * setup memory and board specific bits prior to relocation.
 * relocate armboot to ram
 * setup stack
 *
 *************************************************************************
 */

_TEXT_BASE:
	.word	TEXT_BASE /* address of _start in the linked image */

.globl _armboot_start
_armboot_start:
	.word _start

#ifdef CONFIG_CNS3000
.globl _armboot_mem_end
_armboot_mem_end:
	.word	0x08000000
#endif

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start
_bss_start:
	.word __bss_start

.globl _bss_end
_bss_end:
	.word _end

#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de
#endif


/*
 * the actual start code
 */
.globl start_code
start_code:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0,#0x1f
	orr	r0, r0,#0xd3
	msr	cpsr, r0

	ldr r0, =0x73000020
	mov r1, #1
	str r1, [r0]

	/*
	 * we do sys-critical inits only at reboot,
	 * not when booting from ram!
	 */
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************
 */
cpu_init_crit:
	/*
	 * To run direct from reset some importent
	 * initialisation code must be added here
	 */
	/* Ensure that the MMU and caches are off */
	/* ?? flush v4 I/D caches */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/* ?? disable MMU stuff and caches */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	mcr	p15, 0, r0, c1, c0, 0

	/* 
	 * Handle secondary mpcores
	 * CPU1 enter dormant mode
	 */

	/* Read core number into r0 */
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, r0, #0x0f

	cmp	r0, #0x00
	beq	primary_core
	bl	secondary_cores

primary_core:
	/* 
	 * Initialise memory 
	 * Setup memory and board specific bits prior to relocation.
	 */
#ifdef CNS3XXX_EARLY_DEBUG
	bl early_uart_init
#ifdef CNS3XXX_EARLY_DEBUG_GPIO
	bl early_gpioa_init
	ldr r2, =CNS3XXX_EARLY_DEBUG_GPIO_CPU
	bl output_led	
#endif
    ldr r2, =MSG_EARLY
    bl outputstring

    ldr r2, =MSG_LOWLEVEL
    bl outputstring
#endif
	bl	lowlevel_init
#ifdef CNS3XXX_EARLY_DEBUG_GPIO
	ldr r2, =CNS3XXX_EARLY_DEBUG_GPIO_DRAM
	bl output_led
#endif

#ifdef CNS3XXX_EARLY_DEBUG
    ldr r0, =CNS3000_VEGA_MISC_BASE
    ldr r1, [r0, #0x0]
    cmp r1, #0x1
	beq relocate	/* don't do simple test after re-mapped */

    ldr r2, =MSG_DRAM_SIMPLE_TEST
    bl outputstring
	ldr	r0, =0x55555555	/* test pattern */
	ldr r1, =0x20000	/* test size */
	bl mem_simple_test	

	cmp r0, r3
	beq simple_test_ok
    ldr r2, =MSG_FAIL
	b dram_test_result
simple_test_ok:
#ifdef CNS3XXX_EARLY_DEBUG_GPIO
	ldr r2, =CNS3XXX_EARLY_DEBUG_GPIO_DTEST
	bl output_led
#endif
    ldr r2, =MSG_PASS
dram_test_result:
    bl outputstring
#endif

#endif /* CONFIG_SKIP_LOWLEVEL_INIT */

#ifndef CONFIG_SKIP_RELOCATE_UBOOT
relocate:				/* relocate U-Boot to RAM	    */
#ifdef CNS3XXX_EARLY_DEBUG
    ldr r2, =MSG_RELOCATE
    bl outputstring
#endif
	adr	r0, _start		/* pc relative  address of label    */

#ifdef CONFIG_CNS3000
	ldr	r1, =0x20000000
#else
	ldr	r1, _TEXT_BASE		/* linked image address of label    */
	cmp	r0, r1                  /* test if we run from flash or RAM */
	beq	stack_setup             /* ifeq we are in the RAM copy      */
#endif

	ldr	r2, _armboot_start
	ldr	r3, _bss_start
	sub	r2, r3, r2		/* r2 <- size of armboot            */
	add	r2, r0, r2		/* r2 <- source end address         */

copy_loop:
	ldmia	r0!, {r3-r10}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r10}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end addreee [r2]    */
	ble	copy_loop

#if 0
#ifdef CONFIG_SPI_FLASH_BOOT
	ldr	r0, =CONFIG_KERNEL_OFFSET
	ldr	r1, =0x21000000		/* copy bootpImage from spi flash [r2] to [r1] */
	ldr	r2, =CONFIG_KERNEL_OFFSET /* the start address of bootpImage */
	ldr	r3, =CONFIG_KERNEL_END	/* the end address of bootpImage */
	sub	r2, r3, r2		/* r2 <- size of bootpImage */
	add	r2, r0, r2		/* r2 <- source end addr of bootpImage */

linux_copy_loop:
	ldmia	r0!, {r3-r10}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r10}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end addreee [r2]    */
	ble	linux_copy_loop
#endif /* CONFIG_SPI_FLASH_BOOT */
#endif

#ifdef CONFIG_CNS3000
#ifdef CNS3XXX_EARLY_DEBUG
    ldr r2, =MSG_REMAP
    bl outputstring
#endif
	/* Re-map */
	ldr	r0, =0x76000000
	mov	r1, #1
	str	r1, [r0]
#endif
#endif	/* CONFIG_SKIP_RELOCATE_UBOOT */

	/* Set up the stack						    */
stack_setup:
#ifdef CONFIG_CNS3000
	ldr	r0, _armboot_mem_end
#else
	ldr	r0, _TEXT_BASE		/* upper 128 KiB: relocated uboot   */
#endif
	sub	r0, r0, #CFG_MALLOC_LEN	/* malloc area                      */
	sub	r0, r0, #CFG_GBL_DATA_SIZE /* bdinfo                        */
#ifdef CONFIG_USE_IRQ
	sub	r0, r0, #(CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ)
#endif
	sub	sp, r0, #12		/* leave 3 words for abort-stack    */

clear_bss:
	ldr	r0, _bss_start		/* find start of bss segment        */
	ldr	r1, _bss_end		/* stop here                        */
	mov 	r2, #0x00000000		/* clear                            */

clbss_l:str	r2, [r0]		/* clear loop...                    */
	add	r0, r0, #4
	cmp	r0, r1
	ble	clbss_l

#ifdef CNS3XXX_EARLY_DEBUG
    ldr r2, =MSG_START_ARMBOOT
    bl outputstring

    ldr r0, =CNS3000_VEGA_PMU_BASE
    str r4, [r0, #0x1C]
#endif
	ldr	pc, _start_armboot

_start_armboot:
	.word start_armboot

#ifdef CNS3XXX_EARLY_DEBUG
early_uart_init:
    ldr r0, =CNS3000_VEGA_PMU_BASE
    ldr r4, [r0, #0x1C]
    ldr r1, =0x00003FF4 /* Enable 24 MHz UART clock */
    str r1, [r0, #0x1C]

    ldr r1, =0x0000004f
    str r1, [r0, #0x00] /* enable UART0 clock and reset it */

	ldr r1, [r0, #0x4]
	orr r1, r1, #(0x1<<6)
	str r1, [r0, #0x4]
	//ldr r1, =0x3000004f
	//str r1, [r0, #0x04] /* do UAT0 software rest*/
    ldr r0, =CNS3000_VEGA_UART0_BASE
    ldr r1, =0x00000080 /* Enter DLAB */
    str r1, [r0, #0x0C]
    ldr r1, =0x00000001 /* pre-scalar value => 24MHz */
    str r1, [r0, #0x08] /* PSR */
    ldr r1, =0x00000027 /* DLL */
    str r1, [r0, #0x00]
    ldr r1, =0x00000000 /* DLM */
    str r1, [r0, #0x04]
    ldr r1, =0x00000000 /* Exit DLAB */
    str r1, [r0, #0x0C]
    ldr r1, =0x00000003 /* 8N1 */
    str r1, [r0, #0x0C]
    ldr r1, =0x00000087
    str r1, [r0, #0x08]
    mov pc, lr

outputstring:
    ldr r0, =CNS3000_VEGA_UART0_BASE
load_character:
    ldrb r3, [r2], #1
    cmp r3, #0x0
    beq outputstring_done
print_character:
    ldr r1, [r0, #0x14]
    and r1, r1, #0x20
    cmp r1, #0x0
    beq print_character
    str r3, [r0, #0x00]
    b load_character
outputstring_done:
    mov pc, lr

mem_simple_test:	/* r0: test pattern, r1: test size */
    ldr r2, =CNS3000_VEGA_DDR2SDRAM_BASE
	add r2, r2, #0x1000000
	add	r1, r1, r2
2:
	cmp	r2, r1
	bhs	mem_simple_test_exit
	str r0, [r2]
	ldr r3, [r2]
	cmp r3, r0
	bne mem_simple_test_exit
	add r2, r2, #4 
	b	2b

mem_simple_test_exit:
    mov pc, lr
#ifdef CNS3XXX_EARLY_DEBUG_GPIO

early_gpioa_init:
	ldr r0, =CNS3000_VEGA_PMU_BASE
	ldr r1, [r0, #0x0]
	orr r1, r1, #(0x1<<10)
	str r1, [r0, #0x0]

	ldr r1, [r0, #0x4]
	orr r1, r1, #(0x1<<10)
	str r1, [r0, #0x4]

	ldr r0, =CNS3000_VEGA_GPIOA_BASE
	ldr r1, =CNS3XXX_EARLY_DEBUG_GPIO_A_MASK
	str r1, [r0, #0x00000008]
	ldr r1, =CNS3XXX_EARLY_DEBUG_GPIO_A_MASK
	str r1, [r0, #0x00000010]
    mov pc, lr
output_led:
	ldr r0, =CNS3000_VEGA_GPIOA_BASE
	str r2, [r0, #0x00000014]
    mov pc, lr

#endif
#endif

/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */

	.macro	bad_save_user_regs
	@ carve out a frame on current user stack
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}	@ Save user registers (now in svc mode) r0-r12

#ifdef CONFIG_CNS3000
	ldr	r2, _armboot_mem_end
#else
	ldr	r2, _armboot_start
#endif
	sub	r2, r2, #(CONFIG_STACKSIZE+CFG_MALLOC_LEN)
	sub	r2, r2, #(CFG_GBL_DATA_SIZE+8)  @ set base 2 words into abort stack
	@ get values for "aborted" pc and cpsr (into parm regs)
	ldmia	r2, {r2 - r3}
	add	r0, sp, #S_FRAME_SIZE		@ grab pointer to old stack
	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}	@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp		@ save current stack into r0 (param register)
	.endm

	.macro	irq_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	@ !!!! R8 NEEDS to be saved !!!! a reserved stack spot would be good.
	add	r8, sp, #S_PC
	stmdb	r8, {sp, lr}^		@ Calling SP, LR
	str	lr, [r8, #0]		@ Save calling PC
	mrs	r6, spsr
	str	r6, [r8, #4]		@ Save CPSR
	str	r0, [r8, #8]		@ Save OLD_R0
	mov	r0, sp
	.endm

	.macro	irq_restore_user_regs
	ldmia	sp, {r0 - lr}^			@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]			@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4		@ return & move spsr_svc into cpsr
	.endm

	.macro get_bad_stack
#ifdef CONFIG_CNS3000
	ldr	r13, _armboot_mem_end
#else
	ldr	r13, _armboot_start		@ setup our mode stack
#endif
	sub	r13, r13, #(CONFIG_STACKSIZE+CFG_MALLOC_LEN)
	sub	r13, r13, #(CFG_GBL_DATA_SIZE+8) @ reserved a couple spots in abort stack

	str	lr, [r13]	@ save caller lr in position 0 of saved stack
	mrs	lr, spsr	@ get the spsr
	str	lr, [r13, #4]	@ save spsr in position 1 of saved stack
	mov	r13, #MODE_SVC	@ prepare SVC-Mode
	@ msr	spsr_c, r13
	msr	spsr, r13	@ switch modes, make sure moves will execute
	mov	lr, pc		@ capture return pc
	movs	pc, lr		@ jump to next instruction & switch modes.
	.endm

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp, IRQ_STACK_START
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, FIQ_STACK_START
	.endm

/*
 * exception handlers
 */
	.align  5
.globl undefined_instruction
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl	do_undefined_instruction

	.align	5
.globl software_interrupt
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl	do_software_interrupt

	.align	5
.globl prefetch_abort
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_prefetch_abort

	.align	5
.globl data_abort
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_data_abort

	.align	5
.globl not_used
not_used:
	get_bad_stack
	bad_save_user_regs
	bl	do_not_used

#ifdef CONFIG_USE_IRQ
	.align	5
.globl irq
irq:
	get_irq_stack
	irq_save_user_regs
	bl 	do_irq
	irq_restore_user_regs

	.align	5
.globl fiq
fiq:
	get_fiq_stack
	/* someone ought to write a more effiction fiq_save_user_regs */
	irq_save_user_regs
	bl 	do_fiq
	irq_restore_user_regs

#else

	.align	5
.globl irq
irq:
	get_bad_stack
	bad_save_user_regs
	bl	do_irq

	.align	5
.globl fiq
fiq:
	get_bad_stack
	bad_save_user_regs
	bl	do_fiq

#endif

	.align  5
.globl reset_cpu
reset_cpu:
        /* Global Software Reset */
	ldr	r0, =0x77000004
	mov	r1, #0
	str	r1, [r0]

_loop_forever:
        b       _loop_forever

#ifdef CNS3XXX_EARLY_DEBUG
MSG_EARLY:
	.string "\r\nCNS3XXX Early Debug\r\n"
MSG_LOWLEVEL:
	.string "Low Level Init\r\n"
MSG_RELOCATE:
	.string "Relocate U-boot to DRAM\r\n"
MSG_REMAP:
	.string "Re-map\r\n"
MSG_DRAM_SIMPLE_TEST:
	.string "DRAM simple test..."
MSG_PASS:
	.string "pass\r\n"
MSG_FAIL:
	.string "fail\r\n"
MSG_START_ARMBOOT:
	.string "Start ARMBOOT......"
#endif
