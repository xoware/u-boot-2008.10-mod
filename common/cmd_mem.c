/*
 * (C) Copyright 2000
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

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#if defined(CONFIG_CMD_MMC)
#include <mmc.h>
#endif
#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif
#ifdef CONFIG_SUPPORT_PFLASH_AND_SFLASH
#include <flash.h>
#endif
#include <watchdog.h>

#ifdef	CMD_MEM_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

static int mod_mem(cmd_tbl_t *, int, int, int, char *[]);

/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
uint	dp_last_addr, dp_last_size;
uint	dp_last_length = 0x40;
uint	mm_last_addr, mm_last_size;

static	ulong	base_address = 0;

#if 1//cns3000
#include <common.h>
#include <exports.h>
#include <malloc.h>
#define MT_VERSION	"0.1"

#define CNS_MTEST_QUIET
//#define CNS_MTEST_STOP_TEST_IF_FAILURE

#if 0
#define MemTestBaseAddr	(0x20000000+0x08000000) //start from 128MB
#define MemTestSize	(0x100000*512-0x08000000) //test size=(512-128)MB
#else
#define MemTestBaseAddr	(0x1000000)
#define MemTestEndAddr	(0x2000000)
#define MEM_TEST_SKIP
#define MEM_TEST_SKIP_START 0x07f00000
#define MEM_TEST_SKIP_END	0x08000000
#define MEM_TEST_SKIP_START_1 0x27f00000
#define MEM_TEST_SKIP_END_1	0x28000000
#define ADDRESS_MASK 0xffffff
#endif

#define VEGA_DDR_BUSWIDTH	16

typedef struct {
	char *name;
	uint32_t (*fp)(uint32_t, uint32_t, uint32_t);
	uint32_t bursts;
	uint32_t failures;
	uint32_t total_failures;
} testDesc;

uint32_t memoryMap_base = MemTestBaseAddr;
#if 0
uint32_t memoryMap_size = MemTestSize;
#endif
uint32_t memoryMap_end = MemTestEndAddr;
uint32_t vega_run_data_bus_tests (uint32_t base);
uint32_t vega_run_addr_bus_tests (uint32_t base, uint32_t end);
uint32_t vega_run_misc_tests (uint32_t base, uint32_t end, uint32_t count);


/* XXX Assumptions/Limitations 
 * 1. Assumes Cache is disabled.
 * 2. No multi-core support.
 */
int do_memtest_cns3000 (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	uint32_t test_counter = 1;
/*
 *	1.	Disable Cache or if Cache is enabled, make sure to issue r/w commands
 * 	  	such that DDR transaction is generated. Octeon does it by turning the
 * 	  	cache to one way and generates addresses that evict the previous cache
 * 	  	entry.
 *	2. 	Allocate contiguous memory for testing.
 *	3.  Run Data Bus Tests
 *	4.  Run Addr Bus Tests
 *	5.  Run Miscellaneous tests
 */

	if (sizeof(void*)!=4) 
	{
		printf ("This program expects 32 bit pointers .. expect the unexpected\n");
	}

#ifndef CNS_MTEST_QUIET
	printf ("argc = %d\n", argc);

	for (i=0; i<=argc; ++i) {
		printf ("argv[%d] = \"%s\"\n",
			i,
			argv[i] ? argv[i] : "<NULL>");
	}
#endif

	if (2 < argc) {
		memoryMap_base = (ulong *)simple_strtoul(argv[1], NULL, 16);
		memoryMap_end = (ulong *)simple_strtoul(argv[2], NULL, 16);        	
	}	
	
	if (3 < argc) {
		test_counter = (ulong *)simple_strtoul(argv[3], NULL, 0);
	}

	memoryMap_end -= 4;
#ifndef CNS_MTEST_QUIET
	printf ("\nMtest diagnostic version %s\n",MT_VERSION);
#endif
	printf ("\nTest memory from 0x%.8x to 0x%.8x", memoryMap_base, memoryMap_end);
	if (test_counter) {
		printf (" %lu time(s)\n", test_counter);
	} else {
		printf (" infinite loop\n");
	}
	vega_run_data_bus_tests (memoryMap_base);
	vega_run_addr_bus_tests (memoryMap_base, memoryMap_end);
	vega_run_misc_tests (memoryMap_base, memoryMap_end, test_counter);


	return 0;
}
static void write_16bit_data_bus_burst (uint32_t *p1, uint32_t *p2)
{
	// Fill one cache entry - Vega has cache line size of 32 Bytes
    p1[ 0] = 0x0000FFFF;
    p1[ 1] = 0x5555AAAA;
    p1[ 2] = 0xCCCC3333;
    p1[ 3] = 0xDDDD2222;
    p1[ 4] = 0x0000FFFF;
    p1[ 5] = 0x5555AAAA;
    p1[ 6] = 0xCCCC3333;
    p1[ 7] = 0xDDDD2222;

      /* Create the compliment of the first pattern and write it to the other
       * entry address (P2), evicting the P1 entry from the L2Cache and push
       * its data out the DDR interface. Sync the L1 and L2 caches forcing the
       * P2 entry into the L2Cache.
       */
    p2[ 0] = 0xFFFF0000;
    p2[ 1] = 0xAAAA5555;
    p2[ 2] = 0x3333CCCC;
    p2[ 3] = 0x2222DDDD;
    p2[ 4] = 0xFFFF0000;
    p2[ 5] = 0xAAAA5555;
    p2[ 6] = 0x3333CCCC;
    p2[ 7] = 0x2222DDDD;

	// asm volatile ("syncw");
	return;
}

uint32_t test_data_bus_burst (uint32_t area, uint32_t bursts)
{
	uint32_t *p1, *p2;
	uint32_t i,j,bus_width,failures,last_failures;

	failures = last_failures = 0;
	bus_width = VEGA_DDR_BUSWIDTH;

	/* XXX : If Cache is enabled, calculate p2 such that p2 evicts p1 from L2C
	 * for now, it (p2) is arbitrary - some 1 MB away from p1
	 */
	p1 = (uint32_t*) (area+0x80);
	p2 = p1 + 1024*1024;
	
	for (i=0; i<bursts; i++)
	{
		switch (bus_width)
		{
			case 16:
				write_16bit_data_bus_burst (p1,p2);
				break;
			default:
				printf ("Not implemented for now \n");
				return 1;
		}

		for (j=0; j <8; j+=2) 
			if ((p1[j] != ~p2[j]) || (p1[j+1] != ~p2[j+1]))
				failures ++;

		if (failures > last_failures)
		{
			last_failures = failures;
			for (j=0; j<8; j+=2)
				printf ("Fail[%d]: "
							"p1[%2d] 0x%08X_%08X  "
							"p2[%2d] 0x%8X_%8X  "
							"xor(%8X_%8X)\n",
							i, j, p1[j],p1[j+1],j,p2[j],p2[j+1],
							(p1[j]^p2[j]), (p1[j+1] ^ p2[j+1]));
		}
	}

	return failures;
}

