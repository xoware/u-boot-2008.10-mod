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
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
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

#include <common.h>
//#include <malloc.h>
#include <net.h> // for NetReceive()

#include "cns3xxx_gsw.h"
#include "cns3xxx_phy.h"
#include "cns3xxx_symbol.h"
#include "cns3xxx_switch_type.h"
#include "cns3xxx_tool.h"
#if defined(CONFIG_RTK8367) || defined(CONFIG_RTK8367_ONE_LEG)
#include "rtl8367/rtk_api.h"
#include "rtl8367/rtk_api_ext.h"
#include "rtl8367/smi.h"
#endif

#define DISABLE_FC

#if 0
#include <malloc.h>
#include <net.h>
#include <asm/arch/star_powermgt.h>
#include <asm/arch/star_misc.h>
#include <asm/arch/star_intc.h>
#include <asm/arch/star_gsw.h>
#endif
//#include "star_str9100_board.h"

//#define CLOSE_PORT // after stop network function, close every port

#define PORT0_PVID 1
#define PORT1_PVID 2
#define PORT2_PVID 3
#define CPU_PVID 5

#define CPU_CACHE_64BYTES		64
#define CPU_CACHE_64ALIGN(X)	(((X) + (CPU_CACHE_64BYTES-1)) & ~(CPU_CACHE_64BYTES-1))

#define CPU_CACHE_BYTES		32
#define CPU_CACHE_ALIGN(X)	(((X) + (CPU_CACHE_BYTES-1)) & ~(CPU_CACHE_BYTES-1))
//#define PRINT(args...) printf(args)
#define PRINT(args...) 

#define TIMEOUT 50000

u16 tx_pmap[3];
u8 linkup_port[3];
static GswDev gsw_dev;
static u8 cns3xxx_gsw_dev_initialized;

static u8 default_mac_addr[] = {0x08, 0xaa, 0xbb, 0xcc, 0xdd, 0xee};

static RXDesc *rx_desc_ring;
static TXDesc *tx_desc_ring;
static pkt_t *pkt_pool;
static u8 *pkt_buffer_pool;
static u8 mem_alloc_ok;

//static int star_gsw_config_port0_VSC7385(void);
static void cns3xxx_gsw_close(GswDev *dev);

//int disable_AN(int port, int y);

void dump_rx_counter(void)
{
	 printf("C_RXOKPKT_MAC0_REG: %d\n", C_RXOKPKT_MAC0_REG);
	 printf("C_RXOKBYTE_MAC0_REG: %d\n", C_RXOKBYTE_MAC0_REG);
	 printf("C_RXRUNT_MAC0_REG: %d\n", C_RXRUNT_MAC0_REG);
	 printf("C_RXLONG_MAC0_REG: %d\n", C_RXLONG_MAC0_REG);
	 printf("C_RXDROP_MAC0_REG: %d\n", C_RXDROP_MAC0_REG);
	 printf("C_RXCRC_MAC0_REG: %d\n", C_RXCRC_MAC0_REG);
	 printf("C_RXARLDROP_MAC0_REG: %d\n", C_RXARLDROP_MAC0_REG);
	 printf("C_VIDROP_MAC0_REG: %d\n", C_VIDROP_MAC0_REG);
	 printf("C_VEDROP_MAC0_REG: %d\n", C_VEDROP_MAC0_REG);
	 printf("C_RXRL_MAC0_REG: %d\n", C_RXRL_MAC0_REG);
	 printf("C_RXPAUSE_MAC0_REG: %d\n", C_RXPAUSE_MAC0_REG);
}

static void get_mac_addr(GswDev *dev)
{
	int reg=0;
	int env_size=0;
	u8 *e=0, *s=0;
	u8 s_env_mac[64];
	u8 v_env_mac[6];
	u8 *mac_addr;

	//env_size = getenv_r("ethaddr", s_env_mac, sizeof(s_env_mac));

	if (env_size > 0) {
		s = s_env_mac;
		for (reg = 0; reg < 6; reg++) {
			//v_env_mac[reg] = s ? simple_strtoul(s, &e, 16) : 0;
			if (s) {
				s = (*e) ? (e + 1) : e;
			}
		}
		mac_addr = v_env_mac;
	} else {
		mac_addr = default_mac_addr;
	}

	//memcpy(dev->mac, mac_addr, 6);
}


void print_vlan_table_entry(VLANTableEntry *entry)
{
        int i=0;

        printf("vlan_index: %d\n", entry->vlan_index);
        printf("valid: %d\n", entry->valid);
        printf("vid: %d\n", entry->vid);
        printf("wan_side: %d\n", entry->wan_side);
        printf("etag_pmap: 0x%x\n", entry->etag_pmap);
        printf("mb_pmap: 0x%x\n", entry->mb_pmap);

	printf("my_mac addr: %x:%x:%x:%x:%x:%x\n", entry->my_mac[0], entry->my_mac[1],entry->my_mac[2],entry->my_mac[3],entry->my_mac[4],entry->my_mac[5]);
        printf("----------------------------\n");


}

static void cns3xxx_gsw_tx_pkt_enqueue(GswDev *dev, pkt_t *pkt)
{
	if (dev->tx_pkt_q_tail) {
		dev->tx_pkt_q_tail->next = pkt;
	}
	dev->tx_pkt_q_tail = pkt;

	if (dev->tx_pkt_q_head == 0) {
		dev->tx_pkt_q_head = pkt;
	}
	dev->tx_pkt_q_count++;
}

static pkt_t *cns3xxx_gsw_tx_pkt_dequeue(GswDev *dev)
{
	pkt_t *pkt;

	pkt = dev->tx_pkt_q_head;
	if (pkt) {
		dev->tx_pkt_q_head = pkt->next;
		pkt->next = 0;
		if (dev->tx_pkt_q_head == 0) {
			dev->tx_pkt_q_tail = 0;
		}
		dev->tx_pkt_q_count--;
	}

	return pkt;
}

static void cns3xxx_gsw_tx_pkt_requeue(GswDev *dev, pkt_t *pkt)
{
	pkt->next = dev->tx_pkt_q_head;
	dev->tx_pkt_q_head = pkt;
	if (dev->tx_pkt_q_tail == 0) {
		dev->tx_pkt_q_tail = pkt;
	}
	dev->tx_pkt_q_count++;
}

