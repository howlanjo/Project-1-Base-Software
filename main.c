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

//==================== Defines ================================
#define PACKET_SIZE				22	//The packet size for the receiving data from RF chip

//=============== Functions ====================================
void SSR_Init(void);
int RF_Init(void);
int InitOneWire(void);
int DisplayInit(void);
void Delay1ms(uint32_t n);
void DelayWait10ms(uint32_t n)
{
  Delay1ms(n*10);
}

//==================== Global Variables =============================
int encoderRefresh = 0, Top = 0, date[3], time[3], tick = 0, Setup = 0;
uint8_t Encoder1[2], Encoder2[2], PushButton[2], movement = 0, i, flag;
int16_t dateStringColor[20], timeStringColor[20];
int16_t lightingStringColor[20];
static int16_t temperatureOUT[2], temperatureIN[2], lux[2], humidityOUT[2], humidityIN[2], lux[2];
static unsigned int pressure[2];
char dateString[20], timeString[20], lightIndexString[2][20];
char months[12][5], temp[50], tempString[50], tempCharacter[2];
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
	//============== Local Variables =====================
	int encoderValue = 0;
	int16_t temp0, temp2, temp3;
	unsigned int temp1;
	int refreshValues = YES, i, t = 0, h = 0;
	double temptemp1, temptemp2;
	screen = 2;
	tempFormat = CEL;
	tempCharacter[0] = 'C';

	// Turning off watch dog timer
	MAP_WDT_A_holdTimer();

	//Configuring pins for peripheral/crystal usage.
	CS_setExternalClockSourceFrequency(32768,48000000);
	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
	CS_startHFXT(false);
	//Setting other clocks to speeds needed throughout the project
	MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);

	//Initialization functions for variable peripherals
	InitFunction();
	err = DisplayInit();
	err |= InitOneWire();
	err |= RF_Init();
	if(err)
	{
		printf("ERROR occured in initialization functions.\n");
	}

	//Setting variables to initial values
	temperatureOUT[0] = 250;	temperatureOUT[1] = 251;
	temperatureIN[0] = 220;		temperatureIN[1] = 221;
	humidityOUT[0] = 440;		humidityOUT[1] = 441;
	humidityIN[0] = 330;		humidityIN[1] = 331;
	pressure[0] = 60;			pressure[1] = 61;
	lux[0] = 9000;				lux[1] = 9001;
	lightIndex[0] = 1;			lightIndex[1] = 2;

	//Enable interupts and set the interupt for the One-Wire to have highest priority
	MAP_Interrupt_enableMaster();
	MAP_Interrupt_setPriority(INT_TA0_0, 0x00);

	//Keep process in infinite loop
	while(1)
	{
		while(!Setup) //'Setup' controls whether or not the user is view the data from
					  //the remote system, or is editing the time for the RTC
		{
	        if (status) //'status' is set high when the IQR pin is set high -- meaning there is a packet
	        			//available to be read in from the RF chip
	        {
	        	status = 0;
	        	r_rx_payload((uint8_t)PACKET_SIZE, &data);	//retrieve data
	        	sscanf(data, "<T%dP%dH%dL%d>", &temp0, &temp1, &temp2, &temp3);  //parse data
	        	temperatureOUT[0] = temp0;
	        	pressure[0] = temp1;
	        	humidityOUT[0] = temp2;
	        	lux[0] = temp3;
	        	calculateLighting(lux[0], lightIndexString[0], &lightIndex[0]); 	//Figure out the lighting condition

	        	//Convert to F if 'tempFormat' is set high when tempFormat is set high, indicating that there needs to be a
	        	//C to F convserion. The data is C from the sensors.
				if(tempFormat == FAR)
				{
					//C to F convserion
					temptemp2 =  ((float)temperatureOUT[0]);
					temperatureOUT[0] = (int)((temptemp2 * 1.8));
					temperatureOUT[0] += 320;
				}
	        }

	        //Tick is set high every second from the RTC interupt
			if(tick)
			{
				i = 0;
				MAP_SysTick_disableModule(); // disable the systick interupt during while getting the
											 // temperature and humidity data from sensor
				//Getting the temp and humidity data
				__delay_cycles(100);
				dht_start_read();
				t = dht_get_temp();
				h = dht_get_rh();
				MAP_SysTick_enableModule(); //Enable the systick interupt again
				__delay_cycles(100);

				//put the humidity and temperature values in the needed variables
				humidityIN[0] = h;
				temperatureIN[0] = t;

				//Convert to F if 'tempFormat' is set high when tempFormat is set high, indicating that there needs to be a
				//C to F convserion. The data is C from the sensors.
				if(tempFormat == FAR)
				{
					//C to F conversion
					temptemp1 =  ((float)temperatureIN[0]);
					temperatureIN[0] = (int)((temptemp1 * 1.8));
					temperatureIN[0] += 320;
				}
			}

			//'screen' is either set to the value of 1 or 2.
			// screen == 1 is the main screen with only the temperatures and lighting condition displayed
			if(screen == 1)
			{
				//If the time has been updated or the screen has been switched, update the string
				if(tick || refreshValues)
				{
					tick = 0;
					//Create the time and date strings
					sprintf(dateString, "%s %02X, %X", months[newTime.month], newTime.dayOfmonth, newTime.year);
					sprintf(timeString, "%02X:%02X:%02X", newTime.hours, newTime.minutes, newTime.seconds);

					//Write "IN:" in the lower part of the screen to indicate to the user which temperature is the
					//inside tempterature
					if(refreshValues == YES)
					{
						ST7735_DrawStringHorizontal(50, 140, "IN:", ST7735_Color565(255, 255, 255), 1);
					}

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
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((temperatureOUT[0] != temperatureOUT[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", temperatureOUT[1]/10, temperatureOUT[1]%10);
					ST7735_DrawStringHorizontal(20, 80, tempString, ST7735_Color565(0, 0, 0), 4);
					ST7735_DrawCharS((20 + strlen(tempString)*4*6), 80, tempCharacter[1], ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", temperatureOUT[0]/10, temperatureOUT[0]%10);
					ST7735_DrawStringHorizontal(20, 80, tempString, ST7735_Color565(255, 0, 0), 4);
					ST7735_DrawCharS((20 + strlen(tempString)*4*6), 80, tempCharacter[0], ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					temperatureOUT[1] = temperatureOUT[0];
					tempCharacter[1] = tempCharacter[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((temperatureIN[0] != temperatureIN[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", temperatureIN[1]/10, temperatureIN[1]%10);
					ST7735_DrawStringHorizontal(70, 130, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((70 + strlen(tempString)*2*6), 130, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", temperatureIN[0]/10, temperatureIN[0]%10);
					ST7735_DrawStringHorizontal(70, 130, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((70 + strlen(tempString)*2*6), 130, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					temperatureIN[1] = temperatureIN[0];
					tempCharacter[1] = tempCharacter[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((lightIndex[0] != lightIndex[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%s", lightIndexString[1]);
					ST7735_DrawStringVertical(0, 60, tempString, ST7735_Color565(0, 0, 0), 2);

					//Write the new string
					sprintf(tempString, "%s", lightIndexString[0]);
					ST7735_DrawStringVertical(0, 60, tempString, ST7735_Color565(0, 255, 0), 2);
					memcpy(lightIndexString[1], lightIndexString[0], sizeof(lightIndexString[0]));
					lightIndex[1] = lightIndex[0];
				}
				//Reset the refreshValues variable. This variable will be set high when the user swithces screen by turning the knob
				refreshValues = NO;
			}
			//'screen' equalling 2 is the screen that displays all the data to the user
			if(screen == 2)
			{
				//If the time has been updated or the screen has been switched, update the string
				if(time || refreshValues)
				{
					tick = 0;
					//Create the time string that will be written
					sprintf(timeString, "%02X:%02X:%02X", newTime.hours, newTime.minutes, newTime.seconds);

					//Write the new time string
					for(i = 0; i < strlen(timeString); i++)
					{
						ST7735_DrawChar(35+(6*i), 5, timeString[i], ST7735_Color565(0, 0, 0), ST7735_Color565(255, 255, 255), 1);
					}
				}

				//When update the items on the screen that will remain constant when the screen is changed.
				if(refreshValues == YES)
				{
					sprintf(tempString, "Inside");
					ST7735_DrawStringVertical(0, 10, tempString, ST7735_Color565(0, 255, 0), 1);

					sprintf(tempString, "Outside");
					ST7735_DrawStringVertical(120, 90, tempString, ST7735_Color565(0, 255, 0), 1);

					sprintf(tempString, "_______________________");
					ST7735_DrawStringHorizontal(0, 55, tempString, ST7735_Color565(255, 255, 255), 1);
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((temperatureIN[0] != temperatureIN[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", temperatureIN[1]/10, temperatureIN[1]%10);
					ST7735_DrawStringHorizontal(12, 30, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((12 + strlen(tempString)*2*6), 25, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", temperatureIN[0]/10, temperatureIN[0]%10);
					ST7735_DrawStringHorizontal(12, 30, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((12 + strlen(tempString)*2*6), 25, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					temperatureIN[1] = temperatureIN[0];
					tempCharacter[1] = tempCharacter[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((humidityIN[0] != humidityIN[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", humidityIN[1]/10, humidityIN[1]%10);
					ST7735_DrawStringHorizontal(74, 30, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((74 + strlen(tempString)*2*6), 27, '%', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", humidityIN[0]/10, humidityIN[0]%10);
					ST7735_DrawStringHorizontal(74, 30, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((74 + strlen(tempString)*2*6), 27, '%', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					humidityIN[1] = humidityIN[0];
				}

				//==========================================================================================================================


				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((temperatureOUT[0] != temperatureOUT[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", temperatureOUT[1]/10, temperatureOUT[1]%10);
					ST7735_DrawStringHorizontal(0, 70, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 68, tempCharacter, ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", temperatureOUT[0]/10, temperatureOUT[0]%10);
					ST7735_DrawStringHorizontal(0, 70, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 68, tempCharacter, ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					temperatureOUT[1] = temperatureOUT[0];
					tempCharacter[1] = tempCharacter[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((humidityOUT[0] != humidityOUT[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%02d.%.1d", humidityOUT[1]/10, humidityOUT[1]%10);
					ST7735_DrawStringHorizontal(0, 90, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 87, '%', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%02d.%.1d", humidityOUT[0]/10, humidityOUT[0]%10);
					ST7735_DrawStringHorizontal(0, 90, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 87, '%', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					humidityOUT[1] = humidityOUT[0];
				}

				if((lux[0] != lux[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%d", lux[1]);
					ST7735_DrawStringHorizontal(0, 112, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 112, 'L', ST7735_Color565(0, 0, 0), ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%d", lux[0]);
					ST7735_DrawStringHorizontal(0, 112, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawCharS((0 + strlen(tempString)*2*6), 112, 'L', ST7735_Color565(255, 0, 0), ST7735_Color565(255, 0, 0), 1);
					lux[1] = lux[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((pressure[0] != pressure[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%d", pressure[1]);
					ST7735_DrawStringHorizontal(0, 135, tempString, ST7735_Color565(0, 0, 0), 2);
					ST7735_DrawStringHorizontal((0 + strlen(tempString)*2*6), 135, "Pa", ST7735_Color565(0, 0, 0), 1);

					//Write the new string
					sprintf(tempString, "%d", pressure[0]);
					ST7735_DrawStringHorizontal(0, 135, tempString, ST7735_Color565(255, 0, 0), 2);
					ST7735_DrawStringHorizontal((0 + strlen(tempString)*2*6), 135, "Pa", ST7735_Color565(255, 0, 0), 1);
					pressure[1] = pressure[0];
				}

				//If the new data value is different than the previous value, or the screen has been changes, update display
				if((lightIndex[0] != lightIndex[1]) || refreshValues == YES)
				{
					//Re-write the last string in black (the same color as the backgruond) to cover it up so that the
					// new value is not written overtop.
					sprintf(tempString, "%s", lightIndexString[1]);
					ST7735_DrawStringVertical(105, 70, tempString, ST7735_Color565(0, 0, 0), 2);

					//Write the new string
					sprintf(tempString, "%s", lightIndexString[0]);
					ST7735_DrawStringVertical(105, 70, tempString, ST7735_Color565(0, 0, 255), 2);
					memcpy(lightIndexString[1], lightIndexString[0], sizeof(lightIndexString[0]));
					lightIndex[1] = lightIndex[0];
				}

				refreshValues = NO;
			}

			//Check to see in the knob has been changed at all
			encoderValue = EncoderDecipher(&Encoder1, &Encoder2, &PushButton);

			//If encoder has been held, this indicates enting the setting of the RTC time
			if(encoderValue == HOLD)
			{
				Setup = YES;	//Now that is this set high, the process will exit the while loop above

				//Set all varaibles to what is needed for editing the time and date
				memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
				memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));
				dateStringColor[0] = ST7735_Color565(255, 0, 0);
				dateStringColor[1] = ST7735_Color565(255, 0, 0);
				dateStringColor[2] = ST7735_Color565(255, 0, 0);

				//Getting the current RTC values and put them into variables that will then used to manipulate
				sprintf(temp, "%x, %x, %x, %x, %x, %x", newTime.month, newTime.dayOfmonth, newTime.year, newTime.hours, newTime.minutes, newTime.seconds);
				sscanf(temp, "%d, %d, %d, %d, %d, %d", &date[0], &date[1], &date[2], &time[0], &time[1], &time[2]);

				//Fill the screen in all black
				ST7735_FillScreen(0);
				//Variable controlling the whether editing the time or date
				Top = YES;
			}
			//If the user turns the knob, change the screen variable and set the refreshValues high so that the screen will change for the user
			else if (encoderValue == RIGHT || encoderValue == LEFT)
			{
				if(screen == 1)
				{
					screen = 2;
					ST7735_FillScreen(0);	//Clear screen
					refreshValues = YES;
				}
				else if(screen == 2)
				{
					screen = 1;
					ST7735_FillScreen(0); 	//Clear screen
					refreshValues = YES;
				}
			}
			//If the user just pressed the encoder (not holds it) change the temperature to be displayed in the opposite type
			else if(encoderValue == PRESS)
			{
				if(tempFormat == CEL)
				{
					tempFormat = FAR;
					tempCharacter[0] = 'F';
				}
				else
				{
					tempFormat = CEL;
					tempCharacter[0] = 'C';
				}
			}
		}

		//Keep process inside this loop while the encoder is not being touched
		while(!movement)
		{
			if(encoderRefresh)
			{
				//Getting the movement of the encoder
				movement = EncoderDecipher(Encoder1, Encoder2, PushButton);
				encoderRefresh = 0;
			}
			if(flag)
			{
				flag = 0;
				//Write the date and time strings
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

		//Is user turned the knob right, increment withever value the user is editing
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

		//Is user turned the knob left, decrement withever value the user is editing
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
						time[0] = 23;
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

		//Is user presses the right, swith which what is highlighted, to indicate to the user
		//what is being edited
		if(movement == PRESS)
		{
			flag = 1;

			if(Top)		// XXX XX XXXX
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
			else				//XX XX XX
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
		//Update the new time and date strings
		sprintf(dateString, "%s %02d,%d", months[date[0]], date[1], date[2]);
		sprintf(timeString, "%02d:%02d:%02d", time[0], time[1], time[2]);

		//If used holds the encoder down, the time and date will be written to the RTC and the process will return
		//to the top where the temp, humid... data is diplayed. Editing the time/date is exited.
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

				//Set the values (in the correct type) back into the RTC structure so that it can be updated.
				sprintf(temp, "%d, %d, %d, %d, %d, %d", date[0], date[1], date[2], time[0], time[1], time[2]);
				sscanf(temp, "%x, %x, %x, %x, %x, %x", &setTime.month, &setTime.dayOfmonth, &setTime.year, &setTime.hours, &setTime.minutes, &setTime.seconds);

				//Set the RTC with the values the uset chose
				MAP_RTC_C_initCalendar(&setTime, RTC_C_FORMAT_BCD);
				MAP_RTC_C_startClock();

				//Clear screen
				ST7735_FillScreen(0);
			}
		}
		movement = NONE;
	}
}
//-----------------------------------------------------------------------
void systick_isr(void)
{
	//Set the old value to 2nd index and new values of the encoder into the 1st index
	Encoder1[1] = Encoder1[0];
	Encoder2[1] = Encoder2[0];

	//Get values of the encoder
	Encoder1[0] = Debouncer(GPIO_PORT_P4, GPIO_PIN6);
	Encoder2[0] = Debouncer(GPIO_PORT_P6, GPIO_PIN5);
	PushButton[0] = Debouncer(GPIO_PORT_P6, GPIO_PIN4);

	//Variable controlling where the process should evaluate where there was movement or not
	encoderRefresh = 1;
}
//-----------------------------------------------------------------------
/* RTC ISR */
void rtc_isr(void)
{
    uint32_t status;

    //interupt hits every second
    tick = 1;
    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)
    {
    	//Toggle the LED of launchpad, get new time from RTC
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        newTime = MAP_RTC_C_getCalendarTime();
    }
}
//-----------------------------------------------------------------------
int DisplayInit (void)
{
	int err = 0;
	//Set the names of the months for when editing the date
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

	//reset variables used fro editing
	memset(dateStringColor, 0xFFFF, sizeof(dateStringColor));
	memset(timeStringColor, 0xFFFF, sizeof(timeStringColor));

	//initialize the screen
	Clock_Init48MHz();                   // set system clock to 48 MHz
	ST7735_InitR(INITR_GREENTAB);

	return err;
}
//------------------------------------------------------------------------------
int InitOneWire(void)
{
	int err = 0;

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

	//Setting RGB LED as output
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
	GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

	//Set P6.1 to be the pin interupt
	MAP_GPIO_setAsInputPin(GPIO_PORT_P6, GPIO_PIN1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P6, GPIO_PIN1);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P6, GPIO_PIN1);

	//Enable the gpio interupt
	MAP_Interrupt_enableInterrupt(INT_PORT6);
	MAP_Interrupt_enableMaster();

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
		//indicating there was a valid packet received
		status = 1;
	}
	if (rf_irq & RF24_IRQ_TXFAILED)
	{
		//indicating there was an invalid packet received
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