static uint32_t write_16bit_data_bus_walk (uint32_t bursts, uint32_t pattern,
											uint32_t *p1, uint32_t *p2)
{
	uint32_t mask;
	uint32_t i,k,failures=0,last_failures=0;

	pattern = pattern & 0XFFFF;
	pattern = pattern | (pattern << (16+1));

	for (i=0; i < bursts; i++)
	{
		mask = pattern;
		// Form Entry -- walk 1's 
		p1[0] = mask;
		p1[1] = ~p1[0];

		p1[2] = p1[0] << 4;
		p1[3] = ~p1[2];

		p1[4] = p1[2] << 4;
		p1[5] = ~p1[4];

		p1[6] = p1[4] << 4;
		p1[7] = ~p1[6];

		// Form Entry -- walk 0's 
		p2[0] = ~mask;
		p2[1] = ~p2[0];

		p2[2] = ~(p2[1] << 4);
		p2[3] = ~p2[2];

		p2[4] = ~(p2[3] << 4);
		p2[5] = ~p2[4];

		p2[6] = ~(p2[5] << 4);
		p2[7] = ~p2[6];

		for (k=0; k <8; k+=2)
			if ((p1[k] != ~p2[k]) || (p1[k + 1] != ~p2[k + 1]))
				failures ++;
		if (failures > last_failures)
		{
			last_failures = failures;
			for (k=0; k<8; k+=2)
				printf ("Fail[%d]: "
				                "p1[%2d] 0x%08X_%08X  "
								"p2[%2d] 0x%08X_%08X  "
								"xor(%08X_%08X)\n",
								i, k, p1[k], p1[k+1], k, p2[k], p2[k+1],
								(p1[k] ^ p2[k]), (p1[k+1] ^ p2[k+1]));
			return failures;
		}
	}

	return failures;
}

#if 0
uint64_t write_64bit_data_bus_walk (uint64_t bursts, uint64_t pattern, 
									uint64_t *p1, uint64_t *p2)
{
	uint64_t mask;
	uint32_t i,j,k,failures,last_failures;

	failures = 0;
	last_failures = 0;

	for (i=0; i<bursts; i++)
	{
		mask = pattern;

		for (j=0; j<64; j+=8)
		{
			p1[0] = mask;
			p1[1] = ~p1[0];

			p1[2] = p1[0] << 1;
			p1[3] = ~p1[2];

			p1[4] = p1[2] << 1;
			p1[5] = ~p1[4];

			p1[6] = p1[4] << 1;
			p1[7] = ~p1[6];

			p1[8] = p1[6] << 1;
			p1[9] = ~p1[8];

			p1[10] = p1[8] << 1;
			p1[11] = ~p1[10];

			p1[12] = p1[10] << 1;
			p1[13] = ~p1[12];

			p1[14] = p1[12] << 1;
			p1[15] = ~p1[14];


			// asm volatile ("syncw");

			p2[0] = ~mask;
			p2[1] = ~p2[0];

			p2[2] = ~(p2[1] << 1);
			p2[3] = ~p2[2];

			p2[ 4] = ~(p2[3] << 1);
			p2[ 5] = ~p2[4];

			p2[ 6] = ~(p2[5] << 1);
			p2[ 7] = ~p2[6];

			p2[ 8] = ~(p2[7] << 1);
			p2[ 9] = ~p2[8];

			p2[10] = ~(p2[9] << 1);
			p2[11] = ~p2[10];

			p2[12] = ~(p2[11] << 1);
			p2[13] = ~p2[12];

			p2[14] = ~(p2[13] << 1);
			p2[15] = ~p2[14];

			//asm volatile ("syncw");
			//
			for (k=0;k<16;k+=2)
				if ((p1[k] != ~p2[k]) | (p1[k + 1] != ~p2[k + 1]))
					failures ++;

			if (failures > last_failures)
			{
				last_failures = failures;
				for (k=0; k<16; k+=2)
					printf ("Fail[%d]: "
									"p1[%2d] 0x%016lX_%016lX  "
									"p2[%2d] 0x%016lX_%016lX  "
									"xor(%016lX_%016lX)\n",
									 i, k, p1[k], p1[k+1], k, p2[k], p2[k+1],
									 (p1[k] ^ p2[k]), (p1[k+1] ^ p2[k+1]));
				return failures;
			}

			if (mask & 0x1000000000000000LL)
				mask = pattern;
			else
				mask <<=8;
		}
	}
	return failures;
}
#endif

uint32_t test_data_bus_walks (uint32_t area, uint32_t pattern, uint32_t bursts)
{
	uint32_t *p1, *p2;

	p1 = (uint32_t*) (area + 0x80);
	p2 = p1 + 1024*1024;

	return write_16bit_data_bus_walk (bursts, pattern, p1, p2);
}

uint32_t vega_run_data_bus_tests (uint32_t base)
{
	uint32_t failures = 0;

	printf ("DDR Data Bus Tests [with base=0x%.8x]\n",base);

	/* Simple test for basic read/write */
	if (test_data_bus_burst (base,16)) {
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
		while(1)
#endif
		printf ("Test failed : Stuck Data line\n");
		failures ++;
	}

	/* Walking ones/zeroes test */
	if (test_data_bus_walks (base,1,8)) {
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
		while(1)
#endif
		printf ("Test failed : Stuck Data line\n");
		failures ++;
	}
	if (test_data_bus_walks (base,3,8)) {
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
		while(1)
#endif
		printf ("Test failed : Stuck Data line\n");
		failures ++;
	}
	if (test_data_bus_walks (base,7,8)) {
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
		while(1)
#endif
		printf ("Test failed : Stuck Data line\n");
		failures ++;
	}

	return (failures==0);

}

int search_alias (uint32_t base, uint32_t p1, uint32_t max_address)
{
	int result;
	volatile uint32_t *p3;
	uint32_t offset;

	result = 0;
	offset = 0x80;

	while (1)
	{
		p3 = (volatile uint32_t *) (base | offset);
		if ((p3 != (volatile uint32_t*) p1) &&
			(p3[0] == 0xAAAAAAAA))
			result = 1;

		offset <<=1;
		if (((base|offset) > max_address) ||
			 (offset &(uint32_t)p3))
			 return result;
	}
	return result;
}

