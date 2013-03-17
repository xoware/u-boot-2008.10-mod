/*
 * Copyright (C) Freescale Semiconductor, Inc. 2006. All rights reserved.
 * Author: Jason Jin<Jason.jin@freescale.com>
 *         Zhang Wei<wei.zhang@freescale.com>
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
 *
 * with the reference on libata and ahci drvier in kernel
 *
 */
#include <common.h>

#include <command.h>
#include <pci.h>
#include <asm/processor.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <scsi.h>
#include <ata.h>
#include <linux/ctype.h>
#include <ahci.h>
#include <cns3000.h>

#include <../board/cavium/cns3000/pm.h>
#include <../board/cavium/cns3000/misc.h>

#define COMMANDSET2_LBA48	(1 << 10)

struct ahci_probe_ent *probe_ent = NULL;
hd_driveid_t *ataid[AHCI_MAX_PORTS];

#define writel_with_flush(a,b)	do { writel(a,b); readl(b); } while (0)


static inline u32 ahci_port_base(u32 base, u32 port)
{
	return base + 0x100 + (port * 0x80);
}


static void ahci_setup_port(struct ahci_ioports *port, unsigned long base,
			    unsigned int port_idx)
{
	base = ahci_port_base(base, port_idx);

	port->cmd_addr = base;
	port->scr_addr = base + PORT_SCR;
}


#define msleep(a) udelay(a * 1000)
#define ssleep(a) msleep(a * 1000)

static int waiting_for_cmd_completed(volatile u8 *offset,
				     int timeout_msec,
				     u32 sign)
{
	int i;
	u32 status;

	for (i = 0; ((status = readl(offset)) & sign) && i < timeout_msec; i++)
		msleep(1);

	return (i < timeout_msec) ? 0 : -1;
}


static int ahci_host_init(struct ahci_probe_ent *probe_ent)
{
	pci_dev_t pdev = probe_ent->dev;
	volatile u8 *mmio = (volatile u8 *)probe_ent->mmio_base;
	u32 tmp, cap_save;
	u16 tmp16;
	int i, j;
	volatile u8 *port_mmio;
	unsigned short vendor;

	cap_save = readl(mmio + HOST_CAP);
	cap_save &= ((1 << 28) | (1 << 17));
	cap_save |= (1 << 27);

	/* global controller reset */
	tmp = readl(mmio + HOST_CTL);
	if ((tmp & HOST_RESET) == 0)
		writel_with_flush(tmp | HOST_RESET, mmio + HOST_CTL);

	/* reset must complete within 1 second, or
	 * the hardware should be considered fried.
	 */
	ssleep(1);

	tmp = readl(mmio + HOST_CTL);
	if (tmp & HOST_RESET) {
		debug("controller reset failed (0x%x)\n", tmp);
		return -1;
	}

	writel_with_flush(HOST_AHCI_EN, mmio + HOST_CTL);
	writel(cap_save, mmio + HOST_CAP);
	writel_with_flush(0xf, mmio + HOST_PORTS_IMPL);

	probe_ent->cap = readl(mmio + HOST_CAP);
	probe_ent->port_map = readl(mmio + HOST_PORTS_IMPL);
	probe_ent->n_ports = (probe_ent->cap & 0x1f) + 1;

	debug("cap 0x%x  port_map 0x%x  n_ports %d\n",
	      probe_ent->cap, probe_ent->port_map, probe_ent->n_ports);

	for (i = 0; i < probe_ent->n_ports; i++) {
		probe_ent->port[i].port_mmio = ahci_port_base((u32) mmio, i);
		port_mmio = (u8 *) probe_ent->port[i].port_mmio;
		ahci_setup_port(&probe_ent->port[i], (unsigned long)mmio, i);

		/* make sure port is not active */
		tmp = readl(port_mmio + PORT_CMD);
		if (tmp & (PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
			   PORT_CMD_FIS_RX | PORT_CMD_START)) {
			tmp &= ~(PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
				 PORT_CMD_FIS_RX | PORT_CMD_START);
			writel_with_flush(tmp, port_mmio + PORT_CMD);

			/* spec says 500 msecs for each bit, so
			 * this is slightly incorrect.
			 */
			msleep(500);
		}

		writel(PORT_CMD_SPIN_UP, port_mmio + PORT_CMD);

		j = 0;
		while (j < 100) {
			msleep(10);
			tmp = readl(port_mmio + PORT_SCR_STAT);
			if ((tmp & 0xf) == 0x3)
				break;
			j++;
		}

#if defined(CONFIG_CNS3000_FPGA)

      /* Force to Gen1 Speed (1.5G) */
      tmp = readl(port_mmio + PORT_SCR_CTL);

      tmp |=  0x1 << 4;
      writel(tmp, port_mmio + PORT_SCR_CTL);

#endif /* #if defined(BUILD_FOR_FPGA) */


		tmp = readl(port_mmio + PORT_SCR_ERR);
		debug("PORT_SCR_ERR 0x%x\n", tmp);
		writel(tmp, port_mmio + PORT_SCR_ERR);

		/* ack any pending irq events for this port */
		tmp = readl(port_mmio + PORT_IRQ_STAT);
		debug("PORT_IRQ_STAT 0x%x\n", tmp);
		if (tmp)
			writel(tmp, port_mmio + PORT_IRQ_STAT);

		writel(1 << i, mmio + HOST_IRQ_STAT);

		/* set irq mask (enables interrupts) */
		writel(DEF_PORT_IRQ, port_mmio + PORT_IRQ_MASK);

		/*register linkup ports */
		tmp = readl(port_mmio + PORT_SCR_STAT);
		debug("Port %d status: 0x%x\n", i, tmp);
		if ((tmp & 0xf) == 0x03)
			probe_ent->link_port_map |= (0x01 << i);
	}

	tmp = readl(mmio + HOST_CTL);
	debug("HOST_CTL 0x%x\n", tmp);
	writel(tmp | HOST_IRQ_EN, mmio + HOST_CTL);
	tmp = readl(mmio + HOST_CTL);
	debug("HOST_CTL 0x%x\n", tmp);

	return 0;
}


