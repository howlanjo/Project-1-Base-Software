#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <driverlib.h>
#include "ST7735.h"
#include "ClockSystem.h"
#include "msp.h"
#include "support.h"
#include "msprf24.h"
#include "nrf_userconfig.h"
#include "globals.h"


void SSR_Init(void);

void Delay1ms(uint32_t n);
// Subroutine to wait 10 msec
// Inputs: n  number of 10 msec to wait
// Outputs: None
// Notes: ...
void DelayWait10ms(uint32_t n){
  Delay1ms(n*10);
}

int z = 0, Top = 0, date[3], time[3], tick = 0, Setup = 0;
uint8_t Encoder1[2], Encoder2[2], PushButton[2], movement = 0, i, flag;
int16_t dateStringColor[20], timeStringColor[20];
char dateString[20], timeString[20];
char months[12][5], temp[50];
static volatile RTC_C_Calendar newTime, setTime;
int err = NONE;
char addr[5];
char buf[32];
uint8_t data[8];
volatile unsigned int user;
int status;

//-----------------------------------------------------------------------
//
//		THIS CODE HAS THE MANUAL CHIP SELECT FOR THE DISPLAY
//
int main (void)
{
	MAP_WDT_A_holdTimer();

	InitFunction();
	err = DisplayInit();
	err = RF_Init();

	MAP_Interrupt_enableMaster();

    while(1)
    {
        if (status)
        {
        	status = 0;
        	r_rx_payload(3, &data);
        	printf("%d  %d  %d\n", data[0], data[1], data[2]);
        	if(data[0] == '0')
        		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
			else if (data[0] == '1')
				GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);

        	if(data[1] == '0')
				GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
			else if (data[1] == '1')
				GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);

        	if(data[2] == '0')
				GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
			else if (data[2] == '1')
				GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);

        }
    }