static inline pkt_t *alloc_pkt(GswDev *dev)
{
	pkt_t *pkt=0;

	pkt = dev->free_pkt_list;
	if (pkt) {
		//printf("get pkt\n");
		dev->free_pkt_list = pkt->next;
		pkt->next = 0;
		dev->free_pkt_count--;
	} else {
		//printf("get no pkt\n");
	}

	return pkt;
}

static inline void free_pkt(GswDev *dev, pkt_t *pkt)
{
	pkt->next = dev->free_pkt_list;
	dev->free_pkt_list = pkt;
	dev->free_pkt_count++;
}

static inline pkt_t *rx_alloc_pkt(GswDev *dev)
{
	pkt_t *pkt=0;

	pkt = dev->rx_free_pkt_list;
	if (pkt) {
		//printf("get pkt\n");
		dev->rx_free_pkt_list = pkt->next;
		pkt->next = 0;
		dev->rx_free_pkt_count--;
	} else {
		//printf("get no pkt\n");
	}

	return pkt;
}

static inline void rx_free_pkt(GswDev *dev, pkt_t *pkt)
{
	pkt->next = dev->rx_free_pkt_list;
	dev->rx_free_pkt_list = pkt;
	dev->rx_free_pkt_count++;
}

#if 0
int setup_all_rx_resources()
{
	int i=0;

	rx_desc_ring = (RXDesc *)malloc(sizeof(RXDesc) * RX_SIZE + CPU_CACHE_BYTES);

	if (!rx_desc_ring)
		return CAVM_ERR;

	for (i = 0; i < RX_SIZE; i++) {
		rx_desc_ring[i].sdp = (u32)gsw_dev.rx_ring_pkt[i]->pkt_buffer;
		rx_desc_ring[i].cown = 0;
		rx_desc_ring[i].lsd = 1;
		rx_desc_ring[i].fsd = 1;
	}
	rx_desc_ring[RX_SIZE-1].eor = 1;

	FS_DESC_PTR0_REG = rx_desc_ring;
	FS_DESC_BASE_ADDR0_REG = rx_desc_ring;
	FS_DMA_CTRL0_REG = 0; // stop FS DMA

	return CAVM_OK;
}

int setup_all_tx_resources(void)
{
	tx_desc_ring = (TXDesc *)malloc(sizeof(TXDesc) * TX_SIZE + CPU_CACHE_BYTES);

	if (!tx_desc_ring)
		return CAVM_ERR;

	TS_DESC_PTR0_REG = tx_desc_ring;
	TS_DESC_BASE_ADDR0_REG = tx_desc_ring;
	TS_DMA_CTRL0_REG = 0; // stop FS DMA

	return CAVM_OK;
}
#endif

static void free_mem(void)
{
	if (tx_desc_ring)
		free(tx_desc_ring);
	if (rx_desc_ring)
		free(rx_desc_ring);
	if (pkt_pool)
		free(pkt_pool);
	if (pkt_buffer_pool)
		free(pkt_buffer_pool);
}

int setup_pkt_pool(void)
{
	pkt_pool = (pkt_t *)malloc(sizeof(pkt_t) * NUM_PKT_BUFFER);
	if (pkt_pool == 0) {
		goto err_out;
	}

	pkt_buffer_pool = (u8 *)malloc(PKT_BUFFER_ALLOC_SIZE * NUM_PKT_BUFFER);
	if (pkt_buffer_pool == 0) {
		goto err_out;
	}

	return CAVM_OK;
err_out:
	free_mem();
	return CAVM_ERR;
}


static int cns3xxx_alloc_mem(GswDev *dev)
{
	
	tx_desc_ring = (TXDesc *)malloc(sizeof(TXDesc) * TX_SIZE + CPU_CACHE_BYTES);
	if (tx_desc_ring == 0) {
		goto err_out;
	}

	rx_desc_ring = (RXDesc *)malloc(sizeof(RXDesc) * RX_SIZE + CPU_CACHE_BYTES);
	if (rx_desc_ring == 0) {
		goto err_out;
	}

	pkt_pool = (pkt_t *)malloc(sizeof(pkt_t) * NUM_PKT_BUFFER);
	if (pkt_pool == 0) {
		goto err_out;
	}

	pkt_buffer_pool = (u8 *)malloc(PKT_BUFFER_ALLOC_SIZE * NUM_PKT_BUFFER);
	if (pkt_buffer_pool == 0) {
		goto err_out;
	}

	dev->rx_pkt_pool = (pkt_t *)malloc(sizeof(pkt_t) * NUM_PKT_BUFFER);
	if (dev->rx_pkt_pool == 0) {
		goto err_out;
	}

	dev->rx_pkt_buffer_pool = (u8 *)malloc(PKT_BUFFER_ALLOC_SIZE * NUM_PKT_BUFFER);
	dev->rx_pkt_buffer_pool = (u8 *)CPU_CACHE_64ALIGN((u32)dev->rx_pkt_buffer_pool);

	if (dev->rx_pkt_buffer_pool == 0) {
		goto err_out;
	}



	mem_alloc_ok = 1;

	return CAVM_OK;

err_out:
	printf("alloc memory fail\n");
	free_mem();

	return CAVM_ERR;
}

