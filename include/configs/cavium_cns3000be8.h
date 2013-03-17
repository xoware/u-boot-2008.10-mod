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
/*
 *  Code, etc. common to all ARM supplied development boards
 */
#include <armsupplied.h>

/*
 * Board info register
 */
#define SYS_ID  (0x10000000)
#define ARM_SUPPLIED_REVISION_REGISTER SYS_ID

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_CNS3000		1		/* in a Cavium Networks CNS3000 SoC */

#define CONFIG_DISPLAY_CPUINFO	1		/* display cpu info */
#define CONFIG_DISPLAY_BOARDINFO 1		/* display board info */

#define CFG_MEMTEST_START	0x100000
#define CFG_MEMTEST_END		0x10000000

#define CFG_HZ	       		(1000)
#define CFG_HZ_CLOCK		1000000		/* Timers clocked at 1Mhz */
#define CFG_TIMERBASE		0x7C800000	/* Timer 1 base	*/
#define CFG_TIMER_RELOAD	0xFFFFFFFF
#define TIMER_LOAD_VAL		CFG_TIMER_RELOAD

//#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs	*/
//#define CONFIG_SETUP_MEMORY_TAGS	1
//#define CONFIG_MISC_INIT_R		1	/* call misc_init_r during start up */

/*
 * Size of malloc() pool
 */
/* scott.check */
#define CFG_MALLOC_LEN		(CONFIG_ENV_SIZE + 512*1024)
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

/*
 * Hardware drivers
 */

/*
 * select serial console configuration
 */
#define CFG_CNS3000_CONSOLE	CNS3000_UART1	/* we use UART1 for console */

/*
 * NS16550 Configuration
 */
# define CFG_SERIAL0		0x78000000
# define CFG_SERIAL1		0x78400000
# define CFG_SERIAL2		0x78800000
# define CFG_SERIAL3		0x78C00000

#define CONFIG_CNS3000_SERIAL
#define CONFIG_CNS3000_CLOCK	12288000
#define CONFIG_CNS3000_PORTS	{ (void *)CFG_SERIAL0, (void *)CFG_SERIAL1 }
#define CONFIG_CONS_INDEX	0

#define CONFIG_BAUDRATE		38400
#define CFG_BAUDRATE_TABLE	{ 2400, 4800, 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_CMDLINE_EDITING

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

/*
 * Real Time Clock
 */
#undef CONFIG_RTC_CNS3000

/* 
 * MMC/SD Host Controller
 */
#undef CONFIG_CNS3000_MMC
#ifdef CONFIG_CNS3000_MMC
#define CONFIG_MMC              1
#define CONFIG_DOS_PARTITION    1
#endif

/*
 * Command line configuration.
 */
#define CONFIG_CMD_BDI
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_ENV
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_IMI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_LOADB
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_ELF
#define CONFIG_CMD_RUN

#ifdef CONFIG_RTC_CNS3000
#define CONFIG_CMD_DATE
#endif

#ifdef CONFIG_CNS3000_MMC
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT
#endif

#define CONFIG_BOOTDELAY	2

#undef CONFIG_UDP_FRAGMENT

#ifdef CONFIG_UDP_FRAGMENT
#define CONFIG_EXTRA_ENV_SETTINGS	\
	"tftp_bsize=512\0"		\
	"udp_frag_size=512\0"
#endif

/*
The kernel command line & boot command below are for a Cavium Networks CNS3000 board
0x00000000  u-boot
0x0000????  knuxernel 
0x0000????  Root File System
*/

#define CONFIG_BOOTARGS "root=/dev/mtdblock0 mem=256M console=ttyS0"
#define CONFIG_BOOTCOMMAND "cp 0x10040000 0x100000 0x100000; bootm"

/*
 * Static configuration when assigning fixed address
 */
#define CONFIG_ETHADDR		00:53:43:4F:54:54
#define CONFIG_NETMASK		255.255.0.0		/* talk on MY local net */
#define CONFIG_IPADDR		172.20.5.230		/* static IP I currently own */
#define CONFIG_SERVERIP		172.20.5.200		/* current IP of my dev pc */
#define CONFIG_BOOTFILE		"/tftpboot/uImage"	/* file to load */

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP				/* undef to save memory		 */
#define CFG_PROMPT		"CNS3000 # "
#define CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
/* Print Buffer Size */
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16)
#define CFG_MAXARGS		16		/* max number of command args	 */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size		*/

#undef	CFG_CLKS_IN_HZ				/* everything, incl board info, in Hz */
#define CFG_LOAD_ADDR		0x00800000	/* default load address */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#undef PHYS_SDRAM_32BIT					/* undefined: 16 bits, defined: 32 bits */