uint32_t test_addr_bus (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	volatile uint32_t *p1 ;
	uint32_t	*base,*start, offset;
	uint32_t i, failures = 0;

	p1 = (uint32_t*) 0x80;
	while ((uint32_t)p1 < area)
		p1 = (uint32_t*)((uint32_t)p1 << 1);
	
	if ((uint32_t)p1 > max_address)
		p1 = (volatile uint32_t *) area;

	for (i=0; i<bursts; i++)
	{
		base = (uint32_t *) p1;
		start = base;

		while (1)
		{
			offset = 0x80;

			while (1)
			{
				p1 = (volatile uint32_t *)((uint32_t)base | offset);
				p1[0] = 0x55555555;
				// XXX evicting address here p2[0] = 0x55555555;
				// sync the l1 and l2 caches
				offset <<=1;
				if ((((uint32_t)base|offset) > max_address) ||
					(offset & (uint32_t)p1))
				{
					base = (uint32_t*) p1;
					break;
				}
			}

			if ((uint32_t)p1 & 0x80) break;
		}

		/*Now check for the values that had been written in the previous loop*/
		p1 = start;
		base = (uint32_t*) p1;

		while (1)
		{
			offset = 0x80;

			while (1)
			{
				p1 = (volatile uint32_t*)((uint32_t)base | offset);
				if (p1[0] !=  0x55555555) failures ++;
				offset <<= 1;
				if ((((uint32_t) base | offset) > max_address) ||
						(offset & (uint32_t) p1))
				{
					base = (uint32_t*)p1;
					break;
				}
			}
			if ((uint32_t)p1 &0x80) break;
		}

		/* Check for address aliasing */
		p1 = start;
		base = (uint32_t*)p1;

		while (1)
		{
			offset = 0x80;
			while (1)
			{
				p1 = (volatile uint32_t *)((uint32_t) base | offset);

				/*Write the inverse pattern to the target address. All other
			 	* addresses should have the normal pattern.
			 	*/
				p1[0] = 0xAAAAAAAA;
				// if cache was enabled, make sure you flush this to DDR

			 	/* Scan across all test addresses looking for a match
			  	 * against the inverse pattern. If there is a match and it
			  	 * is not at the target address, the target address write
			  	 * failed and the pattern went to an aliased address.
			  	 */

				if (search_alias ((uint32_t)start, (uint32_t)p1, max_address))
			 		failures ++;

				p1[0] = 0x55555555;
				offset <<=1;

				if ((((uint32_t) base | offset) > max_address) ||
					(offset & (uint32_t) p1))
				{
					base = (uint32_t*) p1;
					break;
				}
			}
			if ((uint32_t)p1 & 0x80) break;
		}
	}
	return failures;
}

uint32_t vega_run_addr_bus_tests (uint32_t base, uint32_t end)
{
	uint32_t failures=0;
	
	printf ("DDR Address Bus Tests [0x%.8x_0x%.8x]\n", base, end);

	if (test_addr_bus (base, end, 1))
	{
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
		while(1)
#endif
		printf ("Test Failed: Stuck Address line\n");
		failures ++;
	}

	return (failures == 0);
}

uint32_t test_mem_self_addr (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t 	i,failures, probes;
	uint32_t	data,end;
	volatile uint32_t *p1;
	failures = 0;
	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif

	for (i=0; i<bursts; i++)
	{
		/* Write the pattern to memory. Each location receives the address of
		 * the location. Take care of Caching XXX
		 */
		for (p1 = (uint32_t*)area; p1 <(uint32_t*)max_address; p1++) {
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
			*p1 = (uint32_t)p1;
		}

		/*Read by ascending address the written memroy and verify */
		for (p1 = (uint32_t*)area; p1<(uint32_t*)max_address; p1++)
		{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
			data = *p1;
			if (data != (uint32_t)p1) {
				printf ("%s: fail at 0x%.8x\n", __FUNCTION__, p1);
				failures ++;
			}
		}

		/*Read by descending address the written memory and verify */
		end = max_address - sizeof (uint32_t);
		for (p1 = (uint32_t*)end; p1 > (uint32_t*)area; p1--)
		{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
			data = *p1;
			if (data != (uint32_t)p1) { 
				printf ("%s: fail at 0x%.8x\n", __FUNCTION__, p1);
				failures ++;
			}
		}

		#if 0
		/*Read from random address within the area */
		probes = (uint32_t)((max_address - area) / (4*1024));
		for (i=0; i <probes;)
		{
			p1 = area + rand (max_address);
			if ((p1 > (uint64_t*)area) && (p1 < (uint64_t*)end))
			{
				data = *p1;
				if (data != (uint64_t)p1) failures ++;
				i++;
			}
		}
		#endif
	}

	return failures;
}

uint32_t test_mem_march_c (uint32_t area, uint32_t max_address, uint32_t pattern)
{
	uint32_t 	failures = 0;
	uint32_t	end;
	volatile uint32_t	*p1;


#ifndef CNS_MTEST_QUIET
	printf ("func-%s\n",__func__);
	printf ("In with %x area, %x max, %x pattern\n",area,max_address,pattern);
#endif


	end = max_address - sizeof (uint32_t);

	/*Pass 1 ascending address, fill memory with pattern. */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 1 with (%x - %x)\n",area,max_address);
#endif
	for (p1 =(uint32_t*)area; p1 < (uint32_t*)max_address; p1++) {
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		*p1 = pattern;
	}

	/*Pass 2: ascending address, read pattern and write ~pattern */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 2 \n");
#endif
	for (p1 =(uint32_t*)area; p1 < (uint32_t*)max_address; p1++) 
	{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		if (*p1 != pattern) 
		{
			printf ("Pass2 Failure: 0x%010X: "
			                    "want 0x%08X  got 0x%08X  xor(%08X)\n",
								(uint32_t) p1, pattern, *p1, (pattern ^ *p1));
			failures ++;
		}
		*p1 = ~pattern;
	}

	/*Pass 3: ascending address, read ~pattern and write pattern */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 3\n");
#endif
	for (p1 =(uint32_t*)area; p1 < (uint32_t*)max_address; p1++) 
	{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		if (*p1 != ~pattern)
		{
			printf ("Pass3 Failure: 0x%010X: "
			                    "want 0x%016X  got 0x%016X xor(%016X)\n",
								(uint32_t) p1, ~pattern, *p1, (~pattern ^ *p1));
			failures ++;
		}
		*p1 = pattern;
	}

	/*Pass 4: descending address, read pattern and write ~pattern */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 4\n");
#endif
	for (p1 =  (uint32_t*)end; p1 > (uint32_t*)area; p1--)
	{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		if (*p1 != pattern) 
		{
			printf ("Pass4 Failure: 0x%010X: "
			                    "want 0x%016X  got 0x%016X xor(%016X)\n",
								(uint32_t) p1, pattern, *p1, (pattern ^ *p1));
			failures ++;
		}
		*p1 = ~pattern;
	}

	/*Pass 5: descending address, read ~pattern and write pattern. */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 5\n");
#endif
	for (p1 = (uint32_t*)end; p1 > (uint32_t*)area; p1--)
	{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		if (*p1 != ~pattern) 
		{
			printf ("Pass4 Failure: 0x%010X: "
			                    "want 0x%016X  got 0x%016X xor(%016X)\n",
								(uint32_t) p1, ~pattern, *p1, (~pattern ^ *p1));
			failures ++;
		}
		*p1 = pattern;
	}

	/*Pass 6 : ascending address, read pattern */
#ifndef CNS_MTEST_QUIET
	printf ("Phase 6\n");
#endif
	for (p1 = (uint32_t*)area; p1 < (uint32_t*)max_address; p1++) {
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
		if (*p1 != pattern)
		{
			printf ("Pass6 Failure: 0x%010X: "
			                    "want 0x%016X  got 0x%016X xor(%016X)\n",
								(uint32_t) p1, pattern, *p1, (pattern ^ *p1));
			failures ++;
		}
	}

	return failures;
}

uint32_t test_mem_solid (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t i, failures =0;

	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif

	for (i=0; i<bursts; i++)
		failures += test_mem_march_c (area,max_address,0xFFFFFFFF);
	return failures;
}

uint32_t test_mem_checkerboard (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t i,failures =0;
	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif
	for (i=0; i<bursts; i++)
		failures += test_mem_march_c (area, max_address,0x55555555);
	return failures;
}