int cns3xxx_init_mem(GswDev *dev)
{
	int i;

	dev->tx_ring = (TXDesc *)CPU_CACHE_ALIGN((u32)tx_desc_ring);
	dev->rx_ring = (RXDesc *)CPU_CACHE_ALIGN((u32)rx_desc_ring);
	memset(dev->tx_ring, 0, sizeof(TXDesc) * TX_SIZE);
	memset(dev->rx_ring, 0, sizeof(RXDesc) * RX_SIZE);
	dev->tx_ring[TX_SIZE - 1].eor = 1;
	dev->rx_ring[RX_SIZE - 1].eor = 1;

	memset(dev->tx_ring_pkt, 0, sizeof(pkt_t *) * TX_SIZE);
	memset(dev->rx_ring_pkt, 0, sizeof(pkt_t *) * RX_SIZE);

	dev->pkt_pool = pkt_pool;
	memset(dev->pkt_pool, 0, sizeof(pkt_t) * NUM_PKT_BUFFER);

	dev->pkt_buffer_pool = pkt_buffer_pool;


	dev->free_pkt_list = &dev->pkt_pool[0];

	dev->rx_free_pkt_list = &dev->rx_pkt_pool[0];

	for (i = 0; i < (NUM_PKT_BUFFER - 1); i++) {
		dev->pkt_pool[i].next = &dev->pkt_pool[i + 1];
		dev->pkt_pool[i].pkt_buffer = dev->pkt_buffer_pool + (i * PKT_BUFFER_ALLOC_SIZE);

		dev->rx_pkt_pool[i].next = &dev->rx_pkt_pool[i + 1];
		dev->rx_pkt_pool[i].pkt_buffer = dev->rx_pkt_buffer_pool + (i * PKT_BUFFER_ALLOC_SIZE);
		//printf("dev->rx_pkt_pool[%d].pkt_buffer: %x\n", i, dev->rx_pkt_pool[i].pkt_buffer);

	}
	dev->pkt_pool[i].next = 0;
	dev->pkt_pool[i].pkt_buffer = dev->pkt_buffer_pool + (i * PKT_BUFFER_ALLOC_SIZE);

	dev->rx_pkt_pool[i].next = 0;
	dev->rx_pkt_pool[i].pkt_buffer = dev->rx_pkt_buffer_pool + (i * PKT_BUFFER_ALLOC_SIZE);

	for (i = 0; i < TX_SIZE; i++) {
		dev->tx_ring[i].cown = 1;
		dev->tx_ring[i].ico = 0;
		dev->tx_ring[i].uco = 0;
		dev->tx_ring[i].tco = 0;
	}
	
	//printf("init rx alloc pkt\n");
	for (i = 0; i < RX_SIZE; i++) {
		dev->rx_ring_pkt[i] = rx_alloc_pkt(dev);
		dev->rx_ring[i].sdp = (u32)dev->rx_ring_pkt[i]->pkt_buffer;
		dev->rx_ring[i].sdl = PKT_BUFFER_SIZE;
		dev->rx_ring[i].cown = 0;
	}

	dev->tx_pkt_q_head = 0;
	dev->tx_pkt_q_tail = 0;

	dev->cur_tx_desc_idx	= 0;
	dev->cur_rx_desc_idx	= 0;
	dev->tx_pkt_count	= 0;
	dev->rx_pkt_count	= 0;
	dev->free_pkt_count	= NUM_PKT_BUFFER;
	dev->tx_pkt_q_count	= 0;

#ifdef STAR_GSW_DEBUG
	printf("TX Descriptor DMA Start:	0x%08x\n", dev->tx_desc_pool_dma);
	printf("RX Descriptor DMA Start:	0x%08x\n", dev->rx_desc_pool_dma);
#if 0
	for (i = 0; i < NUM_PKT_BUFFER; i++) {
		printf("PKT Buffer[%03d]:	0x%08x\n", i, dev->pkt_pool[i].pkt_buffer);
	}
#endif
#endif


	return 0;
}

#if 0
inline void cns3xxx_gsw_power_enable(void)
{
	PLL_HM_PD_CTRL_REG &= (~(1 << 1)); // power up PLL_RGMII (for MAC)
	CLK_GATE_REG |= (1 << 11); // enable switch clock
}

inline void cns3xxx_gsw_software_reset(void)
{
        SOFT_RST_REG &= (~(1 << 11));
        SOFT_RST_REG |= (1 << 11);
}

// which : 0 or 1
// enable: 0 or 1
int enable_rx_dma(u8 which, u8 enable)
{
	if (which == 0 ) {
		FS_DMA_CTRL0_REG = enable;
	} else if (which == 1 ) {
		FS_DMA_CTRL1_REG = enable;
	} else {
		return CAVM_ERR;
	}
	return CAVM_OK;
}

// which : 0 or 1
// enable: 0 or 1
int enable_tx_dma(u8 which, u8 enable)
{
	if (which == 0 ) {
		//printf("TS_DMA_CTRL0_REG : %x ## enable: %d \n", TS_DMA_CTRL0_REG, enable);
		TS_DMA_CTRL0_REG = enable;
		//printf("xxx TS_DMA_CTRL0_REG : %x ## enable: %d \n", TS_DMA_CTRL0_REG, enable);
	} else if (which == 1 ) {
		TS_DMA_CTRL1_REG = enable;
	} else {
		return CAVM_ERR;
	}
	return CAVM_OK;
}
#endif



