/*
 * Copyright (c) 2008 Cavium Networks 
 * 
 * This file is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2, as 
 * published by the Free Software Foundation. 
 *
 * This file is distributed in the hope that it will be useful, 
 * but AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or 
 * NONINFRINGEMENT.  See the GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA or 
 * visit http://www.gnu.org/licenses/. 
 *
 * This file may also be available under a different license from Cavium. 
 * Contact Cavium Networks for more information
 *
 */

#include <common.h>

#if defined(CONFIG_DISPLAY_CPUINFO)
#define PM_BASE_VIRT		0x77000000
#define PMU_REG_VALUE(offset)	(*((volatile unsigned int *)(PM_BASE_VIRT+offset)))	
#define PLL_HM_PD_CTRL_REG 	PMU_REG_VALUE(0x1C)
#define PLL_LCD_CTRL_REG	PMU_REG_VALUE(0x18)
#define CHIP_REVISION_REG	0x76000A54
#define EFUSE_CHIP_REVISION	0x76000048
#define WAVE_LOT_NUM		0x76000040

int print_cpuinfo (void)
{
	unsigned long id_code;
	unsigned long cpu_id;
	unsigned long rev_id;
	unsigned long rev_id_from_efuse;
	unsigned long wave_lot_number;

	asm ("mrc p15, 0, %0, c0, c0, 0":"=r" (id_code));
	asm ("mrc p15, 0, %0, c0, c0, 5":"=r" (cpu_id));

	printf("CPU: Cavium Networks CNS3000\n");
	printf("ID Code: %lx ", id_code);

	switch ((id_code & 0x0000fff0) >> 4) {
	case 0xb02:
		printf("(Part number: 0xB02, ");
		break;

	default:
		printf("(Part number: unknown, ");
		
	}
	printf("Revision number: %x) \n", (id_code & 0x0000000f));

	printf("CPU ID: %lx \n", cpu_id);
	
	rev_id =*((volatile unsigned int*)CHIP_REVISION_REG);
	rev_id >>= 28; 
	rev_id_from_efuse = *((volatile unsigned int*)EFUSE_CHIP_REVISION);
	wave_lot_number = *((volatile unsigned int *)WAVE_LOT_NUM);
	
	if( rev_id == 0x0 )
	{
		if( ((PLL_HM_PD_CTRL_REG & 0x00000020) >> 5 == 0x00) && 
			( PLL_LCD_CTRL_REG & 0x00C00000 ) >>22 == 0x3 )
		{
			printf("Chip Version: b\n");
			return 0;
		}	
		else
		{
			printf("Chip Version: a\n");
			return 0;
		}
	}
#if 0
	else if( rev_id == 0x2)
		printf("Chip Version: c\n");
	else if( rev_id == 0x3)
		printf("Chip Version: d\n");
#endif
	else if( rev_id_from_efuse == 0x0020)
	{
		if( (wave_lot_number & 0x02914A08) == 0x2914A08 || 
		    (wave_lot_number & 0x00914A08) == 0x0914A08 ||
		    (wave_lot_number & 0x03114A08) == 0x3114A08 ||
		    (wave_lot_number & 0x02114A08) == 0x2114A08 ||
		    (wave_lot_number & 0x00053888) == 0x53888 )
		{
			printf("Chip Version: d\n");
			return 0;	
		}
		else 
		{
			printf("Chip Version: c\n");
			return 0;
		}

	}
	else if( rev_id_from_efuse == 0x0060)
	{	
		printf("chip Version: d\n");	
		return 0;
	}
#if 1
	if(	(wave_lot_number & 0x02914A08) == 0x2914A08 || 
		(wave_lot_number & 0x00914A08) == 0x0914A08 ||
		(wave_lot_number & 0x03114A08) == 0x3114A08 ||
		(wave_lot_number & 0x02114A08) == 0x2114A08 ||
		(wave_lot_number & 0x00053888) == 0x53888  )
	{

		printf("Chip Version: d\n");
		return 0;
	}
	else
	{
		
			printf("Chip Version: c\n");
			return 0;
	}

	printf("Chip Version: unkown,efuse_rev_id=%x,rev_id=%x, wave_lot_number=%x\n", rev_id_from_efuse,rev_id, wave_lot_number);
#endif	
	return 0;
}
#endif
