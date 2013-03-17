/*******************************************************************************
 *
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

#ifndef CNS3XXX_SWITCH_TYPE_H
#define CNS3XXX_SWITCH_TYPE_H

// use which port for network
#define USE_PORT0_VSC8601
//#define USE_PORT1_VSC8601
//#define USE_PORT2_101A

//#define CONFIG_VIRGO
//#define CONFIG_DORADO
//#define CONFIG_AR8316
//#define CONFIG_AR8327
//#define CONFIG_VB // validation board for cns3xxx
//#define CONFIG_VB_2 // validation board v2 for cns3xxx
//#define CONFIG_FPGA
//#define CONFIG_RTK8367
//#define CONFIG_AR8021

#ifdef CONFIG_FPGA
#define	INIT_PORT0_PHY cns3xxx_config_VSC8601(0, 0);
#define	INIT_PORT0_MAC cns3xxx_config_VSC8601_mac(0);
#define PORT0_LINK_UP vsc8601_power_down(0, 0);
#define PORT0_LINK_DOWN vsc8601_power_down(0, 1);

#define	INIT_PORT1_PHY cns3xxx_config_VSC8601(1, 1);
#define PORT1_LINK_UP  vsc8601_power_down(1, 0);
#define PORT1_LINK_DOWN vsc8601_power_down(1, 1);

//#define	INIT_PORT1_PHY icp_101a_init(1);
//cns3000_std_phy_power_down(1,0);
//cns3000_std_phy_power_down(1,1);
#define	INIT_PORT2_PHY icp_101a_init(2, 2);
//#define	INIT_PORT2_MAC icp_101a_init_mac(2);
#define PORT2_LINK_UP cns3xxx_std_phy_power_down(0, 2);
#define PORT2_LINK_DOWN cns3xxx_std_phy_power_down(0, 2);
#endif


#ifdef CONFIG_VB
#define	USE_MAC0
//#define	USE_MAC1
#define	USE_MAC2
#define MAC_NUM 3
#define INIT_PORT0_PHY bcm53115M_init(0, 0);
#define PORT0_LINK_DOWN disable_AN(0, 0);
#define PORT0_LINK_UP disable_AN(0, 1);

#define	INIT_PORT1_PHY 
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 

#define	INIT_PORT2_PHY probe_phy(2, 2);
#define PORT2_LINK_UP cns3xxx_std_phy_power_down(2, 0);
#define PORT2_LINK_DOWN cns3xxx_std_phy_power_down(2, 1);

#define CNS3XXX_MAC2_IP1001_GIGA_MODE

#endif

#ifdef CONFIG_VB_2
#define	USE_MAC0
#define	USE_MAC1
#define	USE_MAC2
#define MAC_NUM 3
	//#define INIT_PORT0_PHY bcm53115M_init(0, 0);
	//#define PORT0_LINK_DOWN disable_AN(0, 0);
	//#define PORT0_LINK_UP disable_AN(0, 1);

// MAC 0, PHY 1 = VSC8601
#define	INIT_PORT0_PHY vsc8601_init(0, 1);
#define PORT0_LINK_UP  vsc8601_power_down(1, 0);
#define PORT0_LINK_DOWN vsc8601_power_down(1, 1);
	
	//#define	INIT_PORT1_PHY 
	//#define PORT1_LINK_UP  
	//#define PORT1_LINK_DOWN 

// MAC 1, PHY 2 = VSC8601
#define	INIT_PORT1_PHY vsc8601_init(1, 2);
#define PORT1_LINK_UP  vsc8601_power_down(2, 0);
#define PORT1_LINK_DOWN vsc8601_power_down(2, 1);

// MAC 2 has switch 
#define	INIT_PORT2_PHY vsc7385_switch_init(2);
#define PORT2_LINK_UP 
#define PORT2_LINK_DOWN 

	#define MAC2_RGMII
	#define CNS3XXX_MAC2_IP1001_GIGA_MODE
#endif

#ifdef CONFIG_RTK8367
#define INIT_PORT0_PHY rtk8367_init_mac(0, 0);
#define PORT0_LINK_DOWN disable_AN(0, 0);
#define PORT0_LINK_UP disable_AN(0, 1);

#define	INIT_PORT1_PHY 
#define PORT1_LINK_UP 
#define PORT1_LINK_DOWN 

#define	INIT_PORT2_PHY
#define PORT2_LINK_UP
#define PORT2_LINK_DOWN

#endif

#ifdef CONFIG_RTK8367_ONE_LEG
#define USE_MAC0
#define MAC_NUM 1
#define INIT_PORT0_PHY rtk8367_init_mac(0, 0);
#define PORT0_LINK_DOWN disable_AN(0, 0);
#define PORT0_LINK_UP disable_AN(0, 1);

#define	INIT_PORT1_PHY 
#define PORT1_LINK_UP 
#define PORT1_LINK_DOWN 

#define	INIT_PORT2_PHY
#define PORT2_LINK_UP
#define PORT2_LINK_DOWN

#endif


#ifdef CONFIG_IP1001_X2
#define	USE_MAC0
#define	USE_MAC1
//#define	USE_MAC2
#define MAC_NUM 2

#define	INIT_PORT0_PHY icp_ip1001_init(0, 2);
#define PORT0_LINK_UP cns3xxx_std_phy_power_down(2, 0);
#define PORT0_LINK_DOWN cns3xxx_std_phy_power_down(2, 1);

#define	INIT_PORT1_PHY icp_ip1001_init(1, 3);
#define PORT1_LINK_UP cns3xxx_std_phy_power_down(3, 0);
#define PORT1_LINK_DOWN cns3xxx_std_phy_power_down(3, 1);

#define	INIT_PORT2_PHY cns3xxx_phy_auto_polling_conf(2, 5);
#define PORT2_LINK_UP  
#define PORT2_LINK_DOWN 
#endif

#ifdef CONFIG_AR8316
#define	INIT_PORT0_PHY ar8316_init(0, 0);
#define	INIT_PORT1_PHY ar8316_init_port5(1, 4);
#define	INIT_PORT2_PHY
#define PORT0_LINK_UP 
#define PORT0_LINK_DOWN 
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 
#define PORT2_LINK_UP  
#define PORT2_LINK_DOWN 
#endif

#ifdef CONFIG_AR8327
#define MAC_NUM 3
#define	USE_MAC0
#define	USE_MAC1
#define	INIT_PORT0_PHY ar8327_init(0, 0);
#define	INIT_PORT1_PHY ar8327_init_wan(1, 4);
#define	INIT_PORT2_PHY
#define PORT0_LINK_UP 
#define PORT0_LINK_DOWN 
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 
#define PORT2_LINK_UP  
#define PORT2_LINK_DOWN 
#endif

#ifdef CONFIG_DORADO2
#define	INIT_PORT0_PHY star_gsw_config_port0_VSC7385();
//#define	INIT_PORT1_PHY star_gsw_config_VSC8201(1,1);
#define	INIT_PORT1_PHY star_gsw_config_VSC8X01();
#define PORT0_LINK_UP disable_AN(0, 1);
#define PORT0_LINK_DOWN disable_AN(0, 0);
#define PORT1_LINK_UP std_phy_power_down(1, 0);
#define PORT1_LINK_DOWN std_phy_power_down(1, 1);
#endif


#ifdef CONFIG_LIBRA
#define	INIT_PORT0_PHY star_gsw_config_icplus_175c_phy4(); 
#define	INIT_PORT1_PHY
#define PORT0_LINK_UP icp_175c_all_phy_power_down(0);
#define PORT0_LINK_DOWN icp_175c_all_phy_power_down(1);
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 
#endif

#ifdef CONFIG_LEO
//#define	INIT_PORT0_PHY star_gsw_config_VSC8201(0,0);
#define	INIT_PORT0_PHY star_gsw_config_VSC8X01();
#define	INIT_PORT1_PHY
#define PORT0_LINK_UP std_phy_power_down(0,0);
#define PORT0_LINK_DOWN std_phy_power_down(0,1);
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 
#endif

#ifdef CONFIG_AR8021
#define	USE_MAC0
//#define	USE_MAC1
#define	USE_MAC2
#define MAC_NUM 3

#define INIT_PORT0_PHY ar8021_init(0, 1);
#define PORT0_LINK_DOWN cns3xxx_std_phy_power_down(1, 1);
#define PORT0_LINK_UP  cns3xxx_std_phy_power_down(1, 0);

#define	INIT_PORT1_PHY cns3xxx_phy_auto_polling_conf(1, 3);
#define PORT1_LINK_UP  
#define PORT1_LINK_DOWN 

#define	INIT_PORT2_PHY cns3xxx_phy_auto_polling_conf(2, 0);
#define PORT2_LINK_UP 
#define PORT2_LINK_DOWN 

#define CNS3XXX_MAC2_IP1001_GIGA_MODE
#endif

#endif