static int cns3xxx_hw_init(GswDev *dev)
{
	u32 reg_config;
	int i, j;

static u8 my_vlan0_mac[6] = {0x00, 0x01, 0x22, 0x33, 0x44, 0x99};
static u8 my_vlan1_mac[6] = {0x00, 0x01, 0xbb, 0xcc, 0xdd, 0x60};
static u8 my_vlan2_mac[6] = {0x00, 0x01, 0xbb, 0xcc, 0xdd, 0x22};
static u8 my_vlan3_mac[6] = {0x00, 0x01, 0xbb, 0xcc, 0xdd, 0x55};

static VLANTableEntry cpu_vlan_table_entry = {0, 1, CPU_PVID, 0, 0, MAC_PORT0_PMAP | MAC_PORT1_PMAP | MAC_PORT2_PMAP | CPU_PORT_PMAP, my_vlan3_mac}; // for cpu


static VLANTableEntry vlan_table_entry[] =
{
                                                 // vlan_index; valid; vid; wan_side; etag_pmap; mb_pmap; *my_mac;
                                                {1, 1, PORT0_PVID, 0, 0, MAC_PORT0_PMAP | MAC_PORT2_PMAP | CPU_PORT_PMAP, my_vlan0_mac},
                                                {2, 1, PORT1_PVID, 1, 0, MAC_PORT1_PMAP | MAC_PORT2_PMAP  | CPU_PORT_PMAP, my_vlan1_mac},
                                                {3, 1, PORT2_PVID, 1, 0, MAC_PORT2_PMAP  | CPU_PORT_PMAP, my_vlan2_mac},
};

static ARLTableEntry arl_table_entry[] =
{
                                             // vid; pmap; *mac; age_field; vlan_mac ; filter

                                                {1, MAC_PORT0_PMAP | CPU_PORT_PMAP, my_vlan0_mac, 7, 1, 1},
                                                {2, MAC_PORT1_PMAP | CPU_PORT_PMAP, my_vlan0_mac, 7, 1, 1},
                                                {3, MAC_PORT2_PMAP | CPU_PORT_PMAP, my_vlan0_mac, 7, 1, 1},
};

#ifndef CONFIG_FPGA

#if defined(CONFIG_RTK8367) || defined(CONFIG_RTK8367_ONE_LEG)
#else
printf("GPIOB value before enabling MDIO: 0x%x\n",  GPIOB_PIN_EN_REG); 

        GPIOB_PIN_EN_REG |= (1 << 14); //enable GMII2_CRS
        GPIOB_PIN_EN_REG |= (1 << 15); //enable GMII2_COL
        GPIOB_PIN_EN_REG |= (1 << 20); //enable MDC
        GPIOB_PIN_EN_REG |= (1 << 21); //enable MDIO
printf("GPIOB value after enabling MDIO: 0x%x\n",  GPIOB_PIN_EN_REG); 
#endif

	/* enable the gsw */
	cns3xxx_gsw_power_enable();

	/* software reset the gsw */
	cns3xxx_gsw_software_reset();
#endif

#ifdef CONFIG_FPGA
	printf("FPGA turns on GSW_SIM_MODE_BIT\n");
	SLK_SKEW_CTRL_REG |= (1 << 31);
#endif

	while (1) {
		if (((SRAM_TEST_REG >> 20) & 1) == 1) {
			//printf("cns3xxx gsw memory test is complete\n");
			break;
		}
	}

	// stop ring0, ring1 TX/RX DMA
	enable_rx_dma(0, 0);
	enable_tx_dma(0, 0);
	enable_rx_dma(1, 0);
	enable_tx_dma(1, 0);

        //cns3xxx_phy_auto_polling_disable(1);


#if 0
	// ref: non-os code
	FC_GLOB_THRS_REG = 0;
        MC_GLOB_THRS_REG = 0;
	FC_GLOB_THRS_REG |= ((0x48)<<16);
        FC_GLOB_THRS_REG |= 0x40;
        MC_GLOB_THRS_REG |= ((0x02)<<8);
        MC_GLOB_THRS_REG |= 0x04;
#endif




        cns3xxx_vlan_table_add(&cpu_vlan_table_entry);
	//printf("setting vlan table\n");
	for (i=0 ; i < (sizeof(vlan_table_entry)/sizeof(VLANTableEntry)) ; ++i) {
		//print_vlan_table_entry(&vlan_table_entry[i]);
		cns3xxx_vlan_table_add(&vlan_table_entry[i]);
	}

#if 0
// read vlan table for debug vlan table.
	printf("  read vlan talbe\n");
	for (i=0 ; i < (sizeof(vlan_table_entry)/sizeof(VLANTableEntry)) ; ++i){	
		VLANTableEntry vtable;

		vtable.vlan_index=i;
		cns3xxx_vlan_table_read(&vtable);
		print_vlan_table_entry(&vtable);

	}
	//return CAVM_ERR;
#endif

        cns3xxx_set_pvid(0, PORT0_PVID);
        cns3xxx_set_pvid(1, PORT1_PVID);
        cns3xxx_set_pvid(2, PORT2_PVID);
        cns3xxx_set_pvid(3, CPU_PVID);

	get_mac_addr(dev);


	/* configure DMA descriptors */
	FS_DESC_PTR0_REG = (u32)dev->rx_ring;
	FS_DESC_BASE_ADDR0_REG = (u32)dev->rx_ring;

	TS_DESC_PTR0_REG = (u32)dev->tx_ring;
	TS_DESC_BASE_ADDR0_REG = (u32)dev->tx_ring;


	// configure MAC global
	reg_config = MAC_GLOB_CFG_REG;

	/* disable aging */
	reg_config &= ~0xf;


	// disable IVL, use SVL
	reg_config &= ~(1 << 7);

#if 1
	// use IVL
	reg_config |= (1 << 7);
#endif

	// disable HNAT
	reg_config &= ~(1 << 26);

	// receive CRC error frame
	reg_config |= (1 << 21);

	MAC_GLOB_CFG_REG = reg_config;
	
	// max packet len 1536
	reg_config = PHY_AUTO_ADDR_REG;
	reg_config &= ~(3 << 30);
	reg_config |= (2 << 30);
	PHY_AUTO_ADDR_REG = reg_config;


	/* configure cpu port */
	reg_config = CPU_CFG_REG;

	// disable CPU port SA learning
	reg_config |= (1 << 19);

	// DMA 4N mode
	reg_config |= (1 << 30);

	CPU_CFG_REG = reg_config;

	return CAVM_OK;
}

static void cns3xxx_gsw_halt(GswDev *dev)
{
#ifdef USE_MAC0
	enable_port(0, 0);
#endif
#ifdef USE_MAC1
	enable_port(1, 0);
#endif
#ifdef USE_MAC2
	enable_port(2, 0);
#endif
	enable_port(3, 0);
#if 1
	//printf("PORT0_LINK_DOWN\n");
	PORT0_LINK_DOWN
	PORT1_LINK_DOWN
	PORT2_LINK_DOWN
#endif
#if 0
	// disable all interrupt status sources
	HAL_GSW_DISABLE_ALL_INTERRUPT_STATUS_SOURCES();

	// clear previous interrupt sources
	HAL_GSW_CLEAR_ALL_INTERRUPT_STATUS_SOURCES();

	// disable all DMA-related interrupt sources
	HAL_INTC_DISABLE_INTERRUPT_SOURCE(INTC_GSW_TSTC_BIT_INDEX);
	HAL_INTC_DISABLE_INTERRUPT_SOURCE(INTC_GSW_FSRC_BIT_INDEX);
	HAL_INTC_DISABLE_INTERRUPT_SOURCE(INTC_GSW_TSQE_BIT_INDEX);
	HAL_INTC_DISABLE_INTERRUPT_SOURCE(INTC_GSW_FSQF_BIT_INDEX);

	// clear previous interrupt sources
	HAL_INTC_CLEAR_EDGE_TRIGGER_INTERRUPT(INTC_GSW_TSTC_BIT_INDEX);
	HAL_INTC_CLEAR_EDGE_TRIGGER_INTERRUPT(INTC_GSW_FSRC_BIT_INDEX);
	HAL_INTC_CLEAR_EDGE_TRIGGER_INTERRUPT(INTC_GSW_TSQE_BIT_INDEX);
	HAL_INTC_CLEAR_EDGE_TRIGGER_INTERRUPT(INTC_GSW_FSQF_BIT_INDEX);

	// stop all DMA operation
	HAL_GSW_TS_DMA_STOP();
	HAL_GSW_FS_DMA_STOP();

	// disable CPU port, GSW MAC port 0 and MAC port 1
	GSW_MAC_PORT_0_CONFIG_REG |= (0x1 << 18);
	GSW_MAC_PORT_1_CONFIG_REG |= (0x1 << 18);
	GSW_CPU_PORT_CONFIG_REG |= (0x1 << 18);

	// software reset the gsw
	PWRMGT_SOFTWARE_RESET_CONTROL_REG |= (0x1 << PWRMGT_GSW_SOFTWARE_RESET_MASK_BIT_INDEX);
	udelay(10);
	PWRMGT_SOFTWARE_RESET_CONTROL_REG &= ~(0x1 << PWRMGT_GSW_SOFTWARE_RESET_MASK_BIT_INDEX);
	udelay(10);
	PWRMGT_SOFTWARE_RESET_CONTROL_REG |= (0x1 << PWRMGT_GSW_SOFTWARE_RESET_MASK_BIT_INDEX);

	/* wait until all embedded memory BIST is complete */
	while (!(GSW_BIST_RESULT_TEST_0_REG & (0x1 << 17))) {
		udelay(100);
	}
#endif
}