uint32_t test_mem_pattern (uint32_t area,uint32_t max_address, uint32_t pattern,
							uint32_t passes)
{
	uint32_t i,failures = 0;
	uint32_t data;
	volatile uint32_t *p1;
#ifndef CNS_MTEST_QUIET
	printf ("func-%s\n",__func__);
	printf ("In with %x area, %x max, %x pattern %x pass \n",area,max_address,pattern,passes);
#endif


	for (i=0; i <passes; i++)
	{
		if (i & 0x1) pattern = ~pattern;
		for (p1 = (uint32_t*)area; p1 < (uint32_t*)max_address; p1++) {
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
			*p1 = pattern;
		}
		// Read now !!
		for (p1 = (uint32_t*) area; p1 < (uint32_t*)max_address; p1++)
		{
#ifdef MEM_TEST_SKIP
			if ((p1 >= MEM_TEST_SKIP_START) && (p1 <= MEM_TEST_SKIP_END))
				continue;
			if ((p1 >= MEM_TEST_SKIP_START_1) && (p1 <= MEM_TEST_SKIP_END_1))
				continue;
#endif
#ifndef CNS_MTEST_QUIET
		if (!((int)p1 & ADDRESS_MASK))
			printf("%s: Test 0x%.8x\n", __FUNCTION__, p1);
#endif
			data = *p1;
			if (data != pattern) failures ++;
		}
	}

	return failures;
}

uint32_t test_mem_leftwalk1 (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t i,failures = 0;
	uint32_t pattern;

	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif
	for (i=0; i<bursts; i++)
		for (pattern =1; pattern !=0; pattern <<=1)
			failures += test_mem_pattern (area, max_address,pattern,1);
	return failures;
}

uint32_t test_mem_rightwalk1 (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t i, failures = 0;
	uint32_t pattern;
	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif
	for (i=0; i<bursts; i++)
		for (pattern =0x80000000; pattern != 0; pattern >>= 1)
			failures += test_mem_pattern (area,max_address, pattern, 1);
	
	return failures;

}

uint32_t test_mem_leftwalk0 (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t 	i, failures = 0;
	uint32_t	pattern;
	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif
	for (i=0; i <bursts; i++)
		for (pattern =1; pattern !=0; pattern <<=1)
			failures += test_mem_pattern (area, max_address, ~pattern,1);

	return failures;
}

uint32_t test_mem_rightwalk0 (uint32_t area, uint32_t max_address, uint32_t bursts)
{
	uint32_t i,failures = 0;
	uint32_t pattern;

	printf ("\n\n<<func:%s>>\n\n",__func__);
#ifndef CNS_MTEST_QUIET
	printf ("In with %x area, %x max, %x bursts\n",area,max_address,bursts);
#endif
	for (i=0; i< bursts; i++)
		for (pattern = 0x80000000; pattern != 0; pattern >>=1)
			failures += test_mem_pattern (area,max_address, ~pattern,1);
	return failures;
}

testDesc tests[] = {

#if 1
	{ "Self Address",          test_mem_self_addr,      1, 0, 0},
	{ "March C- Solid Bits",   test_mem_solid,          1, 0, 0},
#endif
	{ "March C- Checkerboard", test_mem_checkerboard,   1, 0, 0},
	{ "Walking Ones Left",     test_mem_leftwalk1,      1, 0, 0},
	{ "Walking Ones Right",    test_mem_rightwalk1,     1, 0, 0},
	{ "Walking Zeros Left",    test_mem_leftwalk0,      1, 0, 0},
	{ "Walking Zeros Right",   test_mem_rightwalk0,     1, 0, 0},
	#if 0
	{ "Random XOR",            test_mem_xor,          256, 0, 0},
	{ "Random Data",           test_mem_random,        32, 0, 0},
	#endif
	{ NULL, NULL,  0, 0, 0}
};

uint32_t vega_run_misc_tests (uint32_t base, uint32_t end, uint32_t counter)
{
	int i;
	uint32_t failures, pass;

	failures = 0;
	printf ("In %s: with 0x%.8x 0x%.8x\n",__FUNCTION__, base, end);

#if 0
	pass = 1;
	for (pass=1; pass <= (unsigned)counter; pass++)
#else
	pass = 1;
	while (1)
#endif
	{
		printf ("\nPass %d\n",pass);
		for (i=0;;i++)
		{
			if (!tests[i].name) break;

			tests[i].failures = tests[i].fp (base, end, tests[i].bursts);
			if (tests[i].failures)
			{
#ifdef CNS_MTEST_STOP_TEST_IF_FAILURE
				while(1)
#endif
				printf ("Test failed: %s\n",tests[i].name);
				tests[i].total_failures += tests[i].failures;
			}
		}

		printf ("Pass %d Summary\n",pass);
		for (i=0; ; i++)
		{
			if (!tests[i].name) break;
			printf ("%30s: %12u  %12u\n",
								 tests[i].name,
								 tests[i].failures,
								 tests[i].total_failures);
			failures += tests[i].failures;
		}

		if (failures) 
			return failures;
		
		pass++;

		if (counter) {
			if (pass > counter) break;
		}

	}
	return failures;
}



#endif //cns3000


/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16
int do_mem_md ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, length;
#if defined(CONFIG_HAS_DATAFLASH)
	ulong	nbytes, linebytes;
#endif
	int	size;
	int rc = 0;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = dp_last_addr;
	size = dp_last_size;
	length = dp_last_length;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* New command specified.  Check for a size specification.
		 * Defaults to long if no or incorrect specification.
		 */
		if ((size = cmd_get_data_size(argv[0], 4)) < 0)
			return 1;

		/* Address is specified since argc > 1
		*/
		addr = simple_strtoul(argv[1], NULL, 16);
		addr += base_address;

		/* If another parameter, it is the length to display.
		 * Length is the number of objects, not number of bytes.
		 */
		if (argc > 2)
			length = simple_strtoul(argv[2], NULL, 16);
	}

#if defined(CONFIG_HAS_DATAFLASH)
	/* Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once, and all accesses are with the specified bus width.
	 */
	nbytes = length * size;
	do {
		char	linebuf[DISP_LINE_LEN];
		void* p;
		linebytes = (nbytes>DISP_LINE_LEN)?DISP_LINE_LEN:nbytes;

		rc = read_dataflash(addr, (linebytes/size)*size, linebuf);
		p = (rc == DATAFLASH_OK) ? linebuf : (void*)addr;
		print_buffer(addr, p, size, linebytes/size, DISP_LINE_LEN/size);

		nbytes -= linebytes;
		addr += linebytes;
		if (ctrlc()) {
			rc = 1;
			break;
		}
	} while (nbytes > 0);
#else

# if defined(CONFIG_BLACKFIN)
	/* See if we're trying to display L1 inst */
	if (addr_bfin_on_chip_mem(addr)) {
		char linebuf[DISP_LINE_LEN];
		ulong linebytes, nbytes = length * size;
		do {
			linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
			memcpy(linebuf, (void *)addr, linebytes);
			print_buffer(addr, linebuf, size, linebytes/size, DISP_LINE_LEN/size);

			nbytes -= linebytes;
			addr += linebytes;
			if (ctrlc()) {
				rc = 1;
				break;
			}
		} while (nbytes > 0);
	} else
