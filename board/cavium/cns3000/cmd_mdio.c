#include <common.h>
#include <command.h>
#include "cns3xxx_phy.h"

int do_mdio (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	const char *cmd;
	u8 phy_addr;
	u8 phy_reg;
	u16 phy_data;

	cns3xxx_mdc_mdio_disable(0);

	if(argc >= 4) {
		phy_addr = (u8)simple_strtol(argv[2], 0, 0);
		phy_reg = (u8)simple_strtol(argv[3], 0, 0);
	}

	cmd = argv[1];
	if ((strcmp(cmd, "read") == 0) && (argc == 4)) {
		cns3xxx_read_phy(phy_addr, phy_reg, &phy_data);
		printf("phy_data: 0x%x\n", phy_data);
	}	
	else if ((strcmp(cmd, "write") == 0) && (argc == 5)) {
		phy_data = (u16)simple_strtol(argv[4], 0, 0);
   		cns3xxx_write_phy(phy_addr, phy_reg, phy_data);
	}
	else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	return 0;
}

U_BOOT_CMD(
	mdio,	5,	1,	do_mdio,
	"mdio     - read/write PHY register\n",
	"mdio read phy_addr phy_reg\n"
	"mdio write phy_addr phy_reg value\n"
);
