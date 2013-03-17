/*
 * Board specific setup info
 *
 * (C) Copyright 2003, ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
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

#include "cns3xxx_dmc.h"

/* Set up the platform, once the cpu has been initialized */
.globl lowlevel_init
lowlevel_init:
	mov	r12, lr

	ldr r0, =CNS3000_VEGA_MISC_BASE
	ldr	r1, [r0, #0x0]
	cmp r1, #0x1
	beq sdram_end
	

	/* regulator control */
    ldr r0, =CNS3000_VEGA_PMU_BASE
    ldr r1, =0x00469A59
    str r1, [r0, #0x20]

	/* ?? Disable Watchdog */

	/* ?? External interrupt pending clear */

	/* ?? Disable all interrupts */

	/* ?? init system clock */

	/* Initiallise DDR2 SDRAM Control -- Setup pll, mux, memory */
	bl	ddr2_sdram_ctrl_init
sdram_end:

	/* Initiallise Static Memory Controller (SMC: NOR flash) */
	bl	smc_ctrl_init

#if defined(CONFIG_SPI) && defined(CONFIG_CNS3000)
	/* Initiallise SPI Control */
	bl	spi_ctrl_init
#endif

#if defined(CONFIG_LCD) && defined(CONFIG_CNS3000)
	/* Color LCD Control */
	bl	clcd_ctrl_init
#endif
	mov	lr, r12
	mov	pc, lr


ddr2_sdram_ctrl_init:
	/* SDRAM control register base address 0x72000000 */
	ldr	r0, =CNS3000_VEGA_DMC_BASE

	/* DLL initialization */
	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_CTRL_OFFSET]	/* 0x400*/

	ldr r1, =0x00001410
	str r1, [r0, #DMC_PHY_DETECT_CFG_OFFSET] /* 0x408 */

	ldr r1, =0x00000006
	str r1, [r0, #DMC_PHY_GATE_CTRL_OFFSET] /* 0x40C */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_RD_DATA_SLICE_0_CTRL_OFFSET]	/* 0x410 */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_RD_DATA_SLICE_1_CTRL_OFFSET]	/* 0x414 */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_RD_DATA_SLICE_2_CTRL_OFFSET]	/* 0x418 */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_RD_DATA_SLICE_3_CTRL_OFFSET]	/* 0x41C */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_CLK_DLY_CTRL_OFFSET]	/* 0x420 */

	ldr r1, =0x00000000
	str r1, [r0, #DMC_PHY_DLL_LOCK_VALUE_OFFSET] /* 0x424 */

	ldr r1, =0x00000001
	str r1, [r0, #DMC_PHY_DETECT_CTRL_OFFSET]	/* 0x404 */

	ldr r1, =0x00000003
	str r1, [r0, #DMC_PHY_DETECT_CTRL_OFFSET]	/* 0x404 */

	/* ZQ initialization */
	/* Was 0x0006E550 changed to 0x0006A550	   */
	/* Driving Strength [15:13] =3'b101        */
	/* ODT Termination [11:9] = 3'b010         */
	/* Note total 6 instructions were changed  */
	ldr r1, =0x0006A550
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */

	ldr r1, =0x0006A551
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */

calibration_loop:	/* wait for calibration completed */
	ldr	r1, [r0, #DMC_PHY_ZQ_STATUS_OFFSET]	/* 0x42C */
	and	r1, r1, #0x1
	cmp r1, #0x1
	bne calibration_loop

	ldr r1, =0x0006A550
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */

	ldr r1, =0x0006A552
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */
	
	ldr r1, =0x0006A550
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */

	ldr r1, =0x0007A550
	str r1, [r0, #DMC_PHY_ZQ_CTRL_OFFSET]	/* 0x428 */

	ldr r1, =0x00000002
	str r1, [r0, #DMC_READ_DELAY_OFFSET]	/* 0x430 */

	/* Set CAS Latency */
	ldr	r1, =CNS3XXX_DMC_CAS_LATENCY<<1
	str r1, [r0, #DMC_CAS_LATENCY_OFFSET]   /* 0x014 */

	/* Set t_mrd, mode register command time */
	ldr	r1, =CNS3XXX_DMC_T_MRD
	str r1, [r0, #DMC_T_MRD_OFFSET]     /* 0x01C */

	/* SCH_RCD, SCH_RFC, SCH_RP, and SCH_FAW are related to ACLK 
		we just check the special case of CPU frequency (i.e. 700MHz) */
	ldr	r5, =CNS3000_VEGA_MISC_BASE
	ldr	r6, [r5, #0x04]
	and	r6, r6, #0x1E0
	ldr	r5, =0x180
	cmp r5, r6
	beq	aclk_350
	
	/* Set sch_rcd */
	ldr	r1, =CNS3XXX_DMC_SCH_RCD_A300<<8
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set sch_rfc */
	ldr	r1, =CNS3XXX_DMC_SCH_RFC_A300<<8
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set sch_rp */
	ldr	r1, =CNS3XXX_DMC_SCH_RP_A300<<8
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set sch_faw */
	ldr	r1, =CNS3XXX_DMC_SCH_FAW_A300<<8
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */
	
	b aclk_end
aclk_350:
	/* Set sch_rcd */
	ldr	r1, =CNS3XXX_DMC_SCH_RCD_A350<<8
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set sch_rfc */
	ldr	r1, =CNS3XXX_DMC_SCH_RFC_A350<<8
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set sch_rp */
	ldr	r1, =CNS3XXX_DMC_SCH_RP_A350<<8
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set sch_faw */
	ldr	r1, =CNS3XXX_DMC_SCH_FAW_A350<<8
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */

aclk_end:
	ldr	r5, =CNS3000_VEGA_MISC_BASE
	ldr	r6, [r5, #0x04]
	and	r6, r6, #0x600
	ldr	r5, =0x0	
	cmp r5, r6
	beq	ddr_200	
	ldr	r5, =0x200	
	cmp r5, r6
	beq	ddr_266	
	ldr	r5, =0x400	
	cmp r5, r6
	beq	ddr_333	
	ldr	r5, =0x600	
	cmp r5, r6
	beq	ddr_400	

ddr_200:
	/* Set t_ras */
	ldr	r1, =CNS3XXX_DMC_T_RAS_200
	str r1, [r0, #DMC_T_RAS_OFFSET]     /* 0x020 */

	/* Set t_rc */
	ldr	r1, =CNS3XXX_DMC_T_RC_200
	str r1, [r0, #DMC_T_RC_OFFSET]      /* 0x024 */

	/* Set t_rcd */
	ldr	r2, [r0, #DMC_T_RCD_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RCD_200
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set t_rfc */
	ldr	r2, [r0, #DMC_T_RFC_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RFC_200
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set t_rp */
	ldr	r2, [r0, #DMC_T_RP_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RP_200
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set t_rrd */
	ldr	r1, =CNS3XXX_DMC_T_RRD_200
	str r1, [r0, #DMC_T_RRD_OFFSET]     /* 0x034 */

	/* Set t_wr */
	ldr	r1, =CNS3XXX_DMC_T_WR_200
	str r1, [r0, #DMC_T_WR_OFFSET]      /* 0x038 */

	/* Set t_wtr */
	ldr	r1, =CNS3XXX_DMC_T_WTR_200
	str r1, [r0, #DMC_T_WTR_OFFSET]     /* 0x03C */

	/* Set t_faw */
	ldr	r2, [r0, #DMC_T_FAW_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_FAW_200
	orr r1, r1, r2
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */
	
	/* Set refresh_prd */
	ldr r1, =CNS3XXX_DMC_REFRESH_PRD_266
	str r1, [r0, #DMC_REFRESH_PRD_OFFSET]	/* 0x010 */

	b ddr_clock_end

ddr_266:
	/* Set t_ras */
	ldr	r1, =CNS3XXX_DMC_T_RAS_266
	str r1, [r0, #DMC_T_RAS_OFFSET]     /* 0x020 */

	/* Set t_rc */
	ldr	r1, =CNS3XXX_DMC_T_RC_266
	str r1, [r0, #DMC_T_RC_OFFSET]      /* 0x024 */

	/* Set t_rcd */
	ldr	r2, [r0, #DMC_T_RCD_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RCD_266
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set t_rfc */
	ldr	r2, [r0, #DMC_T_RFC_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RFC_266
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set t_rp */
	ldr	r2, [r0, #DMC_T_RP_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RP_266
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set t_rrd */
	ldr	r1, =CNS3XXX_DMC_T_RRD_266
	str r1, [r0, #DMC_T_RRD_OFFSET]     /* 0x034 */

	/* Set t_wr */
	ldr	r1, =CNS3XXX_DMC_T_WR_266
	str r1, [r0, #DMC_T_WR_OFFSET]      /* 0x038 */

	/* Set t_wtr */
	ldr	r1, =CNS3XXX_DMC_T_WTR_266
	str r1, [r0, #DMC_T_WTR_OFFSET]		/* 0x03C */

	/* Set t_faw */
	ldr	r2, [r0, #DMC_T_FAW_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_FAW_266
	orr r1, r1, r2
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */

	/* Set refresh_prd */
	ldr r1, =CNS3XXX_DMC_REFRESH_PRD_266
	str r1, [r0, #DMC_REFRESH_PRD_OFFSET]	/* 0x010 */

	b ddr_clock_end

ddr_333:
	/* Set t_ras */
	ldr	r1, =CNS3XXX_DMC_T_RAS_333
	str r1, [r0, #DMC_T_RAS_OFFSET]     /* 0x020 */

	/* Set t_rc */
	ldr	r1, =CNS3XXX_DMC_T_RC_333
	str r1, [r0, #DMC_T_RC_OFFSET]      /* 0x024 */

	/* Set t_rcd */
	ldr	r2, [r0, #DMC_T_RCD_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RCD_333
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set t_rfc */
	ldr	r2, [r0, #DMC_T_RFC_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RFC_333
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set t_rp */
	ldr	r2, [r0, #DMC_T_RP_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RP_333
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set t_rrd */
	ldr	r1, =CNS3XXX_DMC_T_RRD_333
	str r1, [r0, #DMC_T_RRD_OFFSET]     /* 0x034 */

	/* Set t_wr */
	ldr	r1, =CNS3XXX_DMC_T_WR_333
	str r1, [r0, #DMC_T_WR_OFFSET]      /* 0x038 */

	/* Set t_wtr */
	ldr	r1, =CNS3XXX_DMC_T_WTR_333
	str r1, [r0, #DMC_T_WTR_OFFSET]		/* 0x03C */

	/* Set t_faw */
	ldr	r2, [r0, #DMC_T_FAW_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_FAW_333
	orr r1, r1, r2
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */
	
	/* Set refresh_prd */
	ldr r1, =CNS3XXX_DMC_REFRESH_PRD_333
	str r1, [r0, #DMC_REFRESH_PRD_OFFSET]	/* 0x010 */

	b ddr_clock_end

ddr_400:
	/* Set t_ras */
	ldr	r1, =CNS3XXX_DMC_T_RAS_400
	str r1, [r0, #DMC_T_RAS_OFFSET]     /* 0x020 */

	/* Set t_rc */
	ldr	r1, =CNS3XXX_DMC_T_RC_400
	str r1, [r0, #DMC_T_RC_OFFSET]      /* 0x024 */

	/* Set t_rcd */
	ldr	r2, [r0, #DMC_T_RCD_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RCD_400
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RCD_OFFSET]     /* 0x028 */

	/* Set t_rfc */
	ldr	r2, [r0, #DMC_T_RFC_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RFC_400
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RFC_OFFSET]     /* 0x02C */

	/* Set t_rp */
	ldr	r2, [r0, #DMC_T_RP_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_RP_400
	orr r1, r1, r2
	str r1, [r0, #DMC_T_RP_OFFSET]      /* 0x030 */

	/* Set t_rrd */
	ldr	r1, =CNS3XXX_DMC_T_RRD_400
	str r1, [r0, #DMC_T_RRD_OFFSET]     /* 0x034 */

	/* Set t_wr */
	ldr	r1, =CNS3XXX_DMC_T_WR_400
	str r1, [r0, #DMC_T_WR_OFFSET]      /* 0x038 */

	/* Set t_wtr */
	ldr	r1, =CNS3XXX_DMC_T_WTR_400
	str r1, [r0, #DMC_T_WTR_OFFSET]     /* 0x03C */

	/* Set t_faw */
	ldr	r2, [r0, #DMC_T_FAW_OFFSET]
	ldr r1, =CNS3XXX_DMC_T_FAW_400
	orr r1, r1, r2
	str r1, [r0, #DMC_T_FAW_OFFSET]		/* 0x054 */

	/* Set refresh_prd */
	ldr r1, =CNS3XXX_DMC_REFRESH_PRD_400
	str r1, [r0, #DMC_REFRESH_PRD_OFFSET]	/* 0x010 */

ddr_clock_end:
	/* Set t_xp */
	ldr	r1, =CNS3XXX_DMC_T_XP
	str r1, [r0, #DMC_T_XP_OFFSET]      /* 0x040 */

	/* Set t_xsr */
	ldr	r1, =CNS3XXX_DMC_T_XSR
	str r1, [r0, #DMC_T_XSR_OFFSET]     /* 0x044 */

	/* Set t_esr */
	ldr	r1, =CNS3XXX_DMC_T_ESR
	str r1, [r0, #DMC_T_ESR_OFFSET]     /* 0x048 */

	ldr r1, =0x00014000 |DMC_ROW_BITS<<3 |DMC_COLUMN_BITS
	str r1, [r0, #DMC_MEMORY_CFG_OFFSET]	/* 0x00C*/

	ldr	r1, =DMC_MEMORY_WIDTH<<6|DMC_BANK_BITS<<4|0x4
	str r1, [r0, #DMC_MEMORY_CFG2_OFFSET]   /* 0x04C */

	ldr r1, =0x000000FF
	str r1, [r0, #DMC_CHIP_0_CFG_OFFSET]	/* 0x200 */

	/* NOP */
	ldr r1, =0x000C0000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* precharge all */
	ldr r1, =0x00000000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* extended mode register 2 */
	ldr r1, =0x000A0000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* extended mode register 3 */
	ldr r1, =0x000B0000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* [1]= 0 Full Strength					*/
    /* [6]= 0 and [2]= 1, 75 ohm			*/
	/* The memory command in the form of 0x0009xxxx */
    /* is the command to extended mode register		*/

	/* extended mode register - DLL Enable / AL = 0 */
	ldr r1, =0x00090004
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

#if (CNS3XXX_DMC_CAS_LATENCY == 0x5)
	ldr r1, =0x00080952
#else
	/* mode register - BL=4/DLL reset/CASL=6/WR=6 */
	ldr r1, =0x00080B62
#endif
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* precharge all */
	ldr r1, =0x00000000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */
	
	/* auto-refresh */
	ldr r1, =0x00040000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	/* auto-refresh */
	ldr r1, =0x00040000
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

#if (CNS3XXX_DMC_CAS_LATENCY == 0x5)
	ldr r1, =0x00080852
#else
	/* mode register - BL=4 / disable DLL reset/ CASL=6 / WR=6 */
	ldr r1, =0x00080A62
#endif
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

#if 0
	ldr r1, =0x000903E4
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */

	ldr r1, =0x00090064
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */
#else
	/* extended mode register - OCD default/ ODT = 50ohm */
	/* Change 0x000903C4 -> 0x00090384	75 ohm */
	/* ( Bit 6, Bit 2 ) 
	 * (0, 1: 75 ohm ) 
	 * (1, 1: 50 ohm )
	 * (1, 0: 150 ohm)
	 */  
	ldr r1, =0x00090384
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */
	
	/* extended mode register - OCD exit/ ODT = 50ohm */
	/* Change 0x00090044 to 0x00090004 Clear the bit [7:9] */
	ldr r1, =0x00090004
	str r1, [r0, #DMC_DIRECT_CMD_OFFSET]	/* 0x008 */
#endif

	/* change controller state to READY */
	ldr r1, =0x00000000
	str r1, [r0, #0x004]

	/* Set request memory for LCD high priority */
	ldr	r1, =0x00000400
	str r1, [r0, #DMC_USER_CONFIG0_OFFSET]   /* 0x304 */

	mov	pc, lr

smc_ctrl_init:
	/* SMC control register base address 0x73000000 */
	ldr	r0, =CNS3000_VEGA_SMC_BASE

	/* set cycles for CS0 NOR FLASH */
	
#if 0
	ldr r1, [r0, #SMC_SRAM_CYCLES0_0_OFFSET]
#else
	ldr	r1, =0x0002B3CC
#endif
	str	r1, [r0, #SMC_SET_CYCLES_OFFSET]	/* 0x014 */

	/* set opmode bit, 16-bits width */
	ldr	r1, =0x00000001
	str	r1, [r0, #SMC_SET_OPMODE_OFFSET]	/* 0x018*/

	/* an idle cycle occurs after each burst */
	ldr	r1, =0x00000001
	str	r1, [r0, #SMC_REFRESH_PERIOD_0_OFFSET]	/* 0x020*/

	/* select CS0 */
	ldr	r1, =0x00400000
	str	r1, [r0, #SMC_DIRECT_CMD_OFFSET]	/* 0x010 */

	mov	pc, lr

#if defined(CONFIG_SPI) && defined(CONFIG_CNS3000)
spi_ctrl_init:
	/* SPI control register base address 0x71000000 */
	ldr	r0, =CNS3000_VEGA_SSP_BASE

	/* enable SPI high speed read for system boot up */
        ldr     r1, [r0, #SPI_CFG_OFFSET]
        mov     r2, #0x40000000
        orr     r1, r1, r2
        str     r1, [r0, #SPI_CFG_OFFSET]

        /* set bit rate */
        mov     r1, #0x01
        str     r1, [r0, #SPI_BIT_RATE_OFFSET]

	mov	pc, lr
#endif

#if defined(CONFIG_LCD) && defined(CONFIG_CNS3000)
clcd_ctrl_init:
	/* Color LCD control register base address 0x80000000 */
	ldr	r0, =CNS3000_VEGA_CLCD_BASE

	/* */
	ldr	r1, =0x00000000
	str	r1, [r0, #CLCD_LCD_CONTROL_OFFSET]

	/* */
	ldr	r1, =0x00000002
	str	r1, [r0, #CLCD_LCD_IMSC_OFFSET]

	/* */
	ldr	r1, =0x01012874
	str	r1, [r0, #CLCD_LCD_TIMING0_OFFSET]

	/* */
	ldr	r1, =0x0202250F
	str	r1, [r0, #CLCD_LCD_TIMING1_OFFSET]

	/* */
	ldr	r1, =0x05DF1800
	str	r1, [r0, #CLCD_LCD_TIMING2_OFFSET]

	/* */
	ldr	r1, =0x00000000
	str	r1, [r0, #CLCD_LCD_TIMING3_OFFSET]

	/* */
	ldr	r1, =CNS3000_LCD_FB_SRAM0_BASE		/* 0x08000000 */
	str	r1, [r0, #CLCD_LCD_UPBASE_OFFSET]

	/* */
	ldr	r1, =CNS3000_LCD_FB_SRAM0_BASE		/* 0x08000000 */
	str	r1, [r0, #CLCD_LCD_UPBASE_OFFSET]

	/* */
	ldr	r1, =0x00000000
	str	r1, [r0, #CLCD_LCD_IMSC_OFFSET]

	/* */
	ldr	r1, =0x0000006D
	str	r1, [r0, #CLCD_LCD_CONTROL_OFFSET]

	/* */
	ldr	r1, =0x0000086D
	str	r1, [r0, #CLCD_LCD_CONTROL_OFFSET]

	mov	pc, lr
#endif