static void ahci_print_info(struct ahci_probe_ent *probe_ent)
{
//	pci_dev_t pdev = probe_ent->dev;
	volatile u8 *mmio = (volatile u8 *)probe_ent->mmio_base;
	u32 vers, cap, impl, speed;
	const char *speed_s;
	u16 cc;
	const char *scc_s;

	vers = readl(mmio + HOST_VERSION);
	cap = probe_ent->cap;
	impl = probe_ent->port_map;

	speed = (cap >> 20) & 0xf;
	if (speed == 1)
		speed_s = "1.5";
	else if (speed == 2)
		speed_s = "3";
	else
		speed_s = "?";

		scc_s = "SATA";

	printf("AHCI %02x%02x.%02x%02x "
	       "%u slots %u ports %s Gbps 0x%x impl %s mode\n",
	       (vers >> 24) & 0xff,
	       (vers >> 16) & 0xff,
	       (vers >> 8) & 0xff,
	       vers & 0xff,
	       ((cap >> 8) & 0x1f) + 1, (cap & 0x1f) + 1, speed_s, impl, scc_s);

	printf("flags: "
	       "%s%s%s%s%s%s"
	       "%s%s%s%s%s%s%s\n",
	       cap & (1 << 31) ? "64bit " : "",
	       cap & (1 << 30) ? "ncq " : "",
	       cap & (1 << 28) ? "ilck " : "",
	       cap & (1 << 27) ? "stag " : "",
	       cap & (1 << 26) ? "pm " : "",
	       cap & (1 << 25) ? "led " : "",
	       cap & (1 << 24) ? "clo " : "",
	       cap & (1 << 19) ? "nz " : "",
	       cap & (1 << 18) ? "only " : "",
	       cap & (1 << 17) ? "pmp " : "",
	       cap & (1 << 15) ? "pio " : "",
	       cap & (1 << 14) ? "slum " : "",
	       cap & (1 << 13) ? "part " : "");
}

