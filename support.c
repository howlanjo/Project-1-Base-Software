#include <driverlib.h>
#include "ST7735.h"
#include "ClockSystem.h"
#include "support.h"

//-----------------------------------------------------------------------
void InitFunction(void)
{
	RTC_C_Calendar currentTime;
	currentTime.dayOfmonth = 0x00;
	currentTime.hours = 0x18;
	currentTime.minutes = 0x22;
	currentTime.month = 0x01;
	currentTime.seconds = 0x44;
	currentTime.year = 0x2016;

	//	Halting the Watchdog

	//	Configuring SysTick to trigger at 3000 (MCLK is 3MHz so this will make it toggle every 0.02s)
	MAP_SysTick_enableModule();
	MAP_SysTick_setPeriod(3000);
	MAP_SysTick_enableInterrupt();

	//	Configuring GPIO as an output
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

	//	Configuring GPIO as an input
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN6);
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P6, GPIO_PIN5);
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P6, GPIO_PIN4);

	/* Configuring pins for peripheral/crystal usage and LED for output */
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

	CS_setExternalClockSourceFrequency(32000,48000000);

	/* Starting LFXT in non-bypass mode without a timeout. */
	CS_startLFXT(false);

	/* Initializing RTC with current time as described in time in
	 * definitions section */
	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Setup Calendar Alarm for 10:04pm (for the flux capacitor) */
	MAP_RTC_C_setCalendarAlarm(0x04, 0x22, RTC_C_ALARMCONDITION_OFF,
			RTC_C_ALARMCONDITION_OFF);

	/* Specify an interrupt to assert every minute */
	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	MAP_RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	MAP_RTC_C_enableInterrupt(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);

	/* Start RTC Clock */
	MAP_RTC_C_startClock();

	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	MAP_Interrupt_enableSleepOnIsrExit();

	//	Enabling MASTER interrupts
//	MAP_Interrupt_enableMaster();

//	ST7735_SetRotation(3);
//	ST7735_FillScreen(0x0000);

}
//-----------------------------------------------------------------------
// Make P2.2 an output, enable digital I/O, ensure alt. functions off
void SSR_Init(void)
{
	// initialize P2.2 and make it output
	P2SEL0 &= ~0x04;
	P2SEL1 &= ~0x04;            // configure SSR pin as GPIO
	P2DS |= 0x04;               // make SSR pin high drive strength
	P2DIR |= 0x04;              // make SSR pin out
}
//-----------------------------------------------------------------------
int binaryToDecimal(int bin1, int bin2)
{
	if(bin1 == 0 && bin2 == 0)
		return 0;
	if(bin1 == 0 && bin2 == 1)
		return 1;
	if(bin1 == 1 && bin2 == 0)
		return 2;
	if(bin1 == 1 && bin2 == 1)
		return 3;
}
//-----------------------------------------------------------------------
uint8_t Debouncer(uint_fast8_t portPtr, uint_fast8_t pinPtr)
{
	uint8_t temp[2];
	int i = 0;

	//Remain in for loop until 6 consecutive reads of the same value are read
	for(i = 0; i < 6; i++)
	{
		 temp[0] = MAP_GPIO_getInputPinValue(portPtr, pinPtr);

		 //If encoder value differs from last value read, start over
		 if(temp[0] != temp[1])
		 {
			 i = 0;
			 temp[1] = temp[0];
		 }
	}

	return temp[0];
}
//-----------------------------------------------------------------------
int EncoderDecipher(uint8_t encoder1[], uint8_t encoder2[], uint8_t pushbutton[])
{
	int sum[3];
	static int y = 0, movement = NONE, test = 1;

/*	//Combining signal A and B to a single integer
	sum[0] = binaryToDecimal(encoder1[0], encoder2[0]);
	sum[1] = binaryToDecimal(encoder1[1], encoder2[1]);

	//Creating 4 bit sum value
	sum[2] = ((sum[0] & 0x03) << 2);
	sum[2] = (sum[2] + sum[1]);*/

	if((!encoder1[0] && !encoder2[0] && encoder1[1] && encoder2[1])
	 ||(!encoder1[0] && encoder2[0] && encoder1[1] && encoder2[1])
	 ||(encoder1[0] && encoder2[0] && encoder1[1] && !encoder2[1])
	 ||(encoder1[0] && !encoder2[0] && !encoder1[1] && !encoder2[1]))
	{
		//Counter-Clockwise
//		encoder1[1] = encoder1[0];
//		encoder2[1] = encoder2[0];
		movement = LEFT;
	}
	else if((!encoder1[0] && !encoder2[0] && encoder1[1] && !encoder2[1])
			 ||(encoder1[0] && !encoder2[0] && encoder1[1] && encoder2[1])
			 ||(encoder1[0] && encoder2[0] && !encoder1[1] && encoder2[1])
			 ||(!encoder1[0] && encoder2[0] && !encoder1[1] && !encoder2[1]))
	{
		//Clockwise
//		encoder1[1] = encoder1[0];
//		encoder2[1] = encoder2[0];
		movement = RIGHT;
	}
	else
	{
		movement = NONE;
	}

	if(pushbutton[0] != pushbutton[1])
	{
		test = 1;
		pushbutton[1] = pushbutton[0];
		if(pushbutton[0] == 1 & movement != HOLD)
			movement = PRESS;
		else
			movement = NONE;
	}
	else
	{
		if(pushbutton[0] == 0)
			y++;
		else
			y = 0;
		if(y == 10000 && test)
		{
			movement = HOLD;
			test = 0;
			y = 0;
		}
	}

	return movement;
}
/*
int StringOut( char filePath, char *string)
{
	int ret = 0;	//Set ret to 0 meaning no errors
	FILE *ptr;

	err = f_open(ptr, filePath, "a");	//open in 'append' mode. This way the file isn't deleted, just simply added onto
	if(!err)
	{
		err |= f_write(ptr, string, sizeof(string), &outSize);		//write string
		err |= f_close(ptr);		//close file

		if(outSize != sizeof(string))
		{
			ret = -1;	//error occured. This value will be returned out of function
		}
	}
	else
	{
		ret = -2; 	//error occured. This value will be returned out of function
	}
	return ret;
}
*/


