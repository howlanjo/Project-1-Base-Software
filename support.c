#include <driverlib.h>
#include "ST7735.h"
#include "ClockSystem.h"
#include "support.h"

uint16_t Light[6];
//-----------------------------------------------------------------------
void InitFunction(void)
{
	RTC_C_Calendar currentTime;
	currentTime.dayOfmonth = 0x20;
	currentTime.hours = 0x18;
	currentTime.minutes = 0x00;
	currentTime.month = 0x00;
	currentTime.seconds = 0x00;
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
//	MAP_RTC_C_setCalendarAlarm(0x04, 0x22, RTC_C_ALARMCONDITION_OFF, RTC_C_ALARMCONDITION_OFF);

//	newTime = MAP_RTC_C_getCalendarTime();

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

	if((!encoder1[0] && !encoder2[0] && encoder1[1] && encoder2[1])
	 ||(!encoder1[0] && encoder2[0] && encoder1[1] && encoder2[1])
	 ||(encoder1[0] && encoder2[0] && encoder1[1] && !encoder2[1])
	 ||(encoder1[0] && !encoder2[0] && !encoder1[1] && !encoder2[1]))
	{
		movement = LEFT;
	}
	else if((!encoder1[0] && !encoder2[0] && encoder1[1] && !encoder2[1])
			 ||(encoder1[0] && !encoder2[0] && encoder1[1] && encoder2[1])
			 ||(encoder1[0] && encoder2[0] && !encoder1[1] && encoder2[1])
			 ||(!encoder1[0] && encoder2[0] && !encoder1[1] && !encoder2[1]))
	{
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
		if(y == 15000 && test)
		{
			movement = HOLD;
			test = 0;
			y = 0;
		}
	}

	return movement;
}
//-----------------------------------------------------------------------
void calculateLighting(uint16_t value, char stringOUT[30], int8_t *index)
{
	memset(stringOUT, 0, sizeof(stringOUT));

	Light[0] = 8000.0;
	Light[1] = 7000.0;
	Light[2] = 6400.0;
	Light[3] = 5800.0;
	Light[4] = 5200.0;
	Light[5] = 4000.0;

	if(value > Light[0])
	{
		sprintf(stringOUT, "Dark");
		*index = 0;
	}
	else if (value < Light[0] && value > Light[1])
	{
		sprintf(stringOUT, "Moon");
		*index = 1;
	}
	else if (value < Light[1] && value > Light[2])
	{
		sprintf(stringOUT, "Dusk");
		*index = 2;
	}
	else if (value < Light[2] && value > Light[3])
	{
		sprintf(stringOUT, "Cloud");
		*index = 3;
	}
	else if (value < Light[3] && value > Light[4])
	{
		sprintf(stringOUT, "Sun");
		*index = 4;
	}
	else if (value < Light[4] && value > Light[5])
	{
		sprintf(stringOUT, "Bright");
		*index = 5;
	}
	else
	{
		sprintf(stringOUT, "ERROR");
		*index = 6;
	}
}

