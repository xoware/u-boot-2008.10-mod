/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <common.h>

#include "misc.h"
#include "pm.h"
#include "spi_bcm53115m.h"

// for bcm53115M
#define ROBO_SPIF_BIT 7
#define BCM53115_SPI_CHANNEL 1
#define ROBO_RACK_BIT 5

/* define access macros */
#define SPI_MEM_MAP_VALUE(reg_offset)       (*((u32 volatile *)(0x71000000 + reg_offset)))

#define SPI_CONFIGURATION_REG           SPI_MEM_MAP_VALUE(0x40)
#define SPI_SERVICE_STATUS_REG          SPI_MEM_MAP_VALUE(0x44)
#define SPI_BIT_RATE_CONTROL_REG        SPI_MEM_MAP_VALUE(0x48)
#define SPI_TRANSMIT_CONTROL_REG        SPI_MEM_MAP_VALUE(0x4C)
#define SPI_TRANSMIT_BUFFER_REG         SPI_MEM_MAP_VALUE(0x50)
#define SPI_RECEIVE_CONTROL_REG         SPI_MEM_MAP_VALUE(0x54)
#define SPI_RECEIVE_BUFFER_REG          SPI_MEM_MAP_VALUE(0x58)
#define SPI_FIFO_TRANSMIT_CONFIG_REG        SPI_MEM_MAP_VALUE(0x5C)
#define SPI_FIFO_TRANSMIT_CONTROL_REG       SPI_MEM_MAP_VALUE(0x60)
#define SPI_FIFO_RECEIVE_CONFIG_REG     SPI_MEM_MAP_VALUE(0x64)
#define SPI_INTERRUPT_STATUS_REG        SPI_MEM_MAP_VALUE(0x68)
#define SPI_INTERRUPT_ENABLE_REG        SPI_MEM_MAP_VALUE(0x6C)

#define SPI_TRANSMIT_BUFFER_REG_ADDR        (CNS3XXX_SSP_BASE +0x50)
#define SPI_RECEIVE_BUFFER_REG_ADDR     (CNS3XXX_SSP_BASE +0x58)

#define GPIOB_MEM_MAP_VALUE(reg_offset)       (*((u32 volatile *)(0x74800000 + reg_offset)))

#define GPIOB_GPIO_DATA_OUT_REG					GPIOB_MEM_MAP_VALUE(0x00)
#define GPIOB_GPIO_DATA_IN_REG					GPIOB_MEM_MAP_VALUE(0x04)
#define GPIOB_PIN_DIR_REG								GPIOB_MEM_MAP_VALUE(0x08)


static int __spi_initial = 0;

void cns3xxx_spi_initial(void)
{
	if(__spi_initial)
		return;

	cns3xxx_pwr_clk_en(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);
	cns3xxx_pwr_power_up(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);
	cns3xxx_pwr_soft_rst(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);

	GPIOB_PIN_DIR_REG |= (0x1 << 18);
	GPIOB_GPIO_DATA_OUT_REG &= ~(0x1 << 18);

	/* share pin config. */
	HAL_MISC_ENABLE_SPI_PINS();
	cns3xxx_pwr_clk_en(CNS3XXX_PWR_CLK_EN(SPI_PCM_I2C));
	cns3xxx_pwr_soft_rst(CNS3XXX_PWR_SOFTWARE_RST(SPI_PCM_I2C));

	//SPI Pin Drive Strength
	//(0x30: 21mA) 
	//(0x20: 15.7mA) 
	//(0x10: 10.5mA) 
	//(0x00: 5.2mA)
//	MISC_IO_PAD_DRIVE_STRENGTH_CTRL_B &= ~0x30;
//	MISC_IO_PAD_DRIVE_STRENGTH_CTRL_B |= 0x30; //21mA...

	SPI_CONFIGURATION_REG = (((0x0 & 0x3) << 0) |	/* 8bits shift length */
				 (0x0 << 9) |	/* SPI mode */
				 (0x0 << 10) |	/* disable FIFO */
				 (0x1 << 11) |	/* SPI master mode */
				 (0x0 << 12) |	/* disable SPI loopback mode */
				 (0x1 << 13) |	/* clock phase */
				 (0x1 << 14) |	/* clock polarity */
				 (0x0 << 24) |	/* disable - SPI data swap */
#ifdef CONFIG_SPI_CNS3XXX_2IOREAD
				 (0x1 << 29) |	/* enable - 2IO Read mode */
#else
				 (0x0 << 29) |	/* disablea - 2IO Read mode */
#endif
				 (0x0 << 30) |	/* disable - SPI high speed read for system boot up */
				 (0x0 << 31));	/* disable - SPI */

	/* Set SPI bit rate 100MHz/6 */
	SPI_BIT_RATE_CONTROL_REG = 0x2;

	/* Set SPI Tx channel 0 */
	SPI_TRANSMIT_CONTROL_REG = 0x1;

	/* Set Tx FIFO Threshold, Tx FIFO has 2 words */
	SPI_FIFO_TRANSMIT_CONFIG_REG &= ~(0x03 << 4);
	SPI_FIFO_TRANSMIT_CONFIG_REG |= ((0x0 & 0x03) << 4);

	/* Set Rx FIFO Threshold, Rx FIFO has 2 words */
	SPI_FIFO_RECEIVE_CONFIG_REG &= ~(0x03 << 4);
	SPI_FIFO_RECEIVE_CONFIG_REG |= ((0x0 & 0x03) << 4);

	/* Disable all interrupt */
	SPI_INTERRUPT_ENABLE_REG = 0x0;

	/* Clear spurious interrupt sources */
	SPI_INTERRUPT_STATUS_REG = (0x0F << 4);

	/* Enable SPI */
	SPI_CONFIGURATION_REG |= (0x1 << 31);

	__spi_initial = 1;
	
	return;
}

