#include "driverlib.h"
#include "support.h"

int dht_get_temp()
{
  uint16_t temp_temp;
  while (dht_current_state != DHT_IDLE);
  temp_temp = (((dht_data.val.th&0x7f)<<8)+dht_data.val.tl);
  return ((-1)*((dht_data.val.th&0x80)>>7)+temp_temp);
}

int dht_get_rh()
{
  uint16_t temp_rh;
  while (dht_current_state != DHT_IDLE);
  temp_rh = (dht_data.val.hh<<8)+dht_data.val.hl;
  return temp_rh;
}

void
dht_start_read() {
  // First, low pulse of 1ms
  //P2OUT &= ~BIT0;  // *** Timer1 input capture on pin 0 interferes with an SPI port on the MSP432
  //P2SEL &= ~BIT0;
  //P2DIR |= BIT0;
	P7OUT &= ~BIT3;  // *** use Timer0 input capture on pin 0 instead on P7.3 instead
	P7SEL0 &= ~BIT3; // note there are two select bits on the MSP432 for I/O
	P7DIR |= BIT3;  // initially set to output to drive pin low

  TA0CCTL0 &= ~CCIFG;  // clear interrupt flag
//  TA0CCR0 = 16000u; // count for 1 ms at 16 MHz SMCLK clock
  TA0CCR0 = 12000u; // count for 1 ms at 12 MHz SMCLK clock
  TA0CCTL0 = CCIE;
  TA0CTL = TACLR;
  TA0CTL = TASSEL_2 | ID_0 | MC_1;  // use SMCLK for timer, initally used for counting (MC_1) not capture

  /* Enabling interrupt on timer 0, interrupt stucture is different for MSP432 so use driver library */
  MAP_Interrupt_enableInterrupt(INT_TA0_0);

  dht_current_state = DHT_TRIGGERING;
}

// void __attribute__((interrupt (TIMER1_A0_VECTOR)))  *** old style interrupt, now put in startup file

