/* ----------------------------------------------------------------------------
 *         Microchip Microprocessor (MPU) Software Team
 * ----------------------------------------------------------------------------
 * Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Microchip's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY MICROCHIP "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL MICROCHIP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "common.h"
#include "hardware.h"
#include "arch/at91_sfr.h"
#include "arch/at91_rstc.h"
#include "arch/at91_pmc/pmc.h"
#include "arch/at91_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_ddrsdrc.h"
#include "gpio.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "ddramc.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "sam9x60_board.h"
#include "board_hw_info.h"
#include "twi.h"
#include "flexcom.h"
#include "board.h"

#define PLLA_DIV 1
#define PLLA_COUNT 0x3f
#define PLLA_LOOP_FILTER 0
#define PLLA_CLOCK 200000000
#define PLLA_FRACR(_p, _q) \
	((unsigned int)((((unsigned long)(_p)) << 22) / (_q)))

const unsigned int usart_base[] = {
		AT91C_BASE_DBGU, AT91C_BASE_FLEXCOM0, AT91C_BASE_FLEXCOM1, AT91C_BASE_FLEXCOM2,
		AT91C_BASE_FLEXCOM3, AT91C_BASE_FLEXCOM4, AT91C_BASE_FLEXCOM5, AT91C_BASE_FLEXCOM6,
		AT91C_BASE_FLEXCOM7, AT91C_BASE_FLEXCOM8, AT91C_BASE_FLEXCOM9, AT91C_BASE_FLEXCOM10,
		AT91C_BASE_FLEXCOM11, AT91C_BASE_FLEXCOM12 };

static void at91_dbgu_hw_init(void)
{
	const struct pio_desc dbgu_pins[][3] = {
		{ /* DBGU */
			{"RXD", AT91C_PIN_PA(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"TXD", AT91C_PIN_PA(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ /* FLEXCOM0 */
			{"FLEXCOM0_RXD", AT91C_PIN_PA(1), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLEXCOM0_TXD", AT91C_PIN_PA(0), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM1 */
			{"FLEXCOM1_RXD", AT91C_PIN_PA(6), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLEXCOM1_TXD", AT91C_PIN_PA(5), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM2 */
			{"FLEXCOM2_RXD", AT91C_PIN_PA(8), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLEXCOM2_TXD", AT91C_PIN_PA(7), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM3 */
			{"FLEXCOM3_RXD", AT91C_PIN_PC(23), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLEXCOM3_TXD", AT91C_PIN_PC(22), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM4 */
			{"FLEXCOM4_RXD", AT91C_PIN_PA(11), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLEXCOM4_TXD", AT91C_PIN_PA(12), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM5 */
			{"FLEXCOM5_RXD", AT91C_PIN_PA(21), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLEXCOM5_TXD", AT91C_PIN_PA(22), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM6 */
			{"FLEXCOM6_RXD", AT91C_PIN_PA(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLEXCOM6_TXD", AT91C_PIN_PA(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM7 */
			{"FLEXCOM7_RXD", AT91C_PIN_PC(1), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLEXCOM7_TXD", AT91C_PIN_PC(0), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM8 */
			{"FLEXCOM8_RXD", AT91C_PIN_PB(5), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLEXCOM8_TXD", AT91C_PIN_PB(4), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM9 */
			{"FLEXCOM9_RXD", AT91C_PIN_PC(9), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLEXCOM9_TXD", AT91C_PIN_PC(8), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM10 */
			{"FLEXCOM10_RXD", AT91C_PIN_PC(17), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLEXCOM10_TXD", AT91C_PIN_PC(16), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM11 */
			{"FLEXCOM11_RXD", AT91C_PIN_PB(20), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLEXCOM11_TXD", AT91C_PIN_PB(19), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
		{ /* FLEXCOM12 */
			{"FLEXCOM12_RXD", AT91C_PIN_PB(22), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLEXCOM12_TXD", AT91C_PIN_PB(21), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A}, 
		},
	};
	const unsigned int periph_id[] = {
		AT91C_ID_DBGU, AT91C_ID_FLEXCOM0, AT91C_ID_FLEXCOM1, AT91C_ID_FLEXCOM2,
		AT91C_ID_FLEXCOM3, AT91C_ID_FLEXCOM4, AT91C_ID_FLEXCOM5, AT91C_ID_FLEXCOM6,
		AT91C_ID_FLEXCOM7, AT91C_ID_FLEXCOM8, AT91C_ID_FLEXCOM9, AT91C_ID_FLEXCOM10,
		AT91C_ID_FLEXCOM11, AT91C_ID_FLEXCOM12 };

	pio_configure(dbgu_pins[CONFIG_CONSOLE_INDEX]);
	pmc_enable_periph_clock(periph_id[CONFIG_CONSOLE_INDEX]);
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();
	usart_init(BAUDRATE(MASTER_CLOCK, BAUD_RATE));
}

static void at91_green_led_on(void)
{
	pio_set_gpio_output(AT91C_PIN_PB(13), 0);
	pio_set_gpio_output(AT91C_PIN_PB(11), 0);
	pio_set_gpio_output(AT91C_PIN_PB(12), 1);
}

#if defined(CONFIG_FLEXCOM)
unsigned int at91_flexcom0_init(void)
{
	const struct pio_desc flx_pins[] = {
		{"FLX_IO0", AT91C_PIN_PA(0), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLX_IO1", AT91C_PIN_PA(1), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(flx_pins);
	pmc_enable_periph_clock(AT91C_ID_FLEXCOM0);

	flexcom_init(0);

	return flexcom_get_regmap(0);
}
#endif

#if defined CONFIG_TWI
void twi_init()
{
#if defined(CONFIG_FLEXCOM)
       twi_bus_init(at91_flexcom0_init);
#endif
#if defined(CONFIG_AUTOCONFIG_TWI_BUS)
	dbg_loud("Auto-Config the TWI Bus by the board\n");
	at91_board_config_twi_bus();
#endif
}
#endif

#if defined(CONFIG_AUTOCONFIG_TWI_BUS)
void at91_board_config_twi_bus(void)
{
	at24xx_twi_bus = 0;
}
#endif
#if defined CONFIG_FLEXCOM
static struct at91_flexcom flexcoms[] = {
#if defined CONFIG_TWI
	{AT91C_ID_FLEXCOM0, FLEXCOM_TWI, AT91C_BASE_FLEXCOM0, 0},
#endif
};

void board_flexcoms_init()
{
	flexcoms_init(flexcoms);
}
#endif

void wilc_pwrseq()
{
	const struct pio_desc wilc_down_pins[] = {
		{"WILC_RESETN", AT91C_PIN_PB(25), 0, PIO_DEFAULT, PIO_OUTPUT},
		{"WILC_EN", AT91C_PIN_PA(29), 0, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	const struct pio_desc wilc_powerup_pins[] = {
		{"WILC_EN", AT91C_PIN_PA(29), 1, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	const struct pio_desc wilc_en_pins[] = {
		{"WILC_RESETN", AT91C_PIN_PB(25), 1, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(wilc_down_pins);
	udelay(5000);
	pio_configure(wilc_powerup_pins);
	udelay(5000);
	pio_configure(wilc_en_pins);
}

void hw_init(void)
{
	struct pmc_pll_cfg plla_config;

	/* Disable watchdog */
	at91_disable_wdt();

	/* Green Led ON */
	at91_green_led_on();

	/* Configure & Enable PLLA */
	plla_config.mul = 49;
	plla_config.div = PLLA_DIV;
	plla_config.count = PLLA_COUNT;
	plla_config.fracr = 0;
	plla_config.loop_filter = PLLA_LOOP_FILTER;
	pmc_sam9x60_cfg_pll(PLL_ID_PLLA, &plla_config);

	pmc_cfg_mck(BOARD_PRESCALER_PLLA);

	/* Initialize dbgu */
	initialize_dbgu();

	/* Enable External Reset */
	writel(AT91C_RSTC_KEY_UNLOCK | AT91C_RSTC_URSTEN, AT91C_BASE_RSTC + RSTC_RMR);

	/* Init timer */
	timer_init();

#ifdef CONFIG_FLEXCOM
	board_flexcoms_init();
#endif

#ifdef CONFIG_TWI
	twi_init();
#endif

	/* Initialize DDRAM Controller */
	ddram_init();

	/* Perform the WILC initialization sequence */
	wilc_pwrseq();
}

#ifdef CONFIG_DATAFLASH

#if defined(CONFIG_QSPI)
void at91_qspi_hw_init(void)
{
	const struct pio_desc qspi_pins[] = {
		{"QSPI0_SCK", AT91C_PIN_PB(19), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"QSPI0_CS", AT91C_PIN_PB(20), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"QSPI0_IO0", AT91C_PIN_PB(21), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"QSPI0_IO1", AT91C_PIN_PB(22), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"QSPI0_IO2", AT91C_PIN_PB(23), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"QSPI0_IO3", AT91C_PIN_PB(24), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(qspi_pins);

	pmc_enable_system_clock(AT91C_PMC_QSPICLK);
	pmc_enable_periph_clock(CONFIG_SYS_ID_QSPI);
}
#endif  /* #ifdef CONFIG_QSPI */

#endif	/* #ifdef CONFIG_DATAFLASH */

#ifdef CONFIG_SDCARD
#ifdef CONFIG_OF_LIBFDT
void at91_board_set_dtb_name(char *of_name)
{
	strcpy(of_name, "at91-sam9x60ek.dtb");
}
#endif

#define ATMEL_SDHC_GCKDIV_VALUE     5

void at91_sdhc_hw_init(void)
{
#ifdef CONFIG_SDHC0
	const struct pio_desc sdmmc_pins[] = {
		{"SDMMC0_CMD",	AT91C_PIN_PA(16), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_CK",	AT91C_PIN_PA(17), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT0",	AT91C_PIN_PA(15), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT1",	AT91C_PIN_PA(18), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT2",	AT91C_PIN_PA(19), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT3",	AT91C_PIN_PA(20), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};
	pio_configure(sdmmc_pins);

	pmc_enable_periph_clock(CONFIG_SYS_ID_SDHC);
	pmc_enable_generic_clock(CONFIG_SYS_ID_SDHC,
				 GCK_CSS_PLLA_CLK,
				 ATMEL_SDHC_GCKDIV_VALUE);
#endif
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_NANDFLASH
void nandflash_hw_init(void)
{
	unsigned int reg;
	const struct pio_desc nand_pins[] = {
		{"NANDOE", CONFIG_SYS_NAND_OE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDWE", CONFIG_SYS_NAND_WE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDALE", CONFIG_SYS_NAND_ALE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDCLE", CONFIG_SYS_NAND_CLE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDCS", CONFIG_SYS_NAND_ENABLE_PIN, 1, PIO_DEFAULT,
		 PIO_OUTPUT},
		{"D0", AT91C_PIN_PD(6), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D1", AT91C_PIN_PD(7), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D2", AT91C_PIN_PD(8), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D3", AT91C_PIN_PD(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D4", AT91C_PIN_PD(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D5", AT91C_PIN_PD(11), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D6", AT91C_PIN_PD(12), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D7", AT91C_PIN_PD(13), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(nand_pins);
	pmc_enable_periph_clock(AT91C_ID_PIOD);

	reg = readl(AT91C_BASE_SFR + SFR_CCFG_EBICSA);
	reg |= AT91C_EBI_CS3A_SM | AT91C_EBI_NFD0_ON_D16;
	reg &= ~AT91C_EBI_DRV;
	writel(reg, AT91C_BASE_SFR + SFR_CCFG_EBICSA);

	/* Configure SMC CS3 for NAND */
	writel(AT91C_SMC_NWESETUP_(4), AT91C_BASE_SMC + SMC_SETUP3);

	writel(AT91C_SMC_NWEPULSE_(10) | AT91C_SMC_NCS_WRPULSE_(20) |
	       AT91C_SMC_NRDPULSE_(10) | AT91C_SMC_NCS_RDPULSE_(20),
	       AT91C_BASE_SMC + SMC_PULSE3);

	writel(AT91C_SMC_NWECYCLE_(20) | AT91C_SMC_NRDCYCLE_(20),
	       AT91C_BASE_SMC + SMC_CYCLE3);

	writel(AT91C_SMC_READMODE | AT91C_SMC_WRITEMODE | AT91C_SMC_TDFEN |
	       AT91_SMC_TDF_(15), AT91C_BASE_SMC + SMC_CTRL3);
}
#endif /* #ifdef CONFIG_NANDFLASH */