static inline u8 cns3xxx_spi_bus_idle(void)
{
	return ((SPI_SERVICE_STATUS_REG & 0x1) ? 0 : 1);
}

static inline u8 cns3xxx_spi_tx_buffer_empty(void)
{
	return ((SPI_INTERRUPT_STATUS_REG & (0x1 << 3)) ? 1 : 0);
}

static inline u8 cns3xxx_spi_rx_buffer_full(void)
{
	return ((SPI_INTERRUPT_STATUS_REG & (0x1 << 2)) ? 1 : 0);
}

u8 cns3xxx_spi_tx_rx(u8 tx_channel, u8 tx_eof, u32 tx_data,
			    u32 * rx_data)
{
	u8 rx_channel;
	u8 rx_eof;

	while (!cns3xxx_spi_bus_idle()) ;	// do nothing

	while (!cns3xxx_spi_tx_buffer_empty()) ;	// do nothing

	SPI_TRANSMIT_CONTROL_REG &= ~(0x7);
	SPI_TRANSMIT_CONTROL_REG |= (tx_channel & 0x3) | ((tx_eof & 0x1) << 2);

	SPI_TRANSMIT_BUFFER_REG = tx_data;

	while (!cns3xxx_spi_rx_buffer_full()) ;	// do nothing

	rx_channel = SPI_RECEIVE_CONTROL_REG & 0x3;
	rx_eof = (SPI_RECEIVE_CONTROL_REG & (0x1 << 2)) ? 1 : 0;

	*rx_data = SPI_RECEIVE_BUFFER_REG;

	if ((tx_channel != rx_channel) || (tx_eof != rx_eof)) {
		return 0;
	} else {
		return 1;
	}
}

int cns3xxx_spi_tx_rx_n(u32 tx_data, u32 *rx_data, u32 tx_channel, u32 tx_eof_flag)
{
//	u8 cns3xxx_spi_tx_rx(u8 tx_channel, u8 tx_eof, u32 tx_data, u32 * rx_data);

	return cns3xxx_spi_tx_rx(tx_channel, tx_eof_flag, tx_data, rx_data);
}

int bcm53115M_reg_read(int page, int offset, u8 *buf, int len)
{
	u32 ch = BCM53115_SPI_CHANNEL;
	u8 cmd_byte;
	u32	dumy_word;
	u32 spi_status;
	int i;

	/*
	 * Normal SPI Mode (Command Byte)
	 * Bit7		Bit6		Bit5		Bit4		Bit3		Bit2		Bit1		Bit0
	 * 0		1			1			Mode=0		CHIP_ID2	ID1			ID0(lsb)	Rd/Wr(0/1)	
	 *
	 */

	/* Normal Read Operation */
	/* 1. Issue a normal read command(0x60) to poll the SPIF bit in the 
	      SPI status register(0XFE) to determine the operation can start */
	do 
        {
		cmd_byte = 0x60;      
		cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0xFE, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0x00, &spi_status, ch, 1);
		//udelay(100);
	}while ((spi_status >> ROBO_SPIF_BIT) & 1) ; // wait SPI bit to 0

	/* 2. Issue a normal write command(0x61) to write the register page value
		  into the SPI page register(0xFF) 	 */
	cmd_byte = 0x61;      
	cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(0xFF, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(page, &dumy_word, ch, 1);

	/* 3. Issue a normal read command(0x60) to setup the required RobiSwitch register
		  address 	 */
	cmd_byte = 0x60;	  
	cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(offset, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(0x00, &dumy_word, ch, 1);

	/* 4. Issue a normal read command(0x60) to poll the RACK bit in the 
	      SPI status register(0XFE) to determine the completion of read 	 */
	do 
	{
		cmd_byte = 0x60;	  
		cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0xFE, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0x00, &spi_status, ch, 1);
		//udelay(100);
	}while (((spi_status >> ROBO_RACK_BIT) & 1) == 0); // wait RACK bit to 1
	 
	/* 5. Issue a normal read command(0x60) to read the specific register's conternt
		  placed in the SPI data I/O register(0xF0) 	 */
	cmd_byte = 0x60;	  
	cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(0xF0, &dumy_word, ch, 0);
	// read content
	for (i=0; i<len; i++) {
		cns3xxx_spi_tx_rx_n(0x00, &dumy_word, ch, ((i==(len-1)) ? 1 : 0));
		buf[i] = (u8)dumy_word;
	}
	
	return 0;

}

