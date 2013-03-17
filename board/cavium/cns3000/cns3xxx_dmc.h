/*******************************************************************************
 *
 *   Copyright (c) 2009 Cavium Networks 
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
1*   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *   more details.
 *
 *   You should have received a copy of the GNU General Public License along with
 *   this program; if not, write to the Free Software Foundation, Inc., 59
 *   Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   The full GNU General Public License is included in this distribution in the
 *   file called LICENSE.
 *
 ********************************************************************************/

#ifndef CNS3XXX_DMC_H
#define CNS3XXX_DMC_H


#define CNS3XXX_DMC_CAS_LATENCY		0x6
//#define CNS3XXX_DMC_CAS_LATENCY		0x5
#define CNS3XXX_DMC_T_MRD			0x2
#define CNS3XXX_DMC_T_XP			0x3
#define CNS3XXX_DMC_T_XSR			0xC8
#define CNS3XXX_DMC_T_ESR			0xC8

/* 
 * DMC parameter for DDR2 clock frequency 200 MHz, 
 * MCLK = 200 MHz, clock cycle 5ns
 */
#define CNS3XXX_DMC_T_RAS_200			0x09	/* 45 ns = 9 * MCLK*/
#define CNS3XXX_DMC_T_RC_200			0x0C	/* 60 ns = 12 * MCLK*/
#define CNS3XXX_DMC_T_RCD_200			0x03	/* 15 ns = 3 * MCLK*/
#define CNS3XXX_DMC_T_RFC_200			0x1A	/* 127.5 ns = 25.5 * MCLK*/
#define CNS3XXX_DMC_T_RP_200			0x03	/* 15 ns = 3 * MCLK*/
#define CNS3XXX_DMC_T_RRD_200			0x02	/* 10 ns = 2 * MCLK*/
#define CNS3XXX_DMC_T_WR_200			0x03	/* 15 ns = 3 * MCLK*/
#define CNS3XXX_DMC_T_WTR_200			0x02	/* 7.5 ns = 1.5 * MCLK*/
#define CNS3XXX_DMC_T_FAW_200			0x09	/* 45 ns = 9 * MCLK */
#define CNS3XXX_DMC_REFRESH_PRD_200		0x618	/* 7800 ns = 1560 * MCLK */

/* 
 * DMC parameter for DDR2 clock frequency 266 MHz, 
 * MCLK = 266 MHz, clock cycle 3.75939ns
 */
#define CNS3XXX_DMC_T_RAS_266			0x0C	/* 45 ns = 11.97 * MCLK  */
#define CNS3XXX_DMC_T_RC_266			0x10	/* 60 ns = 15.96 * MCLK */
#define CNS3XXX_DMC_T_RCD_266			0x04	/* 15 ns = 3.99 * MCLK*/
#define CNS3XXX_DMC_T_RFC_266			0x22	/* 127.5 ns = 33.915 * MCLK*/
#define CNS3XXX_DMC_T_RP_266			0x04	/* 15 ns = 3.99 * MCLK */
#define CNS3XXX_DMC_T_RRD_266			0x03	/* 10 ns = 2.66 * MCLK */
#define CNS3XXX_DMC_T_WR_266			0x04	/* 15 ns = 3.99 * MCLK */
#define CNS3XXX_DMC_T_WTR_266			0x02	/* 7.5 ns = 1.995 * MCLK*/
#define CNS3XXX_DMC_T_FAW_266			0x0C	/* 45 ns = 11.97 * MCLK */
#define CNS3XXX_DMC_REFRESH_PRD_266		0x81B	/* 7800ns = 2074.804 * MCLK */
/* 
 * DMC parameter for DDR2 clock frequency 333 MHz, 
 * MCLK = 333 MHz, clock cycle 3.003ns
 */
#define CNS3XXX_DMC_T_RAS_333			0x0F	/* 45 ns = 14.985 * MCLK */		
#define CNS3XXX_DMC_T_RC_333			0x14	/* 60 ns = 19.98 * MCLK */
#define CNS3XXX_DMC_T_RCD_333			0x05	/* 15 ns = 4.995 * MCLK */
#define CNS3XXX_DMC_T_RFC_333			0x2B	/* 127.5 ns = 42.457 * MCLK */
#define CNS3XXX_DMC_T_RP_333			0x05	/* 15 ns = 4.995 * MCLK */
#define CNS3XXX_DMC_T_RRD_333			0x04	/* 10 ns = 3.33 * MCLK */
#define CNS3XXX_DMC_T_WR_333			0x05	/* 15 ns = 4.995 * MCLK */
#define CNS3XXX_DMC_T_WTR_333			0x03	/* 7.5 ns = 2.4975 * MCLK */
#define CNS3XXX_DMC_T_FAW_333			0x0F	/* 45 ns = 14.985 * MCLK*/
#define CNS3XXX_DMC_REFRESH_PRD_333		0xA26	/* 7800 ns = 2597.4 * MCLK */

/* 
 * DMC parameter for DDR2 clock frequency 400 MHz, 
 * MCLK = 400 MHz, clock cycle 2.50ns
 */
#define CNS3XXX_DMC_T_RAS_400			0x12	/* 45 ns = 18 * MCLK */
#define CNS3XXX_DMC_T_RC_400			0x18	/* 60 ns = 24 * MCLK */
#define CNS3XXX_DMC_T_RCD_400			0x06	/* 15 ns = 6 * MCLK */
#define CNS3XXX_DMC_T_RFC_400			0x33	/* 127.5 ns = 51 * MCLK*/
#define CNS3XXX_DMC_T_RP_400			0x06	/* 15 ns = 6 * MCLK */
#define CNS3XXX_DMC_T_RRD_400			0x04	/* 10 ns = 4 * MCLK */
#define CNS3XXX_DMC_T_WR_400			0x06	/* 15 ns = 6 * MCLK */
#define CNS3XXX_DMC_T_WTR_400			0x03	/* 7.5 ns = 3 * MCLK*/
#define CNS3XXX_DMC_T_FAW_400			0x12	/* 45 ns = 18 * MCLK */
#define CNS3XXX_DMC_REFRESH_PRD_400		0xC30	/* 7800 ns = 3120 * MCLK */