#define CONFIG_NR_DRAM_BANKS		1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1		       	0x00000000	/* SDRAM Bank #1 */

#ifdef PHYS_SDRAM_32BIT
#define PHYS_SDRAM_1_SIZE		0x10000000	/* 0x10000000 = 256 MB */
#else
#define PHYS_SDRAM_1_SIZE		0x08000000	/* 0x08000000 = 128 MB */
#endif

#define CFG_MONITOR_BASE		TEXT_BASE

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
/*
 *  Use the CFI flash driver for ease of use
 */
#define CFG_FLASH_BASE			0x10000000

#define CFG_FLASH_CFI			1
#define CONFIG_FLASH_CFI_DRIVER		1
#define CONFIG_FLASH_SHOW_PROGRESS	45			/* count down from 45/5: 9..1 */
#define CFG_FLASH_CFI_WIDTH		FLASH_CFI_8BIT
#define CONFIG_FLASH_CFI_LEGACY
#define CFG_FLASH_LEGACY_8MiBx8

#define CFG_MAX_FLASH_BANKS		1			/* max number of memory banks */
#define PHYS_FLASH_SIZE			0x00800000		/* 8MB */
#define CFG_MAX_FLASH_SECT		135			/* 135 max number of sectors on one chip */

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT		(5 * CFG_HZ)		/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT		(5 * CFG_HZ)		/* Timeout for Flash Write */

#define CFG_FLASH_EMPTY_INFO		1			/* flinfo indicates empty blocks */

/*
 * SPI serial flash (dataflash) (Base Address: 0x60000000)
 */
#define CFG_SPI_FLASH_BASE		0x60000000

#define CFG_MAX_DATAFLASH_BANKS		1
#define CFG_DATAFLASH_LOGIC_ADDR_CS0	CFG_SPI_FLASH_BASE	/* Logical adress for CS0 */

/* environment data, spi dataflash only. */
#undef  CONFIG_ENV_IS_IN_DATAFLASH
//#define  CONFIG_ENV_IS_IN_DATAFLASH

#ifdef CONFIG_ENV_IS_IN_DATAFLASH

#undef CFG_FLASH_CFI
#undef CONFIG_FLASH_CFI_DRIVER
#undef CONFIG_FLASH_CFI_LEGACY
#undef CFG_FLASH_LEGACY_8MiBx8

#define CONFIG_HAS_DATAFLASH		1

#define CONFIG_ENV_SIZE			0x1000			/* Total Size of Environment Sector */
#define CONFIG_ENV_OFFSET		0x30000			/* the offset of u-boot environment on dataflash */
#define CONFIG_ENV_ADDR			(CFG_DATAFLASH_LOGIC_ADDR_CS0 + CONFIG_ENV_OFFSET) /* the address of environment */

#define CONFIG_KERNEL_OFFSET		0x40000			/* the offset of bootpImage on dataflash */
#define	CONFIG_KERNEL_END		0x7FFFFF		/* the end of bootpImage on dataflash */
#else
#define CONFIG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_SIZE			0x2000
#define CONFIG_ENV_OFFSET		(PHYS_FLASH_SIZE - CONFIG_ENV_SIZE)
#define CONFIG_ENV_ADDR			(CFG_FLASH_BASE + CONFIG_ENV_OFFSET)
#endif /* CONFIG_ENV_IS_IN_DATAFLASH */

#ifdef CONFIG_SPI_FLASH_BOOT
#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND		"go 0x1000000"
#endif /* CONFIG_SPI_FLASH_BOOT */

// If FPGA, force some actions
//#define CONFIG_CNS3000_FPGA

// Warning: Seems has memory leak issue, it might be u-boot bug. 
// Need to check again.
//#define CONFIG_AHCI_CNS3000

#ifdef CONFIG_AHCI_CNS3000
#define CONFIG_CMD_SCSI
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_PCI
//#define CONFIG_SCSI_AHCI
#define CONFIG_DOS_PARTITION

// Ignore compile error
#define CONFIG_SATA_ULI5288

#define CFG_SCSI_MAX_SCSI_ID    2
#define CFG_SCSI_MAX_LUN        2
#define CFG_SCSI_MAX_DEVICE     (CFG_SCSI_MAX_SCSI_ID * CFG_SCSI_MAX_LUN)
#define CFG_SCSI_MAXDEVICE      CFG_SCSI_MAX_DEVICE
#endif



#endif /* __CONFIG_H */
