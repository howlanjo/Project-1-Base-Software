/* msp430_spi.c
 * Library for performing SPI I/O on a wide range of MSP430 chips.
 *
 * Serial interfaces supported:
 * 1. USI - developed on MSP430G2231
 * 2. USCI_A - developed on MSP430G2553
 * 3. USCI_B - developed on MSP430G2553
 * 4. USCI_A F5xxx - developed on MSP430F5172, added F5529
 * 5. USCI_B F5xxx - developed on MSP430F5172, added F5529
 *
 * Copyright (c) 2013, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <msp432.h>
#include "msp430_spi.h"
#include "nrf_userconfig.h"
#include "spi.h"
#include "gpio.h"

const eUSCI_SPI_MasterConfig spiMasterConfig =
{
        EUSCI_A_SPI_CLOCKSOURCE_SMCLK,             // SMCLK Clock Source
        3000000,                                   // SMCLK 3Mhz
        500000,                                    // SPICLK = 500khz
        EUSCI_A_SPI_MSB_FIRST,                     // MSB First
        EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,    // Phase
        EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW, // High polarity
        EUSCI_A_SPI_3PIN                           // 3Wire SPI Mode
};

void spi_init()
{
	// Selecting P1.5 P1.6 and P1.7 in SPI mode
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10,
			GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// CS setup.
	GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7);
	GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN7);

	// Configuring SPI in 3wire master mode
	SPI_initMaster(EUSCI_B3_MODULE, &spiMasterConfig);

	//Enable SPI module
	SPI_enableModule(EUSCI_B3_MODULE);
}

uint8_t spi_transfer(uint8_t inb)
{
	UCB3TXBUF = inb;
	while ( !(UCB3IFG & UCRXIFG) )  // Wait for RXIFG indicating remote byte received via SOMI
		;
	return UCB3RXBUF;
}

uint16_t spi_transfer16(uint16_t inw)
{
	uint16_t retw;
	uint8_t *retw8 = (uint8_t *)&retw, *inw8 = (uint8_t *)&inw;
	UCB3TXBUF = inw8[1];
	while ( !(UCB3IFG & UCRXIFG) )
		;
	retw8[1] = UCB3RXBUF;
	UCB3TXBUF = inw8[0];
	while ( !(UCB3IFG & UCRXIFG) )
		;
	retw8[0] = UCB3RXBUF;
	return retw;
}