int bcm53115M_reg_write(int page, int offset, u8 *buf, int len)
{
	u32 ch = BCM53115_SPI_CHANNEL;
	u8 cmd_byte;
	u32 dumy_word;
	u32 spi_status;
	int i;

	/*
	 * Normal SPI Mode (Command Byte)
	 * Bit7		Bit6		Bit5		Bit4		Bit3		Bit2		Bit1		Bit0
	 * 0		1			1			Mode=0		CHIP_ID2	ID1			ID0(lsb)	Rd/Wr(0/1)	
	 *
	 */

	/* Normal Write Operation */
	/* 1. Issue a normal read command(0x60) to poll the SPIF bit in the 
	      SPI status register(0XFE) to determine the operation can start */

	do 
        {
		cmd_byte = 0x60;      
		cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0xFE, &dumy_word, ch, 0);
		cns3xxx_spi_tx_rx_n(0x00, &spi_status, ch, 1);
	}while ((spi_status >> ROBO_SPIF_BIT) & 1) ; // wait SPI bit to 0

	/* 2. Issue a normal write command(0x61) to write the register page value
		  into the SPI page register(0xFF) 	 */
	cmd_byte = 0x61;      
	cns3xxx_spi_tx_rx_n((u32)cmd_byte, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(0xFF, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(page, &dumy_word, ch, 1);

	/* 3. Issue a normal write command(0x61) and write the address of the accessed
		  register followed by the write content starting from a lower byte */
	cmd_byte = 0x61;	  
	cns3xxx_spi_tx_rx_n((u32)cmd_byte, &dumy_word, ch, 0);
	cns3xxx_spi_tx_rx_n(offset, &dumy_word, ch, 0);
	// write content
	for (i=0; i<len; i++) {
		cns3xxx_spi_tx_rx_n((u32)buf[i], &dumy_word, ch, ((i==(len-1)) ? 1 : 0));
	}

	return 0;
}


int vsc7385_reg_read(u32 block, u32 subblock, u32 addr, u32 *value)
{
	u32 ch = BCM53115_SPI_CHANNEL;
	u8 cmd_byte;
	u32 dumy_word, data[4];

/*
  Read from VSC7385 register

               7   6   5   4   3   2   1   0
               __________  _  ______________
  Byte 0:       Block ID   0     Subblock
               _____________________________
  Byte 1:                 Address
               _____________________________
  Bytes 2-3:               Dummy
               _____________________________
  Bytes 4-7:             More dummy           <---- read data shifts out during these bytes

*/

	cmd_byte = ((block & 0x7) << 5) | (0 << 4) | (subblock & 0xf);

	cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);  // send command
	cns3xxx_spi_tx_rx_n(addr, &dumy_word, ch, 0);      // send address
	cns3xxx_spi_tx_rx_n(0x00, &dumy_word, ch, 0);      // pad byte
	cns3xxx_spi_tx_rx_n(0x00, &dumy_word, ch, 0);      // pad byte
	cns3xxx_spi_tx_rx_n(0x00, &data[0], ch, 0);        // read data 0
	cns3xxx_spi_tx_rx_n(0x00, &data[1], ch, 0);        // read data 1
	cns3xxx_spi_tx_rx_n(0x00, &data[2], ch, 0);        // read data 2
	cns3xxx_spi_tx_rx_n(0x00, &data[3], ch, 1);        // read data 3

	*value = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	
	return 0;
}

