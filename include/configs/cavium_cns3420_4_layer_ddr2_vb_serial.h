/*
 * (C) Copyright 2003
 * Texas Instruments.
 * Kshitij Gupta <kshitij@ti.com>
 * Configuation settings for the TI OMAP Innovator board.
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 * Configuration for Versatile PB.
 *
 * (C) Copyright 2008
 * Cavium Networks Ltd.
 * Scott Shu <scott.shu@caviumnetworks.com>
 * Configuration for Cavium Networks CNS3000 Platform
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H
/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define PHYS_SDRAM_32BIT					/* undefined: 16 bits, defined: 32 bits */
#define MEM_512MBIT_WIDTH_16

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT		(120 * CFG_HZ)		/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT		(5 * CFG_HZ)		/* Timeout for Flash Write */

#define UBOOT_TYPE "CNS3420 serial flash"

/*
 * SPI serial flash (dataflash) (Base Address: 0x60000000)
 */
#define CONFIG_SPI_FLASH_BOOT		1
#define CONFIG_SPI			1

/* PSE MAC/PHY Configuration */
#define CONFIG_VB_2

#include <configs/cavium_cns3xxx_common.h>

#endif /* __CONFIG_H */
