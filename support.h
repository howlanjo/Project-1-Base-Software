#ifndef SUPPORT_H_
#define SUPPORT_H_

#define YES		1
#define NO		0

#define	NONE	0
#define	LEFT	1
#define	RIGHT	2
#define	PRESS	3
#define	HOLD	4

volatile RTC_C_Calendar newTime;

uint8_t Debouncer(uint_fast8_t portPtr, uint_fast8_t pinPtr);
void InitFunction(void);
int binaryToDecimal(int bin1, int bin2);
int EncoderDecipher(uint8_t encoder1[], uint8_t encoder2[], uint8_t pushbutton[]);



#endif /* SUPPORT_H_ */