void cns3xxx_gsw_close(GswDev *dev)
{
	cns3xxx_gsw_halt(dev);
}

static int cns3xxx_gsw_init(GswDev *dev)
{
	int err;


	// init tx pmap array
	tx_pmap[0] = 1; // port 0
	tx_pmap[1] = (1 << 1); // port 1
	tx_pmap[2] = (1 << 4); // port 2


	if (!mem_alloc_ok) {
		err = cns3xxx_alloc_mem(dev);
		if (err) {
			return err;
		}
	}

	cns3xxx_init_mem(dev);

	
	err = cns3xxx_hw_init(dev);

        // GIGA mode off
        MAC0_CFG_REG &= (~(1<<16));
        MAC1_CFG_REG &= (~(1<<16));
        MAC2_CFG_REG &= (~(1<<16));

#ifndef CONFIG_FPGA
        // GIGA mode off
        MAC0_CFG_REG |= (1<<16);
        MAC1_CFG_REG |= (1<<16);
        MAC2_CFG_REG |= (1<<16);
#endif


	if (err != CAVM_OK) {
		printf("cns3xxx_hw_init fail\n");
		cns3xxx_gsw_halt(dev);
		memset(dev, 0, sizeof(GswDev));
		return err;
	}

	return CAVM_OK;
}

#define TIMEOUT_WAIT
static void cns3xxx_gsw_open(GswDev *dev)
{
	u16 phy_data;
	int i=0;
	unsigned char *which_port=0;

#if 0
	cns3xxx_phy_auto_polling_enable(0, 0);
	cns3xxx_phy_auto_polling_enable(1, 0);
	cns3xxx_phy_auto_polling_enable(2, 0);
#endif

	// disable all ports
	enable_port(0, 0);
	enable_port(1, 0);
	enable_port(2, 0);
	enable_port(3, 0);

	//printf("cns3xxx_gsw_open\n");
	//static int init_phy=0;

	// config the phy
	INIT_PORT0_PHY 
	INIT_PORT1_PHY
	INIT_PORT2_PHY

	//INIT_PORT2_MAC
	
	PORT0_LINK_UP
	PORT1_LINK_UP
	PORT2_LINK_UP

#ifdef DISABLE_FC
	// disable all ports flow control function
	MAC0_CFG_REG &= (~(1 << 12));
	MAC0_CFG_REG &= (~(1 << 11));
	MAC1_CFG_REG &= (~(1 << 12));
	MAC1_CFG_REG &= (~(1 << 11));
	MAC2_CFG_REG &= (~(1 << 12));
	MAC2_CFG_REG &= (~(1 << 11));
#endif

	// start tx/rx DMA operation
	//enable_tx_dma(0, 1);
	enable_rx_dma(0, 1);

#ifdef USE_MAC0
	enable_port(0, 1);
#endif
#ifdef USE_MAC1
	enable_port(1, 1);
#endif
#ifdef USE_MAC2
	enable_port(2, 1);
#endif
	enable_port(3, 1);

	which_port = getenv("port");

	if (which_port == 0 || ( !((which_port[0] - '0') >= 0) && ((which_port[0] - '0') <= 2)) ) {
		dev->which_port = -1; // don't specify port variable, or not 0, 1, 2 port
	} else {
		dev->which_port = (which_port[0] - '0'); // 0, 1 , 2 port
	}

	// search which port link up
	for (i=0 ; i < MAC_NUM; ++i){
		u32 time_out=0;
		u32 mac_cfg=0;

		// check link up
		do {
			const u8 mac_addr[]={0x0c, 0x10, 0x18};
			//cns3xxx_read_phy(i, 1 , &phy_data);
			mac_cfg=SWITCH_REG_VALUE(mac_addr[i]);
			++time_out;
#ifdef TIMEOUT_WAIT
			if ( time_out > TIMEOUT) 
				break;
#endif
	                if (ctrlc()) 
				break;


		} while ((mac_cfg & 1) == 0);

		if (time_out <= TIMEOUT) {
			linkup_port[i] = 1;
			//printk("port %d : link up\n", i);
		} else {
			linkup_port[i] = 0;
			//printk("port %d : link down\n", i);
		}
		//printk("time_out: %d\n", time_out);

	} // end for (i=0 ; i < 3; ++i)

#if 0
	for (i=0 ; i < 3; ++i){
		printf("linkup_port[%d]: %d\n", i, linkup_port[i]);
	}
#endif
}