/* ACLK = 300 MHz (i.e. CPU 600MHz), clock cycle 3.33ns */
#define CNS3XXX_DMC_SCH_RCD_A300			0x02	/* RCD    15 ns = 4.5 * ACLK => 5-3 = 2 */
#define CNS3XXX_DMC_SCH_RFC_A300			0x24	/* RFC 127.5 ns = 38.28 * ACLK => 39-3 = 36*/
#define CNS3XXX_DMC_SCH_RP_A300				0x02	/* RP     15 ns = 4.5 * ACLK => 5-3 = 2 */
#define CNS3XXX_DMC_SCH_FAW_A300			0x0B	/* FAW    45 ns = 13.5 * ACLK => 14-3 = 11*/

/* ACLK = 350 MHz (i.e. CPU 700MHz), clock cycle 2.857ns */
#define CNS3XXX_DMC_SCH_RCD_A350			0x03	/* RCD    15 ns = 5.25 * ACLK => 6-3 = 3 */
#define CNS3XXX_DMC_SCH_RFC_A350			0x2A	/* RFC 127.5 ns = 44.62 * ACLK => 45-3 = 42 */
#define CNS3XXX_DMC_SCH_RP_A350				0x03	/* RP     15 ns = 5.25 * ACLK => 6-3 = 3 */
#define CNS3XXX_DMC_SCH_FAW_A350			0x0D	/* FAW    45 ns = 15.75 * ACLK => 16-3 = 13 */

/* for memory_cfg[5:3] */
#define DMC_ROW_BITS_11_BITS	0x0
#define DMC_ROW_BITS_12_BITS	0x1
#define DMC_ROW_BITS_13_BITS	0x2
#define DMC_ROW_BITS_14_BITS	0x3
#define DMC_ROW_BITS_15_BITS	0x4
#define DMC_ROW_BITS_16_BITS	0x5

/* for memory_cfg[2:0] */
#define DMC_COLUMN_BITS_9_BITS	0x1
#define DMC_COLUMN_BITS_10_BITS	0x2
#define DMC_COLUMN_BITS_11_BITS	0x3

/* memory_cfg2 register (0x7200004C) */
/* for memory_cfg2[5:4] */
#define DMC_BANK_BITS_2_BITS	0x0
#define DMC_BANK_BITS_3_BITS	0x3

/* memory_cfg register (0x7200000C) */
#define MEMORY_CFG_2048_16	0x0001401A	/* 2048Mbit, data width 16 */
#define MEMORY_CFG_2048_8	0x00014022	/* 2048Mbit, data width  8 */
#define MEMORY_CFG_1024_16	0x00014012	/* 1024Mbit, data width 16 */
#define MEMORY_CFG_1024_8	0x0001401A	/* 1024Mbit, data width  8 */
#define MEMORY_CFG_512_16	0x00014012	/*  512Mbit, data width 16 */
#define MEMORY_CFG_512_8	0x0001401A	/*  512Mbit, data width  8 */

/* for memory_cfg2[7:6] */
#define DMC_MEMORY_WIDTH_16_BITS	0x0
#define DMC_MEMORY_WIDTH_32_BITS	0x1
#define DMC_MEMORY_WIDTH_64_BITS	0x2

/*
 *	DRAM Configuration 
 *	           ba / ra / ca
 *	2048 x 16   3 / 15 / 10 
 *	2048 x  8   3 / 14 / 10 
 *	1024 x 16   3 / 13 / 10
 *	1024 x  8   3 / 14 / 10
 *	 512 x 16   2 / 13 / 10
 *	 512 x  8   2 / 14 / 10
 */
#ifdef MEM_1024MBIT_WIDTH_16
	#define DMC_BANK_BITS	DMC_BANK_BITS_3_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_13_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#elif defined MEM_1024MBIT_WIDTH_8
	#define DMC_BANK_BITS	DMC_BANK_BITS_3_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_14_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#elif defined MEM_512MBIT_WIDTH_16
	#define DMC_BANK_BITS	DMC_BANK_BITS_2_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_13_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#elif defined MEM_512MBIT_WIDTH_8
	#define DMC_BANK_BITS	DMC_BANK_BITS_2_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_14_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#elif defined MEM_2048MBIT_WIDTH_16
	#define DMC_BANK_BITS	DMC_BANK_BITS_3_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_14_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#elif defined MEM_2048MBIT_WIDTH_8
	#define DMC_BANK_BITS	DMC_BANK_BITS_3_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_15_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#else	/* default is 1024Mb, datawidth:16 */
	#define DMC_BANK_BITS	DMC_BANK_BITS_3_BITS
	#define DMC_ROW_BITS	DMC_ROW_BITS_13_BITS
	#define DMC_COLUMN_BITS DMC_COLUMN_BITS_10_BITS
#endif

#ifdef PHYS_SDRAM_32BIT
	#define DMC_MEMORY_WIDTH DMC_MEMORY_WIDTH_32_BITS
#else
	#define DMC_MEMORY_WIDTH DMC_MEMORY_WIDTH_16_BITS
#endif

#endif /* end #ifndef CNS3XXX_DMC_H */