static int ahci_init_one(pci_dev_t pdev)
{
//	u32 iobase;
	u16 vendor;
	int rc;

	memset((void *)ataid, 0, sizeof(hd_driveid_t *) * AHCI_MAX_PORTS);

	probe_ent = malloc(sizeof(struct ahci_probe_ent));
	memset(probe_ent, 0, sizeof(struct ahci_probe_ent));

	probe_ent->dev = pdev;

	probe_ent->host_flags = ATA_FLAG_SATA
				| ATA_FLAG_NO_LEGACY
				| ATA_FLAG_MMIO
				| ATA_FLAG_PIO_DMA
				| ATA_FLAG_NO_ATAPI;
	probe_ent->pio_mask = 0x1f;
	probe_ent->udma_mask = 0x7f;	/*Fixme,assume to support UDMA6 */

	probe_ent->mmio_base = CNS3000_VEGA_AHCI_BASE;

#if defined(CONFIG_CNS3000_FPGA)
      {
	u32 sata_phy_init;
	/*
	* force sata phy at GEN. 1
	*/
	    *(u32 volatile *) 0x76000604 = 0xB;
	/*
	* Initial SATA PHY TEST CHIP
	* 1. reset
	* 2. reverse comma_det
	*/
	    sata_phy_init  = 0xf;
	//  sata_phy_init |= 0xf << 8;
	    *(u32 volatile *) 0x76000514 = sata_phy_init;

	/*
	* set 1ms timer, hclk = 30Mhz
	*/
	    *(u32 volatile *) (CNS3000_VEGA_AHCI_BASE + 0xe0) = 30000;
      }

#else
	/*
	 * set 1ms timer
	 */
	debug ("Set Timer 1ms = %d\n",cns3xxx_pll_cpu_clock() * 500);
	*(u32 volatile *) (CNS3000_VEGA_AHCI_BASE + 0xe0) = cns3xxx_pll_cpu_clock() * 500;
#endif

	/* initialize adapter */
	rc = ahci_host_init(probe_ent);
	if (rc)
		goto err_out;

	ahci_print_info(probe_ent);

	return 0;

      err_out:
	return rc;
}


#define MAX_DATA_BYTE_COUNT  (4*1024*1024)

static int ahci_fill_sg(u8 port, unsigned char *buf, int buf_len)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	struct ahci_sg *ahci_sg = pp->cmd_tbl_sg;
	u32 sg_count;
	int i;

	sg_count = ((buf_len - 1) / MAX_DATA_BYTE_COUNT) + 1;
	if (sg_count > AHCI_MAX_SG) {
		printf("Error:Too much sg!\n");
		return -1;
	}

	for (i = 0; i < sg_count; i++) {
		ahci_sg->addr =
		    cpu_to_le32((u32) buf + i * MAX_DATA_BYTE_COUNT);
		ahci_sg->addr_hi = 0;
		ahci_sg->flags_size = cpu_to_le32(0x3fffff &
					  (buf_len < MAX_DATA_BYTE_COUNT
					   ? (buf_len - 1)
					   : (MAX_DATA_BYTE_COUNT - 1)));
		ahci_sg++;
		buf_len -= MAX_DATA_BYTE_COUNT;
	}

	return sg_count;
}


static void ahci_fill_cmd_slot(struct ahci_ioports *pp, u32 opts)
{
	pp->cmd_slot->opts = cpu_to_le32(opts);
	pp->cmd_slot->status = 0;
	pp->cmd_slot->tbl_addr = cpu_to_le32(pp->cmd_tbl & 0xffffffff);
	pp->cmd_slot->tbl_addr_hi = 0;
}


static void ahci_set_feature(u8 port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 cmd_fis_len = 5;	/* five dwords */
	u8 fis[20];

	/*set feature */
	memset(fis, 0, 20);
	fis[0] = 0x27;
	fis[1] = 1 << 7;
	fis[2] = ATA_CMD_SETF;
	fis[3] = SETFEATURES_XFER;
//	fis[12] = __ilog2(probe_ent->udma_mask + 1) + 0x40 - 0x01;
	fis[12] = 0x46;  // Force DMA mode 6

	memcpy((unsigned char *)pp->cmd_tbl, fis, 20);
	ahci_fill_cmd_slot(pp, cmd_fis_len);
	writel(1, port_mmio + PORT_CMD_ISSUE);
	readl(port_mmio + PORT_CMD_ISSUE);

	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE, 150, 0x1)) {
		printf("set feature error!\n");
	}
}