#ifdef CONFIG_RTK8367 
void init_rtk8367()
{
		cns3xxx_mdc_mdio_disable(1);
    	PMU_REG_VALUE(0x0) |= (0x1<<10);    /* enable GPIO */
	    PMU_REG_VALUE(0x4) &= ~(0x1<<10);   /* reset GPIO */
	    PMU_REG_VALUE(0x4) |= (0x1<<10);
    	MISC_REG_VALUE(0x18) &= ~(0x3<<20); /* configure GPIOB*/

		if (0 != rtk_switch_init())  {
			printk("rtk_switch_init fail\n");
			return;
		} else {
			rtk_port_mac_ability_t ability;
			rtk_data_t mode;
			#if 1
			rtk_portmask_t portmask;
			portmask.bits[0] = 0x1F;
			rtk_led_enable_set(LED_GROUP_0, portmask);
			rtk_led_enable_set(LED_GROUP_1, portmask);
			rtk_cpu_enable_set(ENABLE);
			rtk_cpu_tagPort_set(RTK_EXT_1_MAC, CPU_INSERT_TO_ALL);
			rtk_cpu_tagPort_set(RTK_EXT_0_MAC, CPU_INSERT_TO_ALL);
			rtk_vlan_init();
    ability.forcemode = 1;
    ability.speed = 2;
    ability.duplex = 1;
    ability.link = 1;    
    ability.nway = 0;    
    ability.txpause = 1;
    ability.rxpause = 1;   
#if 1
			//rtk_port_macForceLinkExt0_set(MODE_EXT_RGMII, &ability);
			rtk_port_macForceLinkExt1_set(MODE_EXT_RGMII, &ability);
#else
			//rtk_port_macForceLinkExt0_set(MODE_EXT_RGMII_33V, &ability);
			rtk_port_macForceLinkExt1_set(MODE_EXT_RGMII_33V, &ability);
#endif
#if 0
{
		u32 rData, mAddr;
		mAddr = 0x1305;
		if (0 == smi_read(mAddr, &rData)) {
			printk("[0x%.8x] => 0x%.8x\n", mAddr, rData);
		} else {
			printk("[0x%.8x] => fail\n", mAddr);
		}
		
		
		mAddr = 0x1310;
		if (0 == smi_read(mAddr, &rData)) {
			printk("[0x%.8x] => 0x%.8x\n", mAddr, rData);
		} else {
			printk("[0x%.8x] => fail\n", mAddr);
		}

		mAddr = 0x1311;
		if (0 == smi_read(mAddr, &rData)) {
			printk("[0x%.8x] => 0x%.8x\n", mAddr, rData);
		} else {
			printk("[0x%.8x] => fail\n", mAddr);
		}
		
		
		if (0 == rtk_port_macForceLinkExt1_get(&mode, &ability)) {
			printk("mode %d\n", mode);
			printk("forcemode %d spped %d duplex %d link %d nway %d txpause %d rxpause %d\n",
    			ability.forcemode, ability.speed, ability.duplex, 
			    ability.link, ability.nway, ability.txpause, ability.rxpause);   
		};
}
#endif
		
			#endif
		}
}
#endif

#ifdef CONFIG_RTK8367_ONE_LEG
void init_rtk8367()
{
	printf("init_rtk8367 one leg\n");
		cns3xxx_mdc_mdio_disable(1);
    	//PMU_REG_VALUE(0x0) |= (0x1<<10);    /* enable GPIO */
	    //PMU_REG_VALUE(0x4) &= ~(0x1<<10);   /* reset GPIO */
	    //PMU_REG_VALUE(0x4) |= (0x1<<10);
    	MISC_REG_VALUE(0x18) &= ~(0x3<<20); /* configure GPIOB*/

		smi_init(0,0,0);  //cypress 110324
		//smi_init();
		smi_reset(0,0);  //cypress 110324
		//smi_reset();

		if (0 != rtk_switch_init())  {
			printk("rtk_switch_init fail\n");
			return;
		} 
		else 
		{
			rtk_port_mac_ability_t ability;
			rtk_data_t mode;

			#if 1

				/*								//cypress 110324, comment them all
				rtk_portmask_t portmask;
				portmask.bits[0] = 0x1F;
				rtk_led_enable_set(LED_GROUP_0, portmask);
				rtk_led_enable_set(LED_GROUP_1, portmask);

				rtk_cpu_enable_set(ENABLE);
				//rtk_cpu_tagPort_set(RTK_EXT_1_MAC, CPU_INSERT_TO_ALL);
				rtk_cpu_tagPort_set(RTK_EXT_0_MAC, CPU_INSERT_TO_ALL);

				rtk_vlan_init();
				*/

			    ability.forcemode = 1;
			    ability.speed = 2;				// 2: GIGA , 1: 100M
			    ability.duplex = 1;
			    ability.link = 1;    
			    ability.nway = 0;    
			    ability.txpause = 1;
			    ability.rxpause = 1;   

				#if 1
					rtk_port_macForceLinkExt0_set(MODE_EXT_RGMII, &ability);
					//rtk_port_macForceLinkExt1_set(MODE_EXT_RGMII, &ability);
				#else
					rtk_port_macForceLinkExt0_set(MODE_EXT_RGMII_33V, &ability);
					//rtk_port_macForceLinkExt1_set(MODE_EXT_RGMII_33V, &ability);
				#endif

                rtk_port_rgmiiDelayExt0_set(1, 4);				//cypress 110324
			#endif
		}
}
#endif

int eth_init(bd_t *bis)
{
	GswDev *dev = &gsw_dev;
	int err = 0;

	if (cns3xxx_gsw_dev_initialized == 0) {

#if defined(CONFIG_RTK8367) || defined(CONFIG_RTK8367_ONE_LEG)
	        init_rtk8367();
#endif
#ifdef CONFIG_RTK8367_ONE_LEG
	SWITCH_REG_VALUE(0xf0) |= (0x0<<4); // rx               //cypress 110324
	SWITCH_REG_VALUE(0xf0) |= (0x0<<6); // tx               //cypress 110324

#endif

		if (cns3xxx_gsw_init(dev) == CAVM_OK) {
#ifndef CLOSE_PORT 
			cns3xxx_gsw_open(dev);
#endif
			cns3xxx_gsw_dev_initialized = 1;
		}
		else {
			err = -1;
		}
	} 
#ifdef CLOSE_PORT 
	cns3xxx_gsw_open(dev);
#endif
	return err;
}



