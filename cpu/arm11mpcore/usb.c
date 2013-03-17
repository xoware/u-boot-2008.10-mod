/*
 * Copyright (c) 2008 Cavium Networks
 *
 * Scott Shu
 *
 * USB OHCI HCD (Host Controller Driver) initialization on the CNW5XXX.
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
 */

#include <common.h>

#include <cns3000.h>

#define readl(a) (*((volatile u32 *)(a)))
#define writel(a, b) (*((volatile u32 *)(b)) = ((volatile u32)a))

#if defined(CONFIG_USB_OHCI_NEW) && defined(CFG_USB_OHCI_CPU_INIT)

int usb_cpu_init(void)
{
	unsigned int u32tmp;

	/* Power On */
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_PLL_HM_PD_CTRL_OFFSET);
	u32tmp &= ~(0x1 << 9);	/* USB PHY */
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_PLL_HM_PD_CTRL_OFFSET);

	/*
	 * Enable USB host clock.
	 */
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_CLK_GATE_OFFSET);
	u32tmp |= (0x1 << 16);
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_CLK_GATE_OFFSET);

	/* Software Reset */
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_SOFT_RST_OFFSET);
	u32tmp &= ~(0x1 << 16);
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_SOFT_RST_OFFSET);
	wait_ms (100);
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_SOFT_RST_OFFSET);
	u32tmp |= (0x1 << 16);
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_SOFT_RST_OFFSET);

	return 0;
}

int usb_cpu_stop(void)
{
	unsigned int u32tmp;

	/*
	 * Disable USB host clock.
	 */
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_CLK_GATE_OFFSET);
	u32tmp &= ~(0x1 << 16);
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_CLK_GATE_OFFSET);

	/* Power Down */
	u32tmp = readl(CNS3000_VEGA_PMU_BASE + PMU_PLL_HM_PD_CTRL_OFFSET);
	u32tmp |= (0x1 << 9);	/* USB PHY */
	writel(u32tmp, CNS3000_VEGA_PMU_BASE + PMU_PLL_HM_PD_CTRL_OFFSET);

	return 0;
}

void usb_cpu_init_fail(void)
{

}

#endif /* defined(CONFIG_USB_OHCI_NEW) && defined(CFG_USB_OHCI_CPU_INIT) */
