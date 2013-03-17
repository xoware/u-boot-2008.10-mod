#include <common.h>
#include <command.h>
#include "spi_bcm53115m.h"

int do_bcm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	const char *cmd;
	u32 block = 0, subblock = 0, addr = 0, value = 0, start_addr = 0, len = 0;

	cns3xxx_spi_initial();

	if(argc >= 5) {
		block = simple_strtol(argv[2], 0, 0);
		subblock = simple_strtol(argv[3], 0, 0);
		addr = simple_strtol(argv[4], 0, 0);
	}

	cmd = argv[1];
	if ((strcmp(cmd, "read") == 0) && (argc == 5)) {
		vsc7385_reg_read(block, subblock, addr, &value);
		printf("value: 0x%x\n", value);
	}	
	else if ((strcmp(cmd, "write") == 0) && (argc == 6)) {
		value = simple_strtol(argv[5], 0, 0);
		vsc7385_reg_write(block, subblock, addr, value);
	}
	else if ((strcmp(cmd, "load") == 0) && (argc == 4)) {
		start_addr = simple_strtol(argv[2], 0, 0);
		len = simple_strtol(argv[3], 0, 0);
		vsc7385_sw_load((u8 *)start_addr, len);
	}
	else if ((strcmp(cmd, "dump") == 0) && (argc == 4)) {
		start_addr = simple_strtol(argv[2], 0, 0);
		len = simple_strtol(argv[3], 0, 0);
		vsc7385_sw_dump(start_addr, len);
	}
	else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	return 0;
}

U_BOOT_CMD(
	bcm,	6,	1,	do_bcm,
	"bcm     - read/write vsc7385 register\n",
	"bcm read block subblock addr\n"
	"bcm write block subblock addr value\n"
	"bcm load start_addr len\n"
	"bcm display icpu_addr len\n"
);
