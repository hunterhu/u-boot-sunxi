/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Some board init for the Allwinner A12-evb board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <fastboot.h>
#include <asm/arch/nand_fspart.h>
#include <asm/arch/nand_bsp.h>
#include <asm/arch/android_misc.h>

DECLARE_GLOBAL_DATA_PTR;

static sunxi_boot_type_t boot_type;
extern sunxi_boot_type_t boot_from(void);

int save_boot_type(void) {

	boot_type = boot_from();
	printf("boot type: %d\n", boot_type);

	return 0;
}

sunxi_boot_type_t get_boot_type(void) {

	return boot_type;
}

void fastboot_partition_init(void)
{
	fastboot_ptentry fb_part;
	int index, part_total;

	printf("--------fastboot partitions--------\n");
	part_total = sunxi_nand_getpart_num();
	printf("-total partitions:%d-\n", part_total);
	printf("%-12s  %-12s  %-12s\n", "-name-", "-start-", "-size-");

	for(index = 0; index < part_total && index < MBR_MAX_PART_COUNT; index++) {
		sunxi_nand_getpart_name(index, &fb_part.name[0]);
		fb_part.start = sunxi_nand_getpart_offset(index) * 512;
		fb_part.length = sunxi_nand_getpart_size(index) * 512;
		fb_part.flags = 0;
		printf("%-12s: %-12x  %-12x\n", fb_part.name, fb_part.start, fb_part.length);
		fastboot_flash_add_ptn(&fb_part);
	}
	printf("-----------------------------------\n");
}

static struct bootloader_message misc_message;

int check_android_misc() {
	loff_t misc_offset = 0, misc_size = 0;
	size_t count = sizeof(misc_message);

	sunxi_nand_getpart_info_byname("misc", &misc_offset, &misc_size);

	if(!misc_offset || !misc_size) {
		sunxi_nand_getpart_info_byname("MISC", &misc_offset, &misc_size);
		if(!misc_offset || !misc_size) {
			puts("no misc partition is found\n");
			return 0;
		}
	}

	sunxi_nand_read_opts(&nand_info[0], misc_offset, &count,
			(u_char *)&misc_message, 0);

#ifdef DEBUG
	printf("misc.command  : %s\n", misc_message.command);
	printf("misc.status   : %s\n", misc_message.status);
	printf("misc.recovery : %s\n", misc_message.recovery);
#endif

	if(!strcmp(misc_message.command, "boot-recovery")) {
		/* there is a recovery command */
		setenv("bootcmd", "run setargs boot_recovery");
		puts("Recovery detected, will boot recovery\n");
		/* android recovery will clean the misc */
	}

	if(!strcmp(misc_message.command, "boot-fastboot")) {
		/* there is a fastboot command */
		setenv("bootcmd", "run setargs boot_fastboot");
		puts("Fastboot detected, will enter fastboot\n");
		/* clean the misc partition ourself */
		memset(&misc_message, 0, sizeof(misc_message));
		sunxi_nand_write_opts(&nand_info[0], misc_offset, &count,
			(u_char *)&misc_message, 0);
	}

	return 0;
}

/* TODO add board specific code here */
int board_init(void)
{
	gd->bd->bi_arch_number = 3495;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;
	return 0;
}

/* Partition init must be after NAND init, so we put the fastboot
 * partition init here in the board late init.
 */
int board_late_init(void)
{
	fastboot_partition_init();
	check_android_misc();
	return 0;
}
void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE);
	return 0;
}

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	sunxi_mmc_init(CONFIG_MMC_SUNXI_SLOT);
	check_android_misc();

	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	puts("Board: A12-EVB\n");
	return 0;
}
#endif