static int ahci_port_start(u8 port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 port_status;
	u32 mem;

	debug("Enter start port: %d\n", port);
	port_status = readl(port_mmio + PORT_SCR_STAT);
	debug("Port %d status: %x\n", port, port_status);
	if ((port_status & 0xf) != 0x03) {
		printf("No Link on this port!\n");
		return -1;
	}
	mem = (u32) malloc(AHCI_PORT_PRIV_DMA_SZ + 2048);
	if (!mem) {
		printf("pp:%X \n",pp);
		if(pp) free(pp);
		printf("No mem for table!\n");
		return -ENOMEM;
	}

	mem = (mem + 0x800) & (~0x7ff);	/* Aligned to 2048-bytes */
	memset((u8 *) mem, 0, AHCI_PORT_PRIV_DMA_SZ);

	/*
	 * First item in chunk of DMA memory: 32-slot command table,
	 * 32 bytes each in size
	 */
	pp->cmd_slot = (struct ahci_cmd_hdr *)mem;
	debug("cmd_slot = 0x%x\n", pp->cmd_slot);
	mem += (AHCI_CMD_SLOT_SZ + 224);

	/*
	 * Second item: Received-FIS area
	 */
	pp->rx_fis = mem;
	mem += AHCI_RX_FIS_SZ;

	/*
	 * Third item: data area for storing a single command
	 * and its scatter-gather table
	 */
	pp->cmd_tbl = mem;
	debug("cmd_tbl_dma = 0x%x\n", pp->cmd_tbl);

	mem += AHCI_CMD_TBL_HDR;
	pp->cmd_tbl_sg = (struct ahci_sg *)mem;

	writel_with_flush((u32) pp->cmd_slot, port_mmio + PORT_LST_ADDR);

	writel_with_flush(pp->rx_fis, port_mmio + PORT_FIS_ADDR);

	writel_with_flush(PORT_CMD_ICC_ACTIVE | PORT_CMD_FIS_RX |
			  PORT_CMD_POWER_ON | PORT_CMD_SPIN_UP |
			  PORT_CMD_START, port_mmio + PORT_CMD);

	debug("Exit start port %d\n", port);

	return 0;
}


static int get_ahci_device_data(u8 port, u8 *fis, int fis_len, u8 *buf,
				int buf_len)
{

	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 opts;
	u32 port_status;
	int sg_count;

	debug("Enter get_ahci_device_data: for port %d\n", port);

	if (port > probe_ent->n_ports) {
		printf("Invaild port number %d\n", port);
		return -1;
	}

	port_status = readl(port_mmio + PORT_SCR_STAT);
	if ((port_status & 0xf) != 0x03) {
		debug("No Link on port %d!\n", port);
		return -1;
	}

	memcpy((unsigned char *)pp->cmd_tbl, fis, fis_len);

	sg_count = ahci_fill_sg(port, buf, buf_len);
	opts = (fis_len >> 2) | (sg_count << 16);
	ahci_fill_cmd_slot(pp, opts);

	writel_with_flush(1, port_mmio + PORT_CMD_ISSUE);

	// 150 msec often times out when reading large file such as bootpImage */
//	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE, 150, 0x1)) {
	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE, 300, 0x1)) {
		printf("timeout exit!\n");
		return -1;
	}
	debug("get_ahci_device_data: %d byte transferred.\n",
	      pp->cmd_slot->status);

	return 0;
}


static char *ata_id_strcpy(u16 *target, u16 *src, int len)
{
	int i;
	for (i = 0; i < len / 2; i++)
		target[i] = le16_to_cpu(src[i]);
	return (char *)target;
}


static void dump_ataid(hd_driveid_t *ataid)
{
	debug("(49)ataid->capability = 0x%x\n", ataid->capability);
	debug("(53)ataid->field_valid =0x%x\n", ataid->field_valid);
	debug("(63)ataid->dma_mword = 0x%x\n", ataid->dma_mword);
	debug("(64)ataid->eide_pio_modes = 0x%x\n", ataid->eide_pio_modes);
	debug("(75)ataid->queue_depth = 0x%x\n", ataid->queue_depth);
	debug("(80)ataid->major_rev_num = 0x%x\n", ataid->major_rev_num);
	debug("(81)ataid->minor_rev_num = 0x%x\n", ataid->minor_rev_num);
	debug("(82)ataid->command_set_1 = 0x%x\n", ataid->command_set_1);
	debug("(83)ataid->command_set_2 = 0x%x\n", ataid->command_set_2);
	debug("(84)ataid->cfsse = 0x%x\n", ataid->cfsse);
	debug("(85)ataid->cfs_enable_1 = 0x%x\n", ataid->cfs_enable_1);
	debug("(86)ataid->cfs_enable_2 = 0x%x\n", ataid->cfs_enable_2);
	debug("(87)ataid->csf_default = 0x%x\n", ataid->csf_default);
	debug("(88)ataid->dma_ultra = 0x%x\n", ataid->dma_ultra);
	debug("(93)ataid->hw_config = 0x%x\n", ataid->hw_config);
}