//	while(1)
//	{
//
////Displaying current time
////		while(!Setup)
////		{
//			if(tick)
//			{
//				tick = 0;
//				sprintf(dateString, "%s %02X,%X", months[newTime.month], newTime.dayOfmonth, newTime.year);
//				sprintf(timeString, "%02X:%02X:%02X", newTime.hours, newTime.minutes, newTime.seconds);
//
//				for(i = 0; i < strlen(dateString); i++)
//				{
//					ST7735_DrawChar(11+(11*i), 10, dateString[i], dateStringColor[i], 0x0000, 2);
//				}
//				for(i = 0; i < strlen(timeString); i++)
//				{
//					ST7735_DrawChar(22+(11*i), 40, timeString[i], timeStringColor[i], 0x0000, 2);
//				}
//
///*				fputs(dateString, fout);
//				fputs("\n", fout);
//				fputs(timeString, fout);
//				fputs("\n", fout);*/
//			}
//			if(EncoderDecipher(&Encoder1, &Encoder2, &PushButton) == HOLD)
//			{
//				Setup = YES;
//				memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//				memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//				dateStringColor[0] = ST7735_Color565(255, 0, 0);
//				dateStringColor[1] = ST7735_Color565(255, 0, 0);
//				dateStringColor[2] = ST7735_Color565(255, 0, 0);
//				Top = YES;
//			}
////		}
////Allowing user to enter new time
//		while(!movement)
//		{
//	        if (status)
//	        {
//	        	status = 0;
//	        	r_rx_payload(8, &data);
//	        	printf("%d  %d  %d\n", data[0], data[1], data[2]);
//	        	if(data[0] == '0')
//	        		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
//				else if (data[0] == '1')
//					GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
//
//	        	if(data[1] == '0')
//					GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
//				else if (data[1] == '1')
//					GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
//
//	        	if(data[2] == '0')
//					GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
//				else if (data[2] == '1')
//					GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
//
//	        }
//
//
//
//
//
//			if(z)
//			{
//				movement = EncoderDecipher(Encoder1, Encoder2, PushButton);
//				z = 0;
//			}
//			if(flag)
//			{
//				flag = 0;
//				for(i = 0; i < strlen(dateString); i++)
//				{
//					ST7735_DrawChar(11+(11*i), 10, dateString[i], dateStringColor[i], 0x0000, 2);
//				}
//				for(i = 0; i < strlen(timeString); i++)
//				{
//					ST7735_DrawChar(22+(11*i), 40, timeString[i], timeStringColor[i], 0x0000, 2);
//				}
//			}
//		}
//
//		if(movement == RIGHT)
//		{
//			flag = 1;
//			if(Top)		// Up
//			{
//				if(dateStringColor[1] == 31)
//				{
//					if(++date[0] > 11)
//						date[0] = 0;
//				}
//				else if(dateStringColor[4] == 31)
//				{
//					if(++date[1] > 31)
//						date[1] = 0;
//				}
//				else
//				{
//					if(++date[2] > 2050)
//						date[2] = 1950;
//				}
//			}
//			else
//			{
//				if(timeStringColor[1] == 31)
//				{
//					if(++time[0] > 24)
//						time[0] = 0;
//				}
//				else if(timeStringColor[4] == 31)
//				{
//					if(++time[1] > 59)
//						time[1] = 0;
//				}
//				else
//				{
//					if(++time[2] > 59)
//						time[2] = 0;
//				}
//			}
//		}
//
//		if(movement == LEFT)
//		{
//			flag = 1;
//			if(Top)		// Up
//			{
//				if(dateStringColor[1] == 31)
//				{
//					if(--date[0] < 0)
//						date[0] = 11;
//				}
//				else if(dateStringColor[4] == 31)
//				{
//					if(--date[1] < 0)
//						date[1] = 31;
//				}
//				else
//				{
//					if(--date[2] < 1950)
//						date[2] = 2050;
//				}
//			}
//			else
//			{
//				if(timeStringColor[1] == 31)
//				{
//					if(--time[0] < 0)
//						time[0] = 24;
//				}
//				else if(timeStringColor[4] == 31)
//				{
//					if(--time[1] < 0)
//						time[1] = 59;
//				}
//				else
//				{
//					if(--time[2] < 0)
//						time[2] = 59;
//				}
//			}
//		}
//
//		if(movement == PRESS)
//		{
//			flag = 1;
////			if(Top)
////				Top = NO;
////			else
////				Top = YES;
//
//			if(Top)		// XXX0XX0XXXX
//			{
//				if(dateStringColor[1] == 31)
//				{
//					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//					dateStringColor[4] = ST7735_Color565(255, 0, 0);
//					dateStringColor[5] = ST7735_Color565(255, 0, 0);
//				}
//				else if(dateStringColor[4] == 31)
//				{
//					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//					dateStringColor[7] = ST7735_Color565(255, 0, 0);
//					dateStringColor[8] = ST7735_Color565(255, 0, 0);
//					dateStringColor[9] = ST7735_Color565(255, 0, 0);
//					dateStringColor[10] = ST7735_Color565(255, 0, 0);
//				}
//				else if(dateStringColor[8] == 31)
//				{
//					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					timeStringColor[0] = ST7735_Color565(255, 0, 0);
//					timeStringColor[1] = ST7735_Color565(255, 0, 0);
//					Top = NO;
//				}
//			}
//			else				//XX0XX0XX
//			{
//				if(timeStringColor[1] == 31)
//				{
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					timeStringColor[3] = ST7735_Color565(255, 0, 0);
//					timeStringColor[4] = ST7735_Color565(255, 0, 0);
//				}
//				else if(timeStringColor[4] == 31)
//				{
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					timeStringColor[6] = ST7735_Color565(255, 0, 0);
//					timeStringColor[7] = ST7735_Color565(255, 0, 0);
//				}
//				else if(timeStringColor[7] == 31)
//				{
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					dateStringColor[0] = ST7735_Color565(255, 0, 0);
//					dateStringColor[1] = ST7735_Color565(255, 0, 0);
//					dateStringColor[2] = ST7735_Color565(255, 0, 0);
//					Top = YES;
//				}
//			}
//		}
//		sprintf(dateString, "%s %02d,%d", months[date[0]], date[1], date[2]);
//		sprintf(timeString, "%02d:%02d:%02d", time[0], time[1], time[2]);
//
//		if(movement == HOLD)
//		{
//			if(!Setup)
//			{	//Setting new time
//				Setup = 1;
//				flag = 1;
//				if(dateStringColor[0] == 31 || dateStringColor[4] == 31 || dateStringColor[8] == 31 || timeStringColor[0] == 31 || timeStringColor[4] == 31 || timeStringColor[7] == 31)
//				{
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//				}
//				else
//				{
//					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//					dateStringColor[0] = ST7735_Color565(255, 0, 0);
//					dateStringColor[1] = ST7735_Color565(255, 0, 0);
//					dateStringColor[2] = ST7735_Color565(255, 0, 0);
//					Top = YES;
//				}
//			}
//			else //Exiting setup
//			{
//				//Setup new time into system. Exiting setup
//				Setup = 0;
//				tick = 1;
//				memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
//				memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//
//				sprintf(temp, "%d, %d, %d, %d, %d, %d", date[0], date[1], date[2], time[0], time[1], time[2]);
//				sscanf(temp, "%x, %x, %x, %x, %x, %x", &setTime.month, &setTime.dayOfmonth, &setTime.year, &setTime.hours, &setTime.minutes, &setTime.seconds);
//
///*				setTime.month = date[0];
//				setTime.dayOfmonth = date[1];
//				setTime.year = date[2];
//				setTime.hours = time[0];
//				setTime.minutes = time[1];
//				setTime.seconds = time[2];*/
//
//				MAP_RTC_C_initCalendar(&setTime, RTC_C_FORMAT_BCD);
//				MAP_RTC_C_startClock();
//
//			}
//		}
//		movement = NONE;
//	}
}
//-----------------------------------------------------------------------
void systick_isr(void)
{
	Encoder1[1] = Encoder1[0];
	Encoder2[1] = Encoder2[0];

	Encoder1[0] = Debouncer(GPIO_PORT_P4, GPIO_PIN6);
	Encoder2[0] = Debouncer(GPIO_PORT_P6, GPIO_PIN5);
	PushButton[0] = Debouncer(GPIO_PORT_P6, GPIO_PIN4);

	z = 1;
}
//-----------------------------------------------------------------------
/* RTC ISR */
void rtc_isr(void)
{
    uint32_t status;

    tick = 1;
    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)
    {
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        newTime = MAP_RTC_C_getCalendarTime();
    }

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {
        /* Interrupts every minute - Set breakpoint here */
        __no_operation();
//        newTime = MAP_RTC_C_getCalendarTime();
    }

    if (status & RTC_C_CLOCK_ALARM_INTERRUPT)
    {
        /* Interrupts at 10:04pm */
        __no_operation();
    }
}
//-----------------------------------------------------------------------
int DisplayInit (void)
{
	int err = 0;
	strcpy(months[0], "JAN");
	strcpy(months[1], "FEB");
	strcpy(months[2], "MAR");
	strcpy(months[3], "APR");
	strcpy(months[4], "MAY");
	strcpy(months[5], "JUN");
	strcpy(months[6], "JUL");
	strcpy(months[7], "AUG");
	strcpy(months[8], "SEP");
	strcpy(months[9], "OCT");
	strcpy(months[10], "NOV");
	strcpy(months[11], "DEC");

	memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
	memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
//	dateStringColor[0] = ST7735_Color565(255, 0, 0);
//	dateStringColor[1] = ST7735_Color565(255, 0, 0);
//	dateStringColor[2] = ST7735_Color565(255, 0, 0);

	strcpy(dateString, "JAN 00,1999");
	strcpy(timeString, "00:11:22");
	Top = YES;
	date[0] = 0;
	date[0] = 0;
	date[2] = 2000;

	time[0] = 0;
	time[1] = 1;
	time[2] = 2;

	Clock_Init48MHz();                   // set system clock to 48 MHz
	ST7735_InitR(INITR_GREENTAB);

	return err;
}
//-----------------------------------------------------------------------
int RF_Init(void)
{
	int err = 0;

	/* Configuring P6.7 as an input. P1.0 as output and enabling interrupts */
//	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
	//Setting RGB LED as output
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
	GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

	//Set P6.1 to be the pin interupt
	MAP_GPIO_setAsInputPin(GPIO_PORT_P6, GPIO_PIN1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P6, GPIO_PIN1);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P6, GPIO_PIN1);

	MAP_Interrupt_enableInterrupt(INT_PORT6);
	MAP_Interrupt_enableMaster();

    WDTCTL = WDTHOLD | WDTPW;

    /* Initial values for nRF24L01+ library config variables */
    rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
    rf_addr_width      = 1;
    rf_speed_power     = RF24_SPEED_MIN | RF24_POWER_MAX;
    rf_channel     	   = 120;

    msprf24_init();  // All RX pipes closed by default
    msprf24_set_pipe_packetsize(0, 3);
    msprf24_open_pipe(0, 1);  // Open pipe#0 with Enhanced ShockBurst enabled for receiving Auto-ACKs

    // Transmit to 'rad01' (0x72 0x61 0x64 0x30 0x31)
    msprf24_standby();
    user = msprf24_current_state();
    memcpy(addr, "\xDE\xAD\xBE\xEF\x01", 5);
//    addr[0] = 0xDE; addr[1] = 0xAD; addr[2] = 0xBE; addr[3] = 0xEF; addr[4] = 0x00;
    w_tx_addr(addr);
    w_rx_addr(0, addr);  // Pipe 0 receives auto-ack's, autoacks are sent back to the TX addr so the PTX node
                     // needs to listen to the TX addr on pipe#0 to receive them.
    msprf24_activate_rx();

	return err;
}
//-----------------------------------------------------------------------
// GPIO ISR
void gpio_isr(void)
{
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P6);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P6, status);

    msprf24_get_irq_reason();  // this updates rf_irq
	if (rf_irq & RF24_IRQ_TX)
	{
		status = 1;
	}
	if (rf_irq & RF24_IRQ_TXFAILED)
	{
		status = 0;
	}
	msprf24_irq_clear(RF24_IRQ_MASK);  // Clear any/all of them
}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