static void cns3xxx_gsw_tx(GswDev *dev)
{
	TXDesc volatile *tx_desc = 0;
	pkt_t *pkt = 0;
	u32 txcount = 0;
	int i=0;

	while ((pkt = cns3xxx_gsw_tx_pkt_dequeue(dev))) {
	#if 0
		printf("  tx pkt : %p ## ", pkt);
		printf("tx pkt->pkt_buffer : %p\n", pkt->pkt_buffer);
		#endif
		tx_desc = &dev->tx_ring[dev->cur_tx_desc_idx];
		#if 0
		printf("dev->cur_tx_desc_idx: %d\n", dev->cur_tx_desc_idx);
		printf("tx_desc : %p\n", tx_desc);
		#endif
		if (!tx_desc->cown) {
			cns3xxx_gsw_tx_pkt_requeue(dev, pkt);
			break;
		} else {
			if (dev->tx_ring_pkt[dev->cur_tx_desc_idx]) {
				free_pkt(dev, dev->tx_ring_pkt[dev->cur_tx_desc_idx]);
			}
		}
#ifdef STAR_GSW_DEBUG
		printf("TX PKT buffer: 0x%08x\n", pkt->pkt_buffer);
		printf("TX PKT length: %d\n", pkt->length);
#endif

#if 0
                /* fill 0 to MIN_PACKET_LEN size */
                if (sg==0 && len < MIN_PACKET_LEN) {
                        tx_desc_ptr->sdl = MIN_PACKET_LEN;
                        memset(skb->data + len, 0, MIN_PACKET_LEN - len);
                } else {
                        tx_desc_ptr->sdl = len;
                }
#endif

		//printf("tx dev->cur_tx_desc_idx: %d\n", dev->cur_tx_desc_idx);
		dev->tx_ring_pkt[dev->cur_tx_desc_idx] = pkt;

#if 0
		{
			int i=0;
		for (i=0 ; i < 6 ; ++i)
			pkt->pkt_buffer[i]=0xff;
		}
#endif

		tx_desc->sdp = (u32)pkt->pkt_buffer;

#if 0
// do software appending
		if (pkt->length < 64) 
			tx_desc->sdl = 64;
		else
#endif
			tx_desc->sdl = pkt->length;

#if 0
		printf("tx\n");
		print_packet(tx_desc->sdp, tx_desc->sdl);
#endif
		tx_desc->cown = 0;
		tx_desc->fsd = 1;
		tx_desc->lsd = 1;
		tx_desc->fr = 1;
#ifdef USE_PORT0_VSC8601
		tx_desc->pmap = 1; // port 0
#endif

#ifdef USE_PORT1_VSC8601
		tx_desc->pmap = (1 << 1); // port 1
#endif

#ifdef USE_PORT2_101A
		tx_desc->pmap = (1 << 4); // port 2
#endif

	//which_port = getenv("port");
	//printf("port: %s\n", which_port);
	if (dev->which_port == -1) {
		//printf("auto probe\n");
		// auto probe
		for (i=0 ; i < 3 ; ++i) {
			if (linkup_port[i] == 1) {
				tx_desc->pmap = tx_pmap[i];
				break;
			}
		}
	} else {
		//printf("assign\n");
		tx_desc->pmap = tx_pmap[dev->which_port];
	}


#if 0
		printf("tx\n");
		print_packet(pkt->pkt_buffer, tx_desc->sdl);
#endif

		// point to next index

		//dev->cur_tx_desc_idx = ((dev->cur_tx_desc_idx + 2) % TX_SIZE);
		dev->cur_tx_desc_idx = ((dev->cur_tx_desc_idx + 1) % TX_SIZE);

#if 0
		if (dev->cur_tx_desc_idx == (TX_SIZE - 1)) {
			dev->cur_tx_desc_idx = 0;
		} else {
			dev->cur_tx_desc_idx++;
		}
#endif

		txcount++;
	}
	//printf("tx_desc addr: %p\n", tx_desc);
	//dump_tx_desc(tx_desc);

	dev->tx_pkt_count += txcount;

	//printf("TS_DESC_BASE_ADDR0_REG: %x\n", TS_DESC_BASE_ADDR0_REG);
	//dump_tx_desc(tx_desc);

#if 0
	TS_DMA_CTRL0_REG = 1;

	{
		int delay=0;

	        for (delay=0; delay<10; delay++);

	}
#endif
	enable_tx_dma(0, 1);
}

s32 eth_send(volatile void *packet, s32 length)
{
	GswDev *dev = &gsw_dev;

	pkt_t *pkt;
	u16 phy_data;

	//printf("tx alloc pkt\n");
	pkt = alloc_pkt(dev);
	if (!pkt) {
		printf("Allocate pkt failed on TX...\n");
		return 0;
	}

	memcpy(pkt->pkt_buffer, (void *)packet, length);
	if (length < PKT_MIN_SIZE) {
		pkt->length = PKT_MIN_SIZE;
		memset(pkt->pkt_buffer + length, 0x00, PKT_MIN_SIZE - length);
	} else {
		pkt->length = length;
	}
	cns3xxx_gsw_tx_pkt_enqueue(dev, pkt);
	cns3xxx_gsw_tx(dev);

	return 0;
}

u32 get_rx_hw_index()
{
	return (FS_DESC_PTR0_REG - FS_DESC_BASE_ADDR0_REG)/32;
}

// hw_index, sw_index begin from 0
int get_rx_count(u32 hw_index, u32 sw_index, u8 cown)
{
	if (hw_index > sw_index)
		return (hw_index - sw_index);
	else if (hw_index < sw_index)
		return ((RX_SIZE) - sw_index + hw_index + 1);
	else 
		if (cown == 0) { // this should not happen
			return -1;
		} else {
			return (RX_SIZE);
		}
}


void dump_rx_ring(u32 begin_index, u32 end_index)
{
}