/*
 * SCSI INQUIRY command operation.
 */
static int ata_scsiop_inquiry(ccb *pccb)
{
	u8 hdr[] = {
		0,
		0,
		0x5,		/* claim SPC-3 version compatibility */
		2,
		95 - 4,
	};
	u8 fis[20];
	u8 *tmpid;
	u8 port;

	/* Clean ccb data buffer */
	memset(pccb->pdata, 0, pccb->datalen);

	memcpy(pccb->pdata, hdr, sizeof(hdr));

	if (pccb->datalen <= 35)
		return 0;

	memset(fis, 0, 20);
	/* Construct the FIS */
	fis[0] = 0x27;		/* Host to device FIS. */
	fis[1] = 1 << 7;	/* Command FIS. */
	fis[2] = ATA_CMD_IDENT;	/* Command byte. */

	/* Read id from sata */
	port = pccb->target;
	if (!(tmpid = malloc(sizeof(hd_driveid_t))))
		return -ENOMEM;

	if (get_ahci_device_data(port, (u8 *) & fis, 20,
				 tmpid, sizeof(hd_driveid_t))) {
		debug("scsi_ahci: SCSI inquiry command failure.\n");
		return -EIO;
	}

	if (ataid[port])
		free(ataid[port]);
	ataid[port] = (hd_driveid_t *) tmpid;

	memcpy(&pccb->pdata[8], "ATA     ", 8);
	ata_id_strcpy((u16 *) &pccb->pdata[16], (u16 *)ataid[port]->model, 16);
	ata_id_strcpy((u16 *) &pccb->pdata[32], (u16 *)ataid[port]->fw_rev, 4);

	dump_ataid(ataid[port]);
	return 0;
}


/*
 * SCSI READ10 command operation.
 */
static int ata_scsiop_read10(ccb * pccb)
{
	u64 lba = 0;
	u32 len = 0;
	u8 fis[20];

	lba = (((u64) pccb->cmd[2]) << 24) | (((u64) pccb->cmd[3]) << 16)
	    | (((u64) pccb->cmd[4]) << 8) | ((u64) pccb->cmd[5]);
	len = (((u32) pccb->cmd[7]) << 8) | ((u32) pccb->cmd[8]);

	/* For 10-byte and 16-byte SCSI R/W commands, transfer
	 * length 0 means transfer 0 block of data.
	 * However, for ATA R/W commands, sector count 0 means
	 * 256 or 65536 sectors, not 0 sectors as in SCSI.
	 *
	 * WARNING: one or two older ATA drives treat 0 as 0...
	 */
	if (!len)
		return 0;
	memset(fis, 0, 20);

//	if (lba & 0xF0000000) {
	if (1) {	// Seagate 1TB HDD has read problem with LBA28 addressing
		/* Construct the FIS */
		debug ("LBA48 addressing\n");
		fis[0] = 0x27;			/* Host to device FIS. */
		fis[1] = 1 << 7;		/* Command FIS. */
		fis[2] = ATA_CMD_RD_DMAX;	/* Command type. */
		fis[3] = len & 0xff;
		fis[11] = (len >> 8) & 0xff;

		/* LBA48 address */
		fis[4] = pccb->cmd[5];
		fis[5] = pccb->cmd[4];
		fis[6] = pccb->cmd[3];
		fis[8] = pccb->cmd[2];

		fis[7] = 3 << 6;

		/* Sector Count */
		fis[12] = pccb->cmd[8];
		fis[13] = pccb->cmd[7];
	} else {
		/* Construct the FIS */
		debug ("LBA28 addressing\n");
		fis[0] = 0x27;		/* Host to device FIS. */
		fis[1] = 1 << 7;	/* Command FIS. */
		fis[2] = ATA_CMD_RD_DMA;	/* Command byte. */
		/* LBA address */
		fis[4] = pccb->cmd[5];
		fis[5] = pccb->cmd[4];
		fis[6] = pccb->cmd[3];
		fis[7] = (pccb->cmd[2] & 0x0f) | 0xe0;

		/* Sector Count */
		fis[12] = pccb->cmd[8];
		fis[13] = pccb->cmd[7];
	}

	/* Read from ahci */
	if (get_ahci_device_data(pccb->target, (u8 *) & fis, 20,
				 pccb->pdata, pccb->datalen)) {
		debug("scsi_ahci: SCSI READ10 command failure.\n");
		return -EIO;
	}

	return 0;
}


