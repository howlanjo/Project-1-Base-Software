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

#define PACKET_SIZE				22


void SSR_Init(void);
int RF_Init(void);
void Delay1ms(uint32_t n);
void DelayWait10ms(uint32_t n){
  Delay1ms(n*10);
}

int z = 0, Top = 0, date[3], time[3], tick = 0, Setup = 0;
uint8_t Encoder1[2], Encoder2[2], PushButton[2], movement = 0, i, flag;
int16_t dateStringColor[20], timeStringColor[20];
int16_t lightingStringColor[20];
static int16_t temperatureOUT[2], temperatureIN[2], lux[2], humidityOUT[2], humidityIN[2], lux[2];
static uint16_t pressure[2];
char dateString[20], timeString[20], lightIndexString[2][20];
char months[12][5], temp[50], tempString[50], tempCharacter;
static volatile RTC_C_Calendar newTime, setTime;
int err = NONE, lightIndex[2], tempFormat;
char addr[5];
char buf[32];
uint8_t data[32];
volatile unsigned int user;
int status, screen;

const Timer_A_UpModeConfig upConfig =
{
		TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source = 12 MHz
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/1 = 16 MHz
        TIMER_PERIOD,                           // 12000 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

//-----------------------------------------------------------------------
//
//		THIS CODE HAS THE MANUAL CHIP SELECT FOR THE DISPLAY
//
int main (void)
{
	int encoderValue = 0, count = 0;
	int16_t temp0, temp1, temp2, temp3;
	int refreshValues = YES, i, t = 0, h = 0;
	screen = 2;
	tempFormat = CEL;
	tempCharacter = 'C';

	MAP_WDT_A_holdTimer();

	//Configuring pins for peripheral/crystal usage and LED for output
	CS_setExternalClockSourceFrequency(32768,48000000);
	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
	CS_startHFXT(false);
	MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);


//	/* Starting HFXT in non-bypass mode without a timeout. Before we start
//	 * we have to change VCORE to 1 to support the 48MHz frequency */
//	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
//	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
//	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
//	CS_setExternalClockSourceFrequency(32000, 48000000);
//	MAP_CS_setDCOFrequency(48000000);

	InitFunction();
	err = DisplayInit();
	err |= InitOneWire();
	err |= RF_Init();

	temperatureOUT[0] = 250;
	temperatureIN[0] = 220;
	humidityOUT[0] = 440;
	humidityIN[0] = 330;
	pressure[0] = 60;
	lux[0] = 9000;
	lightIndex[0] = 1;

	temperatureOUT[1] = 251;
	temperatureIN[1] = 221;
	humidityOUT[1] = 441;
	humidityIN[1] = 331;
	pressure[1] = 60;
	lux[1] = 9000;
	lightIndex[1] = 1;

	MAP_Interrupt_enableMaster();
	MAP_Interrupt_setPriority(INT_TA0_0, 0x00);

	while(1)
	{
		while(!Setup) //Displaying current time
		{
	        if (status)
	        {
	        	status = 0;
	        	r_rx_payload((uint8_t)PACKET_SIZE, &data);
//	        	printf("%s\n", data);
	        	count = sscanf(data, "<T%dP%dH%dL%d>", &temp0, &temp1, &temp2, &temp3);
	        	temperatureOUT[0] = temp0;
	        	pressure[0] = temp1;
	        	humidityOUT[0] = temp2;
	        	lux[0] = temp3;
	        	calculateLighting(lux[0], lightIndexString[0], &lightIndex[0]);
	        }

			if(tick)
			{
			    if(i++ > 5)
			    {
			    	i = 0;
			    	MAP_SysTick_disableModule();
					__delay_cycles(100);
					dht_start_read();
					t = dht_get_temp();
					h = dht_get_rh();
					MAP_SysTick_enableModule();
					__delay_cycles(100);
			    }

				humidityIN[0] = h;
				temperatureIN[0] = t;

				if(tempFormat == FAR)
				{
					temperatureIN[0] = (temperatureIN[0] * 1.8) + 32;
					temperatureOUT[0] = (temperatureOUT[0] * 1.8) + 32;
				}

				tick = 0;
				if(screen == 1)
				{
//					ST7735_FillScreen(0);
					sprintf(dateString, "%s %02X, %X", months[newTime.month], newTime.dayOfmonth, newTime.year);
					sprintf(timeString, "%02X:%02X:%02X", newTime.hours, newTime.minutes, newTime.seconds);

					//Printing the time
					for(i = 0; i < strlen(timeString); i++)
					{
						ST7735_DrawChar(22+(11*i), 10, timeString[i], timeStringColor[i], 0x0000, 2);
					}

					//Printing the date
					for(i = 0; i < strlen(dateString); i++)
					{
						ST7735_DrawChar(30+(6*i), 30, dateString[i], dateStringColor[i], 0x0000, 1);
					}

					if((temperatureOUT[0] != temperatureOUT[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", temperatureOUT[1]/10, temperatureOUT[1]%10);
						ST7735_DrawStringHorizontal(20, 80, tempString, ST7735_Color565(0, 0, 0), 4);
						ST7735_DrawCharS((20 + strlen(tempString)*4*6), 80, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", temperatureOUT[0]/10, temperatureOUT[0]%10);
						ST7735_DrawStringHorizontal(20, 80, tempString, ST7735_Color565(255, 0, 0), 4);
						ST7735_DrawCharS((20 + strlen(tempString)*4*6), 80, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						temperatureOUT[1] = temperatureOUT[0];
					}

					if((temperatureIN[0] != temperatureIN[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", temperatureIN[1]/10, temperatureIN[1]%10);
						ST7735_DrawStringHorizontal(70, 130, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((70 + strlen(tempString)*2*6), 130, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", temperatureIN[0]/10, temperatureIN[0]%10);
						ST7735_DrawStringHorizontal(70, 130, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((70 + strlen(tempString)*2*6), 130, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						temperatureIN[1] = temperatureIN[0];
					}

					if((lightIndex[0] != lightIndex[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%s", lightIndexString[1]);
						ST7735_DrawStringVertical(0, 60, tempString, ST7735_Color565(0, 0, 0), 2);

						sprintf(tempString, "%s", lightIndexString[0]);
						ST7735_DrawStringVertical(0, 60, tempString, ST7735_Color565(0, 255, 0), 2);
						memcpy(lightIndexString[1], lightIndexString[0], sizeof(lightIndexString[0]));
						lightIndex[1] = lightIndex[0];
					}

//					sprintf(tempString, "Sunny");
//					ST7735_DrawStringVertical(0, 60, tempString, ST7735_Color565(255, 0, 0), 2);
					refreshValues = NO;
				}
				if(screen == 2)
				{
					sprintf(timeString, "%02X:%02X:%02X", newTime.hours, newTime.minutes, newTime.seconds);


					for(i = 0; i < strlen(timeString); i++)
					{
						ST7735_DrawChar(35+(6*i), 5, timeString[i], ST7735_Color565(0, 0, 0), ST7735_Color565(255, 255, 255), 1);
					}

					if(refreshValues == YES)
					{
						sprintf(tempString, "Inside");
						ST7735_DrawStringVertical(0, 10, tempString, ST7735_Color565(0, 255, 0), 1);

						sprintf(tempString, "Outside");
						ST7735_DrawStringVertical(120, 90, tempString, ST7735_Color565(0, 255, 0), 1);

						sprintf(tempString, "_______________________");
						ST7735_DrawStringHorizontal(0, 55, tempString, ST7735_Color565(255, 255, 255), 1);
					}

					if((temperatureIN[0] != temperatureIN[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", temperatureIN[1]/10, temperatureIN[1]%10);
						ST7735_DrawStringHorizontal(12, 30, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((12 + strlen(tempString)*2*6), 25, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", temperatureIN[0]/10, temperatureIN[0]%10);
						ST7735_DrawStringHorizontal(12, 30, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((12 + strlen(tempString)*2*6), 25, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						temperatureIN[1] = temperatureIN[0];
					}

					if((humidityIN[0] != humidityIN[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", humidityIN[1]/10, humidityIN[1]%10);
						ST7735_DrawStringHorizontal(74, 30, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((74 + strlen(tempString)*2*6), 27, '%', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", humidityIN[0]/10, humidityIN[0]%10);
						ST7735_DrawStringHorizontal(74, 30, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((74 + strlen(tempString)*2*6), 27, '%', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						humidityIN[1] = humidityIN[0];
					}

					//==========================================================================================================================


					if((temperatureOUT[0] != temperatureOUT[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", temperatureOUT[1]/10, temperatureOUT[1]%10);
						ST7735_DrawStringHorizontal(0, 70, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 68, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", temperatureOUT[0]/10, temperatureOUT[0]%10);
						ST7735_DrawStringHorizontal(0, 70, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 68, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						temperatureOUT[1] = temperatureOUT[0];
					}

					if((humidityOUT[0] != humidityOUT[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", humidityOUT[1]/10, humidityOUT[1]%10);
						ST7735_DrawStringHorizontal(0, 90, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 87, '%', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", humidityOUT[0]/10, humidityOUT[0]%10);
						ST7735_DrawStringHorizontal(0, 90, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 87, '%', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						humidityOUT[1] = humidityOUT[0];
					}

					if((lux[0] != lux[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%d", lux[1]);
						ST7735_DrawStringHorizontal(0, 112, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 112, 'L', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%d", lux[0]);
						ST7735_DrawStringHorizontal(0, 112, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawCharS((0 + strlen(tempString)*2*6), 112, 'L', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
						lux[1] = lux[0];
					}

					if((pressure[0] != pressure[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%02d.%.1d", pressure[1]/10, pressure[1]%10);
						ST7735_DrawStringHorizontal(0, 135, tempString, ST7735_Color565(0, 0, 0), 2);
						ST7735_DrawStringHorizontal((0 + strlen(tempString)*2*6), 135, "mBar", ST7735_Color565(0, 0, 0), 1);

						sprintf(tempString, "%02d.%.1d", pressure[0]/10, pressure[0]%10);
						ST7735_DrawStringHorizontal(0, 135, tempString, ST7735_Color565(255, 0, 0), 2);
						ST7735_DrawStringHorizontal((0 + strlen(tempString)*2*6), 135, "mBar", ST7735_Color565(255, 0, 0), 1);
						pressure[1] = pressure[0];
					}

					if((lightIndex[0] != lightIndex[1]) || refreshValues == YES)
					{
						sprintf(tempString, "%s", lightIndexString[1]);
						ST7735_DrawStringVertical(105, 70, tempString, ST7735_Color565(0, 0, 0), 2);

						sprintf(tempString, "%s", lightIndexString[0]);
						ST7735_DrawStringVertical(105, 70, tempString, ST7735_Color565(0, 0, 255), 2);
						memcpy(lightIndexString[1], lightIndexString[0], sizeof(lightIndexString[0]));
						lightIndex[1] = lightIndex[0];
					}

					refreshValues = NO;
				}
			}

			encoderValue = EncoderDecipher(&Encoder1, &Encoder2, &PushButton);

			if(encoderValue == HOLD)
			{
				Setup = YES;
				memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
				memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
				dateStringColor[0] = ST7735_Color565(255, 0, 0);
				dateStringColor[1] = ST7735_Color565(255, 0, 0);
				dateStringColor[2] = ST7735_Color565(255, 0, 0);


				sprintf(temp, "%x, %x, %x, %x, %x, %x", newTime.month, newTime.dayOfmonth, newTime.year, newTime.hours, newTime.minutes, newTime.seconds);
				sscanf(temp, "%d, %d, %d, %d, %d, %d", &date[0], &date[1], &date[2], &time[0], &time[1], &time[2]);

//				date[0] = newTime.month;
//				date[1] = newTime.dayOfmonth;
//				date[2] = newTime.year;
//				time[0] = newTime.hours;
//				time[1] = newTime.minutes;
//				time[2] = newTime.seconds;

				ST7735_FillScreen(0);

				Top = YES;
			}
			else if (encoderValue == RIGHT || encoderValue == LEFT)
			{
				if(screen == 1)
				{
					screen = 2;
					ST7735_FillScreen(0);
					refreshValues = YES;
				}
				else if(screen == 2)
				{
					screen = 1;
					ST7735_FillScreen(0);
					refreshValues = YES;
				}
			}
			else if(encoderValue == PRESS)
			{
				if(tempFormat == FAR)
				{
					tempFormat = FAR;
					tempCharacter = 'F';
				}
				else
				{
					tempFormat = CEL;
					tempCharacter = 'C';
				}
			}
		}
//Allowing user to enter new time
		while(!movement)
		{
			if(z)
			{
				movement = EncoderDecipher(Encoder1, Encoder2, PushButton);
				z = 0;
			}
			if(flag)
			{
				flag = 0;
				for(i = 0; i < strlen(dateString); i++)
				{
					ST7735_DrawChar((11*i), 10, dateString[i], dateStringColor[i], 0x0000, 2);
				}
				for(i = 0; i < strlen(timeString); i++)
				{
					ST7735_DrawChar(22+(11*i), 40, timeString[i], timeStringColor[i], 0x0000, 2);
				}
			}
		}

		if(movement == RIGHT)
		{
			flag = 1;
			if(Top)		// Up
			{
				if(dateStringColor[1] == 31)
				{
					if(++date[0] > 11)
						date[0] = 0;
				}
				else if(dateStringColor[4] == 31)
				{
					if(++date[1] > 31)
						date[1] = 0;
				}
				else
				{
					if(++date[2] > 2050)
						date[2] = 1950;
				}
			}
			else
			{
				if(timeStringColor[1] == 31)
				{
					if(++time[0] > 24)
						time[0] = 0;
				}
				else if(timeStringColor[4] == 31)
				{
					if(++time[1] > 59)
						time[1] = 0;
				}
				else
				{
					if(++time[2] > 59)
						time[2] = 0;
				}
			}
		}

		if(movement == LEFT)
		{
			flag = 1;
			if(Top)		// Up
			{
				if(dateStringColor[1] == 31)
				{
					if(--date[0] < 0)
						date[0] = 11;
				}
				else if(dateStringColor[4] == 31)
				{
					if(--date[1] < 0)
						date[1] = 31;
				}
				else
				{
					if(--date[2] < 1950)
						date[2] = 2050;
				}
			}
			else
			{
				if(timeStringColor[1] == 31)
				{
					if(--time[0] < 0)
						time[0] = 24;
				}
				else if(timeStringColor[4] == 31)
				{
					if(--time[1] < 0)
						time[1] = 59;
				}
				else
				{
					if(--time[2] < 0)
						time[2] = 59;
				}
			}
		}

		if(movement == PRESS)
		{
			flag = 1;
//			if(Top)
//				Top = NO;
//			else
//				Top = YES;

			if(Top)		// XXX0XX0XXXX
			{
				if(dateStringColor[1] == 31)
				{
					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
					dateStringColor[4] = ST7735_Color565(255, 0, 0);
					dateStringColor[5] = ST7735_Color565(255, 0, 0);
				}
				else if(dateStringColor[4] == 31)
				{
					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
					dateStringColor[7] = ST7735_Color565(255, 0, 0);
					dateStringColor[8] = ST7735_Color565(255, 0, 0);
					dateStringColor[9] = ST7735_Color565(255, 0, 0);
					dateStringColor[10] = ST7735_Color565(255, 0, 0);
				}
				else if(dateStringColor[8] == 31)
				{
					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					timeStringColor[0] = ST7735_Color565(255, 0, 0);
					timeStringColor[1] = ST7735_Color565(255, 0, 0);
					Top = NO;
				}
			}
			else				//XX0XX0XX
			{
				if(timeStringColor[1] == 31)
				{
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					timeStringColor[3] = ST7735_Color565(255, 0, 0);
					timeStringColor[4] = ST7735_Color565(255, 0, 0);
				}
				else if(timeStringColor[4] == 31)
				{
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					timeStringColor[6] = ST7735_Color565(255, 0, 0);
					timeStringColor[7] = ST7735_Color565(255, 0, 0);
				}
				else if(timeStringColor[7] == 31)
				{
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					dateStringColor[0] = ST7735_Color565(255, 0, 0);
					dateStringColor[1] = ST7735_Color565(255, 0, 0);
					dateStringColor[2] = ST7735_Color565(255, 0, 0);
					Top = YES;
				}
			}
		}
		sprintf(dateString, "%s %02d,%d", months[date[0]], date[1], date[2]);
		sprintf(timeString, "%02d:%02d:%02d", time[0], time[1], time[2]);

		if(movement == HOLD)
		{
			if(!Setup)
			{	//Setting new time
				Setup = 1;
				flag = 1;
				if(dateStringColor[0] == 31 || dateStringColor[4] == 31 || dateStringColor[8] == 31 || timeStringColor[0] == 31 || timeStringColor[4] == 31 || timeStringColor[7] == 31)
				{
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
				}
				else
				{
					memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
					memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
					dateStringColor[0] = ST7735_Color565(255, 0, 0);
					dateStringColor[1] = ST7735_Color565(255, 0, 0);
					dateStringColor[2] = ST7735_Color565(255, 0, 0);
					Top = YES;
				}
			}
			else //Exiting setup
			{
				//Setup new time into system. Exiting setup
				Setup = 0;
				tick = 1;
				memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
				memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));

				sprintf(temp, "%d, %d, %d, %d, %d, %d", date[0], date[1], date[2], time[0], time[1], time[2]);
				sscanf(temp, "%x, %x, %x, %x, %x, %x", &setTime.month, &setTime.dayOfmonth, &setTime.year, &setTime.hours, &setTime.minutes, &setTime.seconds);

				MAP_RTC_C_initCalendar(&setTime, RTC_C_FORMAT_BCD);
				MAP_RTC_C_startClock();

				ST7735_FillScreen(0);
			}
		}
		movement = NONE;
	}
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

//	strcpy(dateString, "JAN 00,1999");
//	strcpy(timeString, "00:11:22");
//	Top = YES;
//	date[0] = 0;
//	date[0] = 0;
//	date[2] = 2000;
//
//	time[0] = 0;
//	time[1] = 1;
//	time[2] = 2;

	Clock_Init48MHz();                   // set system clock to 48 MHz
	ST7735_InitR(INITR_GREENTAB);

	return err;
}
//------------------------------------------------------------------------------
int InitOneWire(void)
{
	int err = 0;
	uint32_t HSMfreq, MCLKfreq, SMCLKfreq, ACLKfreq;

	HSMfreq = MAP_CS_getMCLK();  // get HSMCLK value to see if crystal defaulted (it did, so use DCO frequency)
//	printf("HSMfreq=%d\r\n",HSMfreq);

	 // set SMCLK to 12 MHz
	SMCLKfreq = MAP_CS_getSMCLK();  // get SMCLK value to verify it was set correctly
//	printf("SMCLKfreq=%d\r\n",SMCLKfreq);

	MAP_WDT_A_holdTimer();

	/* Configuring Timer_A0 for Up Mode */
	MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);

	// enable timer interrupts (more configuration done in dht22.c)
	MAP_Timer_A_enableInterrupt(TIMER_A0_BASE);

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
    rf_addr_width      = (uint8_t)PACKET_SIZE;
    rf_speed_power     = RF24_SPEED_MIN | RF24_POWER_MAX;
    rf_channel     	   = 120;

    msprf24_init();  // All RX pipes closed by default
    msprf24_set_pipe_packetsize(0, (uint8_t)PACKET_SIZE);
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
//------------------------------------------------------------------------------
void timer0_a0_isr(void)
{
  TA0CCTL0 &= ~CCIFG;
  // This handles only TA0CCR0 interrupts
  switch (dht_current_state)
  {
  case DHT_IDLE:
    break; // Shouldn't be here
  case DHT_TRIGGERING:
    // 1ms has passed since setting the pin low
    // Let P7.0 go high and set Compare input on T1
    P7DIR &= ~BIT3; // input
    P7SEL0 |= BIT3; // Timer0_A0.CCI0A input
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2 | ID_0 | MC_2;
    TA0CCTL0 = CM_2 | CCIS_0 | CAP | CCIE; // Capture on falling edge
    dht_current_state = DHT_WAITING_ACK;
    break;
  case DHT_WAITING_ACK:
    // I don't care about timings here...
    P7DIR &= ~BIT3; // input
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2 | ID_0 | MC_2;
    TA0CCTL0 = CM_1 | CCIS_0 | CAP | CCIE; // Capture on rising edge
    dht_current_state = DHT_ACK_LOW;
    break;
  case DHT_ACK_LOW:
    // I don't care about timings here either...
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2 | ID_0 | MC_2;
    TA0CCTL0 = CM_2 | CCIS_0 | CAP | CCIE; // Capture on falling edge
    dht_current_state = DHT_ACK_HIGH;
    dht_data_byte = dht_data_bit = 0;
    break;
  case DHT_ACK_HIGH:
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2 | ID_0 | MC_2;
    TA0CCTL0 = CM_1 | CCIS_0 | CAP | CCIE; // Capture on rising edge
    dht_current_state = DHT_IN_BIT_LOW;
    break;
  case DHT_IN_BIT_LOW:
    TA0CTL = TACLR;
    TA0CTL = TASSEL_2 | ID_0 | MC_2;
    TA0CCTL0 = CM_2 | CCIS_0 | CAP | CCIE; // Capture on falling edge
    dht_current_state = DHT_IN_BIT_HIGH;
    break;
  case DHT_IN_BIT_HIGH:
    // OK now I need to measure the time since last time
    dht_data.bytes[dht_data_byte] <<= 1;
//    if (TA0CCR0 > 750) {  // > 47 us with 16 MHz SMCLK
    if (TA0CCR0 > 564) {  // > 47 us with 12 MHz SMCLK
      // Long pulse: 1
      dht_data.bytes[dht_data_byte] |= 1;
    }
    if (++dht_data_bit >= 8)
    {
      dht_data_bit = 0;
      dht_data_byte++;
    }
    if (dht_data_byte >= 5)
    {
      // I'm done, bye
      // TODO: check CRC
      TA0CTL = TACLR;
      dht_current_state = DHT_IDLE;
    }
    else
    {
      TA0CTL = TACLR;
      TA0CTL = TASSEL_2 | ID_0 | MC_2;
      TA0CCTL0 = CM_1 | CCIS_0 | CAP | CCIE; // Capture on rising edge
      dht_current_state = DHT_IN_BIT_LOW;
    }
    break;
  }
}
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