# endif

	{
		/* Print the lines. */
		print_buffer(addr, (void*)addr, size, length, DISP_LINE_LEN/size);
		addr += size*length;
	}
#endif

	dp_last_addr = addr;
	dp_last_length = length;
	dp_last_size = size;
	return (rc);
}

int do_mem_mm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return mod_mem (cmdtp, 1, flag, argc, argv);
}
int do_mem_nm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return mod_mem (cmdtp, 0, flag, argc, argv);
}

int do_mem_mw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, writeval, count;
	int	size;

	if ((argc < 3) || (argc > 4)) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/* Address is specified since argc > 1
	*/
	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	/* Get the value to write.
	*/
	writeval = simple_strtoul(argv[2], NULL, 16);

	/* Count ? */
	if (argc == 4) {
		count = simple_strtoul(argv[3], NULL, 16);
	} else {
		count = 1;
	}

	while (count-- > 0) {
		if (size == 4)
			*((ulong  *)addr) = (ulong )writeval;
		else if (size == 2)
			*((ushort *)addr) = (ushort)writeval;
		else
			*((u_char *)addr) = (u_char)writeval;
		addr += size;
	}
	return 0;
}

#ifdef CONFIG_MX_CYCLIC
int do_mem_mdc ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	ulong count;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	count = simple_strtoul(argv[3], NULL, 10);

	for (;;) {
		do_mem_md (NULL, 0, 3, argv);

		/* delay for <count> ms... */
		for (i=0; i<count; i++)
			udelay (1000);

		/* check for ctrl-c to abort... */
		if (ctrlc()) {
			puts("Abort\n");
			return 0;
		}
	}

	return 0;
}

int do_mem_mwc ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	ulong count;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	count = simple_strtoul(argv[3], NULL, 10);

	for (;;) {
		do_mem_mw (NULL, 0, 3, argv);

		/* delay for <count> ms... */
		for (i=0; i<count; i++)
			udelay (1000);

		/* check for ctrl-c to abort... */
		if (ctrlc()) {
			puts("Abort\n");
			return 0;
		}
	}

	return 0;
}
#endif /* CONFIG_MX_CYCLIC */

int do_mem_cmp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr1, addr2, count, ngood;
	int	size;
	int     rcode = 0;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	addr1 = simple_strtoul(argv[1], NULL, 16);
	addr1 += base_address;

	addr2 = simple_strtoul(argv[2], NULL, 16);
	addr2 += base_address;

	count = simple_strtoul(argv[3], NULL, 16);

#ifdef CONFIG_HAS_DATAFLASH
	if (addr_dataflash(addr1) | addr_dataflash(addr2)){
		puts ("Comparison with DataFlash space not supported.\n\r");
		return 0;
	}
#endif

#ifdef CONFIG_BLACKFIN
	if (addr_bfin_on_chip_mem(addr1) || addr_bfin_on_chip_mem(addr2)) {
		puts ("Comparison with L1 instruction memory not supported.\n\r");
		return 0;
	}
#endif

	ngood = 0;

	while (count-- > 0) {
		if (size == 4) {
			ulong word1 = *(ulong *)addr1;
			ulong word2 = *(ulong *)addr2;
			if (word1 != word2) {
				printf("word at 0x%08lx (0x%08lx) "
					"!= word at 0x%08lx (0x%08lx)\n",
					addr1, word1, addr2, word2);
				rcode = 1;
				break;
			}
		}
		else if (size == 2) {
			ushort hword1 = *(ushort *)addr1;
			ushort hword2 = *(ushort *)addr2;
			if (hword1 != hword2) {
				printf("halfword at 0x%08lx (0x%04x) "
					"!= halfword at 0x%08lx (0x%04x)\n",
					addr1, hword1, addr2, hword2);
				rcode = 1;
				break;
			}
		}
		else {
			u_char byte1 = *(u_char *)addr1;
			u_char byte2 = *(u_char *)addr2;
			if (byte1 != byte2) {
				printf("byte at 0x%08lx (0x%02x) "
					"!= byte at 0x%08lx (0x%02x)\n",
					addr1, byte1, addr2, byte2);
				rcode = 1;
				break;
			}
		}
		ngood++;
		addr1 += size;
		addr2 += size;
	}

	printf("Total of %ld %s%s were the same\n",
		ngood, size == 4 ? "word" : size == 2 ? "halfword" : "byte",
		ngood == 1 ? "" : "s");
	return rcode;
}

int do_mem_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, dest, count;
	int	size;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	dest = simple_strtoul(argv[2], NULL, 16);
	dest += base_address;

	count = simple_strtoul(argv[3], NULL, 16);

	if (count == 0) {
		puts ("Zero length ???\n");
		return 1;
	}

#ifndef CFG_NO_FLASH
	/* check if we are copying to Flash */
	if ( (addr2info(dest) != NULL)
#ifndef CONFIG_SUPPORT_PFLASH_AND_SFLASH
#ifdef CONFIG_HAS_DATAFLASH
	   && (!addr_dataflash(dest))
#endif
#else
		&& (addr_flash(dest))
#endif
	   ) {
		int rc;

		puts ("Copy to Flash... ");

		rc = flash_write ((char *)addr, dest, count*size);
		if (rc != 0) {
			flash_perror (rc);
			return (1);
		}
		puts ("done\n");
		return 0;
	}
#endif

#if defined(CONFIG_CMD_MMC)
	if (mmc2info(dest)) {
		int rc;

		puts ("Copy to MMC... ");
		switch (rc = mmc_write ((uchar *)addr, dest, count*size)) {
		case 0:
			putc ('\n');
			return 1;
		case -1:
			puts ("failed\n");
			return 1;
		default:
			printf ("%s[%d] FIXME: rc=%d\n",__FILE__,__LINE__,rc);
			return 1;
		}
		puts ("done\n");
		return 0;
	}

	if (mmc2info(addr)) {
		int rc;

		puts ("Copy from MMC... ");
		switch (rc = mmc_read (addr, (uchar *)dest, count*size)) {
		case 0:
			putc ('\n');
			return 1;
		case -1:
			puts ("failed\n");
			return 1;
		default:
			printf ("%s[%d] FIXME: rc=%d\n",__FILE__,__LINE__,rc);
			return 1;
		}
		puts ("done\n");
		return 0;
	}
#endif

#ifdef CONFIG_HAS_DATAFLASH
	/* Check if we are copying from RAM or Flash to DataFlash */
	if (addr_dataflash(dest) && !addr_dataflash(addr)){
		int rc;

		puts ("Copy to DataFlash... ");

		rc = write_dataflash (dest, addr, count*size);

		if (rc != 1) {
			dataflash_perror (rc);
			return (1);
		}
		puts ("done\n");
		return 0;
	}

	/* Check if we are copying from DataFlash to RAM */
	if (addr_dataflash(addr) && !addr_dataflash(dest)
#ifndef CFG_NO_FLASH
				 && (addr2info(dest) == NULL)
#endif
	   ){
		int rc;
		rc = read_dataflash(addr, count * size, (char *) dest);
		if (rc != 1) {
			dataflash_perror (rc);
			return (1);
		}
		return 0;
	}

	if (addr_dataflash(addr) && addr_dataflash(dest)){
		puts ("Unsupported combination of source/destination.\n\r");
		return 1;
	}