/*
 * SCSI READ CAPACITY10 command operation.
 */
static int ata_scsiop_read_capacity10(ccb *pccb)
{
	u8 buf[12];

	if (!ataid[pccb->target]) {
		printf("scsi_ahci: SCSI READ CAPACITY10 command failure. "
		       "\tNo ATA info!\n"
		       "\tPlease run SCSI commmand INQUIRY firstly!\n");
		return -EPERM;
	}

	memset(buf, 0, 12);

	if ((ataid[pccb->target]->command_set_2 & COMMANDSET2_LBA48)) {
		*(u64 *) buf = be64_to_cpu((u64)ataid[pccb->target]->lba48_capacity[2]*0x100000000 +
                                   (u64)ataid[pccb->target]->lba48_capacity[1]*0x10000 + 
                                   (u64)ataid[pccb->target]->lba48_capacity[0]);
	} else {
		*(u32 *) buf = be32_to_cpu(ataid[pccb->target]->lba_capacity);
	}

	buf[10] = 512 >> 8;
	buf[11] = 512 & 0xff;

	memcpy(pccb->pdata, buf, 12);

	return 0;
}


/*
 * SCSI TEST UNIT READY command operation.
 */
static int ata_scsiop_test_unit_ready(ccb *pccb)
{
	return (ataid[pccb->target]) ? 0 : -EPERM;
}


int scsi_exec(ccb *pccb)
{
	int ret;

	switch (pccb->cmd[0]) {
	case SCSI_READ10:
		ret = ata_scsiop_read10(pccb);
		break;
	case SCSI_RD_CAPAC:
		ret = ata_scsiop_read_capacity10(pccb);
		break;
	case SCSI_TST_U_RDY:
		ret = ata_scsiop_test_unit_ready(pccb);
		break;
	case SCSI_INQUIRY:
		ret = ata_scsiop_inquiry(pccb);
		break;
	default:
		printf("Unsupport SCSI command 0x%02x\n", pccb->cmd[0]);
		return FALSE;
	}

	if (ret) {
		debug("SCSI command 0x%02x ret errno %d\n", pccb->cmd[0], ret);
		return FALSE;
	}
	return TRUE;

}


void scsi_low_level_init(int busdevfunc)
{
	int i;
	u32 linkmap;
	u32 u32tmp;

	u32tmp = MISC_SATA_POWER_MODE;
	u32tmp |= 0x1<< 16;  // Disable SATA PHY 0 from SLUMBER Mode
	u32tmp |= 0x1<< 17;  // Disable SATA PHY 1 from SLUMBER Mode 
	MISC_SATA_POWER_MODE = u32tmp;

	/* Enable SATA PHY */
	cns3xxx_pwr_power_up(0x1 << PM_PLL_HM_PD_CTRL_REG_OFFSET_SATA_PHY0);
	cns3xxx_pwr_power_up(0x1 << PM_PLL_HM_PD_CTRL_REG_OFFSET_SATA_PHY1);

	/* Enable SATA Clock */
	cns3xxx_pwr_clk_en(0x1 << PM_CLK_GATE_REG_OFFSET_SATA);

	/* De-Asscer SATA Reset */
	u32tmp = PM_SOFT_RST_REG;
	u32tmp |= 0x1 << PM_SOFT_RST_REG_OFFST_SATA;
	PM_SOFT_RST_REG = u32tmp;

	ahci_init_one(busdevfunc);

	linkmap = probe_ent->link_port_map;

	for (i = 0; i < CFG_SCSI_MAX_SCSI_ID; i++) {
		if (((linkmap >> i) & 0x01)) {
			if (ahci_port_start((u8) i)) {
				printf("Can not start port %d\n", i);
				continue;
			}
			ahci_set_feature((u8) i);
		}
	}
}


void scsi_bus_reset(void)
{
	/*Not implement*/
}


void scsi_print_error(ccb * pccb)
{
	/*The ahci error info can be read in the ahci driver*/
}