static void cns3xxx_gsw_rx(GswDev *dev)
{
	RXDesc volatile *rx_desc=0;
	pkt_t *rcvpkt;
	pkt_t *newpkt;
	u32 rxcount = 0;
	int i=0, flag=0;

        //printf("8601 crc counter: %d\n", get_crc_good_counter(0));
        #if 0
        printf("8601 crc good counter: %d\n", get_crc_good_counter(0));
        printf("8601 recv err counter: %d\n", get_vsc8601_recv_err_counter(0));
	dump_rx_counter();
	#endif
	
	while (1) {
		u32 phy_counter=0;
		u32 hw_index = get_rx_hw_index();
		int rx_count=0, i=0;

		//phy_counter = get_crc_good_counter(0);
		//printf("phy_counter: %d\n", phy_counter);
		PRINT("  rx dev->cur_rx_desc_idx: %d\n", dev->cur_rx_desc_idx);
		//printf("  rx dev->cur_rx_desc_idx: %d\n", dev->cur_rx_desc_idx);
		rx_desc = &dev->rx_ring[dev->cur_rx_desc_idx];
		//printf("  rx_desc: %p\n", rx_desc);
		//printf("  rx_desc->cown: %d\n", rx_desc->cown);
	#if 0
	{
	u16 phy_data;

	cns3xxx_read_phy(0, 1 , &phy_data);
	printf("gsw_rx phy register 1 : %x\n", phy_data);
	}
	#endif

		PRINT("  rx hw index: %d\n", hw_index);
		rx_count = get_rx_count(hw_index, dev->cur_rx_desc_idx, rx_desc->cown);
		#if 0
		PRINT("  rx count: %d\n", rx_count);
		for (i=0 ; i < rx_count ; ++i) {
			RXDesc volatile *r_desc = &dev->rx_ring[i+dev->cur_rx_desc_idx];
			
			PRINT("  r_desc: %p\n", r_desc);
			PRINT("  r_desc->sdl: %d\n", r_desc->sdl);
			PRINT("  r_desc->cown: %d\n", r_desc->cown);
		}
		#endif

		//printf("rx_desc->cown: %d\n", rx_desc->cown);
		if (rx_desc->cown == 0) {
			//printf("rx_desc->cown == 0\n");
			break;
		}
		//printf("rx_desc->cown != 0\n");
		rcvpkt = dev->rx_ring_pkt[dev->cur_rx_desc_idx];

		//printf("rx_desc->sdl : %d\n", rx_desc->sdl);
		//
		//printf("rx\n");
		#if 0
		for (i=0 ; i < rx_desc->sdl ; ++i) {
			if (*(rcvpkt->pkt_buffer+i) == 0)
				++flag;
			else
				flag=0;
			if (flag > 5 ) break;

		}
		if (flag > 0) {
			//print_packet(rx_desc->sdp, rx_desc->sdl);
			printf("free pkt : %p ## ", rcvpkt);
			printf("free rcvpkt->pkt_buffer : %p\n", rcvpkt->pkt_buffer);
			print_packet(rcvpkt->pkt_buffer, rx_desc->sdl);
		}
		#endif
#if 0
{
	int i=0;
	int offset=0x2e;
	int ok_pkt=1;

	for (i=0 ; i < rx_desc->sdl-offset ; ++i)
		if (rcvpkt->pkt_buffer[i+offset] != 0x11) {
			printk("i+offset: %#x/%d\n", i+offset, i+offset);
			ok_pkt=0;
		}
	if (ok_pkt==0)
		print_packet(rcvpkt->pkt_buffer, rx_desc->sdl);
	printf("rcvpkt->pkt_buffer: %p\n", rcvpkt->pkt_buffer);
}
#endif

		rcvpkt->length = rx_desc->sdl;
		//printf("  rx_desc->sdl: %d\n", rx_desc->sdl);
		//printf("  rx_desc->sp: %#x\n", rx_desc->sp);
		newpkt = rx_alloc_pkt(dev);
		//printf("newpkt->length: %d\n", newpkt->length);
		//memset(newpkt->pkt_buffer, 0, newpkt->length);
		if (newpkt == 0) {
			printf("Allocate pkt failed on RX...\n");
		}
		dev->rx_ring_pkt[dev->cur_rx_desc_idx] = newpkt;
		rx_desc->sdp = (u32)newpkt->pkt_buffer;
		rx_desc->sdl = PKT_BUFFER_SIZE;
		rx_desc->cown = 0;
#ifdef STAR_GSW_DEBUG
		printf("RX PKT buffer: 0x%08x\n", rcvpkt->pkt_buffer);
		printf("RX PKT length: %d\n", rcvpkt->length);
#endif
		NetReceive(rcvpkt->pkt_buffer, rcvpkt->length);
		rx_free_pkt(dev, rcvpkt);

		// point to next index
		
		//dev->cur_rx_desc_idx = ((dev->cur_rx_desc_idx + 2) % RX_SIZE);
		dev->cur_rx_desc_idx = ((dev->cur_rx_desc_idx + 1) % RX_SIZE);
#if 0
		if (dev->cur_rx_desc_idx == (RX_SIZE - 1)) {
			dev->cur_rx_desc_idx = 0;
		} else {
			dev->cur_rx_desc_idx++;
		}
#endif

		rxcount++;
		if (rxcount == RX_SIZE) {
			break;
		}
	}

	dev->rx_pkt_count += rxcount;
	enable_rx_dma(0, 1);
}

s32 eth_rx(void)
{
	GswDev *dev = &gsw_dev;

	cns3xxx_gsw_rx(dev);

	return 0;
}

void eth_halt(void)
{
	GswDev *dev = &gsw_dev;

	//PRINT("...eth_halt()\n");

	if (cns3xxx_gsw_dev_initialized) {
#ifdef CLOSE_PORT 
		cns3xxx_gsw_close(dev);
#endif
		//memset(&gsw_dev, 0, sizeof(GswDev));
		//cns3xxx_gsw_dev_initialized = 0;
	}
}

int do_eth_down (void)
{
	printf("enter do_eth_down!!!\r\n");

	// Disabling MAC Ports
	MAC0_CFG_REG |= (1<<18);
	MAC1_CFG_REG |= (1<<18);
	MAC2_CFG_REG |= (1<<18);

	// Disabling DMA uto poll
	DMA_AUTO_POLL_CFG_REG |= (1<<4);
	DMA_AUTO_POLL_CFG_REG |= (1<<0);

	while (TS_DMA_STA_REG);
	while (1) {
		if (FS_DMA_STA_REG == 0) break;
		if ((FS_DMA_STA_REG==2) && (FS_DMA_CTRL0_REG==0)) {
			if ((FS_DMA_STA_REG==2) && (FS_DMA_CTRL0_REG==0)) // check twice
				break;
		}
	}
	
	return 0;
}