#endif

#ifdef CONFIG_BLACKFIN
	/* See if we're copying to/from L1 inst */
	if (addr_bfin_on_chip_mem(dest) || addr_bfin_on_chip_mem(addr)) {
		memcpy((void *)dest, (void *)addr, count * size);
		return 0;
	}
#endif

	while (count-- > 0) {
		if (size == 4)
			*((ulong  *)dest) = *((ulong  *)addr);
		else if (size == 2)
			*((ushort *)dest) = *((ushort *)addr);
		else
			*((u_char *)dest) = *((u_char *)addr);
		addr += size;
		dest += size;
	}
	return 0;
}

int do_mem_base (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc > 1) {
		/* Set new base address.
		*/
		base_address = simple_strtoul(argv[1], NULL, 16);
	}
	/* Print the current base address.
	*/
	printf("Base Address: 0x%08lx\n", base_address);
	return 0;
}

int do_mem_loop (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, length, i, junk;
	int	size;
	volatile uint	*longp;
	volatile ushort *shortp;
	volatile u_char	*cp;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a size spefication.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	/* Address is always specified.
	*/
	addr = simple_strtoul(argv[1], NULL, 16);

	/* Length is the number of objects, not number of bytes.
	*/
	length = simple_strtoul(argv[2], NULL, 16);

	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */
	if (length == 1) {
		if (size == 4) {
			longp = (uint *)addr;
			for (;;)
				i = *longp;
		}
		if (size == 2) {
			shortp = (ushort *)addr;
			for (;;)
				i = *shortp;
		}
		cp = (u_char *)addr;
		for (;;)
			i = *cp;
	}

	if (size == 4) {
		for (;;) {
			longp = (uint *)addr;
			i = length;
			while (i-- > 0)
				junk = *longp++;
		}
	}
	if (size == 2) {
		for (;;) {
			shortp = (ushort *)addr;
			i = length;
			while (i-- > 0)
				junk = *shortp++;
		}
	}
	for (;;) {
		cp = (u_char *)addr;
		i = length;
		while (i-- > 0)
			junk = *cp++;
	}
}

#ifdef CONFIG_LOOPW
int do_mem_loopw (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, length, i, data;
	int	size;
	volatile uint	*longp;
	volatile ushort *shortp;
	volatile u_char	*cp;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a size spefication.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	/* Address is always specified.
	*/
	addr = simple_strtoul(argv[1], NULL, 16);

	/* Length is the number of objects, not number of bytes.
	*/
	length = simple_strtoul(argv[2], NULL, 16);

	/* data to write */
	data = simple_strtoul(argv[3], NULL, 16);

	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */
	if (length == 1) {
		if (size == 4) {
			longp = (uint *)addr;
			for (;;)
				*longp = data;
					}
		if (size == 2) {
			shortp = (ushort *)addr;
			for (;;)
				*shortp = data;
		}
		cp = (u_char *)addr;
		for (;;)
			*cp = data;
	}

	if (size == 4) {
		for (;;) {
			longp = (uint *)addr;
			i = length;
			while (i-- > 0)
				*longp++ = data;
		}
	}
	if (size == 2) {
		for (;;) {
			shortp = (ushort *)addr;
			i = length;
			while (i-- > 0)
				*shortp++ = data;
		}
	}
	for (;;) {
		cp = (u_char *)addr;
		i = length;
		while (i-- > 0)
			*cp++ = data;
	}
}
#endif /* CONFIG_LOOPW */


/*
 * Perform a memory test. A more complete alternative test can be
 * configured using CFG_ALT_MEMTEST. The complete test loops until
 * interrupted by ctrl-c or by a failure of one of the sub-tests.
 */
int do_mem_mtest (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	vu_long	*addr, *start, *end;
	ulong	val;
	ulong	readback;
	int     rcode = 0;

#if defined(CFG_ALT_MEMTEST)
	vu_long	len;
	vu_long	offset;
	vu_long	test_offset;
	vu_long	pattern;
	vu_long	temp;
	vu_long	anti_pattern;
	vu_long	num_words;
#if defined(CFG_MEMTEST_SCRATCH)
	vu_long *dummy = (vu_long*)CFG_MEMTEST_SCRATCH;
#else
	vu_long *dummy = 0;	/* yes, this is address 0x0, not NULL */
#endif
	int	j;
	int iterations = 1;

	static const ulong bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xaaaaaaaa,	/* alternating 1/0 */
	};
#else
	ulong	incr;
	ulong	pattern;
#endif

	if (argc > 1) {
		start = (ulong *)simple_strtoul(argv[1], NULL, 16);
	} else {
		start = (ulong *)CFG_MEMTEST_START;
	}

	if (argc > 2) {
		end = (ulong *)simple_strtoul(argv[2], NULL, 16);
	} else {
		end = (ulong *)(CFG_MEMTEST_END);
	}

	if (argc > 3) {
		pattern = (ulong)simple_strtoul(argv[3], NULL, 16);
	} else {
		pattern = 0;
	}

