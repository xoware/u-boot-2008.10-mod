#
# (C) Copyright 2000-2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(BOARD).a 

OBJS	:= vega.o cns3xxx_gsw.o cns3xxx_phy.o cpu_clock_scale.o sys_interrupt.o spi_flash.o spi_bcm53115m.o cmd_bcm53115m.o partition.o cmd_mdio.o
SOBJS	:= lowlevel_init.o

OBJS += rtl8367/smi.o rtl8367/rtk_api.o 
OBJS += rtl8367/rtl8370_asicdrv_led.o 
OBJS += rtl8367/rtl8370_asicdrv_vlan.o 
OBJS += rtl8367/rtl8370_asicdrv_cputag.o 
OBJS += rtl8367/rtl8370_asicdrv_phy.o 
OBJS += rtl8367/rtl8370_asicdrv.o 
OBJS += rtl8367/rtl8370_asicdrv_port.o 
OBJS += rtl8367/rtl8370_asicdrv_mib.o


EXTRA_CFLAGS += -I./rtl8367 -D_LITTLE_ENDIAN

CFLAGS += $(EXTRA_CFLAGS)
CPPFLAGS += $(EXTRA_CFLAGS)

$(LIB):	$(OBJS) $(SOBJS)
	$(AR) crv $@ $^

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(SOBJS:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(SOBJS:.o=.S) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