int vsc7385_reg_write(u32 block, u32 subblock, u32 addr, u32 value)
{
	u32 ch = BCM53115_SPI_CHANNEL;
	u8 cmd_byte;
	u32 dumy_word, data[4];

/*
  Write to VSC7385 register

               7   6   5   4   3   2   1   0
               __________  _  ______________
  Byte 0:       Block ID   1     Subblock
               _____________________________
  Byte 1:                 Address
               _____________________________
  Bytes 2-5:             Write Data
  

*/
	data[0] = (value >> 24);
        data[1] = (value >> 16) & 0xff;
        data[2] = (value >> 8) & 0xff; 
        data[3] = value & 0xff; 

	cmd_byte = ((block & 0x7) << 5) | (1 << 4) | (subblock & 0xf);

	cns3xxx_spi_tx_rx_n(cmd_byte, &dumy_word, ch, 0);  // send command
	cns3xxx_spi_tx_rx_n(addr, &dumy_word, ch, 0);      // send address
	cns3xxx_spi_tx_rx_n(data[0], &dumy_word, ch, 0);   // write data 0
	cns3xxx_spi_tx_rx_n(data[1], &dumy_word, ch, 0);   // write data 1
	cns3xxx_spi_tx_rx_n(data[2], &dumy_word, ch, 0);   // write data 2
	cns3xxx_spi_tx_rx_n(data[3], &dumy_word, ch, 1);   // write data 3
	
	return 0;
}

int vsc7385_sw_load(u8 *start_addr, u32 len)
{
	u32 value = 0;	
	int i;

/* 
     1. Stop V-Core clock
        ICPU_CTRL CLK_EN (bit 1) -> 0
*/

	vsc7385_reg_read(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, &value);
	value &= ~(1 << 1);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

/*
     2. Enable external access to on-chip RAM
        ICPU_CTRL EXT_ACC_EN (bit 2) -> 1
*/

	value |= (1 << 2);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

/*
     3. Write start address for code
        ICPU_ADDR ADDR (bits 0-12) -> 0
*/

	value = 0;
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_ADDR, value);

/*
     4. Write data byte-at-a-time, address auto-increments 
        ICPU_DATA DATA (bits 0-7)
*/

	for (i = 0; i < len; i++)
	{
		value = start_addr[i];
		vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_DATA, value);
	}

#if 0
/*
     5. Master reset
        GLORESET MEM_LOCK (bit 2) -> 1
        GLORESET MASTER_RESET (bit 0) -> 1
        sleep 125 usec
*/

	value = (1<<2) | 1;
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_GLORESET, value);
	udelay(150);

/*
     6. Set V-Core to boot from memory
        ICPU_CTRL BOOT_EN (bit 3) -> 1
*/

	vsc7385_reg_read(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, &value);
	value |= (1 << 3);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

/*
7. Start V-Core
   ICPU_CTRL EXT_ACC_EN (bit 2) -> 0
   ICPU_CTRL CLK_EN (bit 1) -> 1
*/

	value &= ~(1 << 2);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);
	value |= (1 << 1);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);
#endif

	return 0;
}

#define WIDTH 4
int vsc7385_sw_dump(u32 start_addr, u32 len)
{
	u32 value = 0;	
	int i;

/* 
     1. Stop V-Core clock
        ICPU_CTRL CLK_EN (bit 1) -> 0
*/

	vsc7385_reg_read(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, &value);
	value &= ~(1 << 1);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

/*
     2. Enable external access to on-chip RAM
        ICPU_CTRL EXT_ACC_EN (bit 2) -> 1
*/

	value |= (1 << 2);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

/*
     3. Write start address for code
        ICPU_ADDR ADDR (bits 0-12) -> start_addr
*/

	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_ADDR, start_addr);

/*
     4. Read data byte-at-a-time, address auto-increments 
        ICPU_DATA DATA (bits 0-7)
*/

	for (i = 0; i < len; i++)
	{
		/* start data row */
		if ((i % WIDTH) == 0) printf("0x%x: ", i);

		vsc7385_reg_read(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_DATA, &value);
		printf("0x%x ", value);

		/* end data row */
		if (((i+1) % WIDTH) == 0) printf("\n");
	}
	printf("\n");

/*
     5. Re-enable V-Core
        ICPU_CTRL EXT_ACC_EN (bit 2) -> 0
        ICPU_CTRL CLK_EN (bit 1) -> 1
*/

	value &= ~(1 << 2);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);
	value |= (1 << 1);
	vsc7385_reg_write(BLOCK_SYSTEM, SUBBLOCK_SYSTEM, ADDRESS_ICPU_CTRL, value);

	return 0;
}