#if defined(CFG_ALT_MEMTEST)
	printf ("Testing %08x ... %08x:\n", (uint)start, (uint)end);
	PRINTF("%s:%d: start 0x%p end 0x%p\n",
		__FUNCTION__, __LINE__, start, end);

	for (;;) {
		if (ctrlc()) {
			putc ('\n');
			return 1;
		}

		printf("Iteration: %6d\r", iterations);
		PRINTF("Iteration: %6d\n", iterations);
		iterations++;

		/*
		 * Data line test: write a pattern to the first
		 * location, write the 1's complement to a 'parking'
		 * address (changes the state of the data bus so a
		 * floating bus doen't give a false OK), and then
		 * read the value back. Note that we read it back
		 * into a variable because the next time we read it,
		 * it might be right (been there, tough to explain to
		 * the quality guys why it prints a failure when the
		 * "is" and "should be" are obviously the same in the
		 * error message).
		 *
		 * Rather than exhaustively testing, we test some
		 * patterns by shifting '1' bits through a field of
		 * '0's and '0' bits through a field of '1's (i.e.
		 * pattern and ~pattern).
		 */
		addr = start;
		for (j = 0; j < sizeof(bitpattern)/sizeof(bitpattern[0]); j++) {
		    val = bitpattern[j];
		    for(; val != 0; val <<= 1) {
			*addr  = val;
			*dummy  = ~val; /* clear the test data off of the bus */
			readback = *addr;
			if(readback != val) {
			     printf ("FAILURE (data line): "
				"expected %08lx, actual %08lx\n",
					  val, readback);
			}
			*addr  = ~val;
			*dummy  = val;
			readback = *addr;
			if(readback != ~val) {
			    printf ("FAILURE (data line): "
				"Is %08lx, should be %08lx\n",
					readback, ~val);
			}
		    }
		}

		/*
		 * Based on code whose Original Author and Copyright
		 * information follows: Copyright (c) 1998 by Michael
		 * Barr. This software is placed into the public
		 * domain and may be used for any purpose. However,
		 * this notice must not be changed or removed and no
		 * warranty is either expressed or implied by its
		 * publication or distribution.
		 */

		/*
		 * Address line test
		 *
		 * Description: Test the address bus wiring in a
		 *              memory region by performing a walking
		 *              1's test on the relevant bits of the
		 *              address and checking for aliasing.
		 *              This test will find single-bit
		 *              address failures such as stuck -high,
		 *              stuck-low, and shorted pins. The base
		 *              address and size of the region are
		 *              selected by the caller.
		 *
		 * Notes:	For best results, the selected base
		 *              address should have enough LSB 0's to
		 *              guarantee single address bit changes.
		 *              For example, to test a 64-Kbyte
		 *              region, select a base address on a
		 *              64-Kbyte boundary. Also, select the
		 *              region size as a power-of-two if at
		 *              all possible.
		 *
		 * Returns:     0 if the test succeeds, 1 if the test fails.
		 */
		len = ((ulong)end - (ulong)start)/sizeof(vu_long);
		pattern = (vu_long) 0xaaaaaaaa;
		anti_pattern = (vu_long) 0x55555555;

		PRINTF("%s:%d: length = 0x%.8lx\n",
			__FUNCTION__, __LINE__,
			len);
		/*
		 * Write the default pattern at each of the
		 * power-of-two offsets.
		 */
		for (offset = 1; offset < len; offset <<= 1) {
			start[offset] = pattern;
		}

		/*
		 * Check for address bits stuck high.
		 */
		test_offset = 0;
		start[test_offset] = anti_pattern;

		for (offset = 1; offset < len; offset <<= 1) {
		    temp = start[offset];
		    if (temp != pattern) {
			printf ("\nFAILURE: Address bit stuck high @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx\n",
				(ulong)&start[offset], pattern, temp);
			return 1;
		    }
		}
		start[test_offset] = pattern;
		WATCHDOG_RESET();

		/*
		 * Check for addr bits stuck low or shorted.
		 */
		for (test_offset = 1; test_offset < len; test_offset <<= 1) {
		    start[test_offset] = anti_pattern;

		    for (offset = 1; offset < len; offset <<= 1) {
			temp = start[offset];
			if ((temp != pattern) && (offset != test_offset)) {
			    printf ("\nFAILURE: Address bit stuck low or shorted @"
				" 0x%.8lx: expected 0x%.8lx, actual 0x%.8lx\n",
				(ulong)&start[offset], pattern, temp);
			    return 1;
			}
		    }
		    start[test_offset] = pattern;
		}

		/*
		 * Description: Test the integrity of a physical
		 *		memory device by performing an
		 *		increment/decrement test over the
		 *		entire region. In the process every
		 *		storage bit in the device is tested
		 *		as a zero and a one. The base address
		 *		and the size of the region are
		 *		selected by the caller.
		 *
		 * Returns:     0 if the test succeeds, 1 if the test fails.
		 */
		num_words = ((ulong)end - (ulong)start)/sizeof(vu_long) + 1;

		/*
		 * Fill memory with a known pattern.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
			WATCHDOG_RESET();
			start[offset] = pattern;
		}

		/*
		 * Check each location and invert it for the second pass.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		    WATCHDOG_RESET();
		    temp = start[offset];
		    if (temp != pattern) {
			printf ("\nFAILURE (read/write) @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				(ulong)&start[offset], pattern, temp);
			return 1;
		    }

		    anti_pattern = ~pattern;
		    start[offset] = anti_pattern;
		}

		/*
		 * Check each location for the inverted pattern and zero it.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		    WATCHDOG_RESET();
		    anti_pattern = ~pattern;
		    temp = start[offset];
		    if (temp != anti_pattern) {
			printf ("\nFAILURE (read/write): @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				(ulong)&start[offset], anti_pattern, temp);
			return 1;
		    }
		    start[offset] = 0;
		}
	}

#else /* The original, quickie test */
	incr = 1;
	for (;;) {
		if (ctrlc()) {
			putc ('\n');
			return 1;
		}

		printf ("\rPattern %08lX  Writing..."
			"%12s"
			"\b\b\b\b\b\b\b\b\b\b",
			pattern, "");

		for (addr=start,val=pattern; addr<end; addr++) {
			WATCHDOG_RESET();
			*addr = val;
			val  += incr;
		}

		puts ("Reading...");

		for (addr=start,val=pattern; addr<end; addr++) {
			WATCHDOG_RESET();
			readback = *addr;
			if (readback != val) {
				printf ("\nMem error @ 0x%08X: "
					"found %08lX, expected %08lX\n",
					(uint)addr, readback, val);
				rcode = 1;
			}
			val += incr;
		}

		/*
		 * Flip the pattern each time to make lots of zeros and
		 * then, the next time, lots of ones.  We decrement
		 * the "negative" patterns and increment the "positive"
		 * patterns to preserve this feature.
		 */
		if(pattern & 0x80000000) {
			pattern = -pattern;	/* complement & increment */
		}
		else {
			pattern = ~pattern;
		}
		incr = -incr;
	}
#endif
	return rcode;
}


/* Modify memory.
 *
 * Syntax:
 *	mm{.b, .w, .l} {addr}
 *	nm{.b, .w, .l} {addr}
 */
static int
mod_mem(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[])
{
	ulong	addr, i;
	int	nbytes, size;
	extern char console_buffer[];

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

#ifdef CONFIG_BOOT_RETRY_TIME
	reset_cmd_timeout();	/* got a good command to get here */
#endif
	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = mm_last_addr;
	size = mm_last_size;

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* New command specified.  Check for a size specification.
		 * Defaults to long if no or incorrect specification.
		 */
		if ((size = cmd_get_data_size(argv[0], 4)) < 0)
			return 1;

		/* Address is specified since argc > 1
		*/
		addr = simple_strtoul(argv[1], NULL, 16);
		addr += base_address;
	}

#ifdef CONFIG_HAS_DATAFLASH
	if (addr_dataflash(addr)){
		puts ("Can't modify DataFlash in place. Use cp instead.\n\r");
		return 0;
	}
#endif

#ifdef CONFIG_BLACKFIN
	if (addr_bfin_on_chip_mem(addr)) {
		puts ("Can't modify L1 instruction in place. Use cp instead.\n\r");
		return 0;
	}
#endif

	/* Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		printf("%08lx:", addr);
		if (size == 4)
			printf(" %08x", *((uint   *)addr));
		else if (size == 2)
			printf(" %04x", *((ushort *)addr));
		else
			printf(" %02x", *((u_char *)addr));

		nbytes = readline (" ? ");
		if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
			/* <CR> pressed as only input, don't modify current
			 * location and move to next. "-" pressed will go back.
			 */
			if (incrflag)
				addr += nbytes ? -size : size;
			nbytes = 1;
#ifdef CONFIG_BOOT_RETRY_TIME
			reset_cmd_timeout(); /* good enough to not time out */
#endif
		}
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (nbytes == -2) {
			break;	/* timed out, exit the command	*/
		}
#endif
		else {
			char *endp;
			i = simple_strtoul(console_buffer, &endp, 16);
			nbytes = endp - console_buffer;
			if (nbytes) {
#ifdef CONFIG_BOOT_RETRY_TIME
				/* good enough to not time out
				 */
				reset_cmd_timeout();
#endif
				if (size == 4)
					*((uint   *)addr) = i;
				else if (size == 2)
					*((ushort *)addr) = i;
				else
					*((u_char *)addr) = i;
				if (incrflag)
					addr += size;
			}
		}
	} while (nbytes);

	mm_last_addr = addr;
	mm_last_size = size;
	return 0;
}

#ifndef CONFIG_CRC32_VERIFY

int do_mem_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong addr, length;
	ulong crc;
	ulong *ptr;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul (argv[1], NULL, 16);
	addr += base_address;

	length = simple_strtoul (argv[2], NULL, 16);

	crc = crc32 (0, (const uchar *) addr, length);

	printf ("CRC32 for %08lx ... %08lx ==> %08lx\n",
			addr, addr + length - 1, crc);

	if (argc > 3) {
		ptr = (ulong *) simple_strtoul (argv[3], NULL, 16);
		*ptr = crc;
	}

	return 0;
}

#else	/* CONFIG_CRC32_VERIFY */

int do_mem_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong addr, length;
	ulong crc;
	ulong *ptr;
	ulong vcrc;
	int verify;
	int ac;
	char **av;

	if (argc < 3) {
  usage:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	av = argv + 1;
	ac = argc - 1;
	if (strcmp(*av, "-v") == 0) {
		verify = 1;
		av++;
		ac--;
		if (ac < 3)
			goto usage;
	} else
		verify = 0;

	addr = simple_strtoul(*av++, NULL, 16);
	addr += base_address;
	length = simple_strtoul(*av++, NULL, 16);

	crc = crc32(0, (const uchar *) addr, length);

	if (!verify) {
		printf ("CRC32 for %08lx ... %08lx ==> %08lx\n",
				addr, addr + length - 1, crc);
		if (ac > 2) {
			ptr = (ulong *) simple_strtoul (*av++, NULL, 16);
			*ptr = crc;
		}
	} else {
		vcrc = simple_strtoul(*av++, NULL, 16);
		if (vcrc != crc) {
			printf ("CRC32 for %08lx ... %08lx ==> %08lx != %08lx ** ERROR **\n",
					addr, addr + length - 1, crc, vcrc);
			return 1;
		}
	}

	return 0;

}
#endif	/* CONFIG_CRC32_VERIFY */


#ifdef CONFIG_CMD_UNZIP
int  gunzip (void *, int, unsigned char *, unsigned long *);

int do_unzip ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned long src, dst;
	unsigned long src_len = ~0UL, dst_len = ~0UL;
	int err;

	switch (argc) {
		case 4:
			dst_len = simple_strtoul(argv[3], NULL, 16);
			/* fall through */
		case 3:
			src = simple_strtoul(argv[1], NULL, 16);
			dst = simple_strtoul(argv[2], NULL, 16);
			break;
		default:
			printf ("Usage:\n%s\n", cmdtp->usage);
			return 1;
	}

	return !!gunzip((void *) dst, dst_len, (void *) src, &src_len);
}
#endif /* CONFIG_CMD_UNZIP */


/**************************************************/
U_BOOT_CMD(
	md,	3,	1,	do_mem_md,
	"md	- memory display\n",
	"[.b, .w, .l] address [# of objects]\n	  - memory display\n"
);


U_BOOT_CMD(
	mm,	2,	1,	do_mem_mm,
	"mm	- memory modify (auto-incrementing)\n",
	"[.b, .w, .l] address\n" "    - memory modify, auto increment address\n"
);


U_BOOT_CMD(
	nm,	2,	1,	do_mem_nm,
	"nm	- memory modify (constant address)\n",
	"[.b, .w, .l] address\n    - memory modify, read and keep address\n"
);

U_BOOT_CMD(
	mw,	4,	1,	do_mem_mw,
	"mw	- memory write (fill)\n",
	"[.b, .w, .l] address value [count]\n	- write memory\n"
);

U_BOOT_CMD(
	cp,	4,	1,	do_mem_cp,
	"cp	- memory copy\n",
	"[.b, .w, .l] source target count\n    - copy memory\n"
);

U_BOOT_CMD(
	cmp,	4,	1,	do_mem_cmp,
	"cmp	- memory compare\n",
	"[.b, .w, .l] addr1 addr2 count\n    - compare memory\n"
);

#ifndef CONFIG_CRC32_VERIFY

U_BOOT_CMD(
	crc32,	4,	1,	do_mem_crc,
	"crc32	- checksum calculation\n",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]\n"
);

#else	/* CONFIG_CRC32_VERIFY */

U_BOOT_CMD(
	crc32,	5,	1,	do_mem_crc,
	"crc32	- checksum calculation\n",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]\n"
	"-v address count crc\n    - verify crc of memory area\n"
);

#endif	/* CONFIG_CRC32_VERIFY */

U_BOOT_CMD(
	base,	2,	1,	do_mem_base,
	"base	- print or set address offset\n",
	"\n    - print address offset for memory commands\n"
	"base off\n    - set address offset for memory commands to 'off'\n"
);

U_BOOT_CMD(
	loop,	3,	1,	do_mem_loop,
	"loop	- infinite loop on address range\n",
	"[.b, .w, .l] address number_of_objects\n"
	"    - loop on a set of addresses\n"
);

#ifdef CONFIG_LOOPW
U_BOOT_CMD(
	loopw,	4,	1,	do_mem_loopw,
	"loopw	- infinite write loop on address range\n",
	"[.b, .w, .l] address number_of_objects data_to_write\n"
	"    - loop on a set of addresses\n"
);
#endif /* CONFIG_LOOPW */

U_BOOT_CMD(
	mtest,	4,	1,	do_mem_mtest,
	"mtest	- simple RAM test\n",
	"[start [end [pattern]]]\n"
	"    - simple RAM read/write test\n"
);
#if 1
U_BOOT_CMD(
	memtest_cns3000,	4,	1,	do_memtest_cns3000,
	"memtest_cns3000	- cns3000 RAM test\n",
#if 0
	"[start size]\n"
#else
	"[start end] [count]\n"
#endif
	"    - vega    EVB2X memory test\n"
	"    - cns3000 memory test\n"
);
#endif
#ifdef CONFIG_MX_CYCLIC
U_BOOT_CMD(
	mdc,	4,	1,	do_mem_mdc,
	"mdc	- memory display cyclic\n",
	"[.b, .w, .l] address count delay(ms)\n    - memory display cyclic\n"
);

U_BOOT_CMD(
	mwc,	4,	1,	do_mem_mwc,
	"mwc	- memory write cyclic\n",
	"[.b, .w, .l] address value delay(ms)\n    - memory write cyclic\n"
);
#endif /* CONFIG_MX_CYCLIC */

#ifdef CONFIG_CMD_UNZIP
U_BOOT_CMD(
	unzip,	4,	1,	do_unzip,
	"unzip - unzip a memory region\n",
	"srcaddr dstaddr [dstsize]\n"
);
#endif /* CONFIG_CMD_UNZIP */


