#ifndef SUPPORT_H_
#define SUPPORT_H_

#define YES		1
#define NO		0

#define	NONE	0
#define	LEFT	1
#define	RIGHT	2
#define	PRESS	3
#define	HOLD	4

#define TIMER_PERIOD    12000u //  = 1ms @ 12 MHz

/* Timer_A UpMode Configuration Parameter */
typedef union {
  struct {
    uint8_t hh;
    uint8_t hl;
    uint8_t th;
    uint8_t tl;
    uint8_t crc;
  } val;
  uint8_t bytes[5];
} dht22data;

dht22data dht_data;
uint8_t dht_data_byte, dht_data_bit;

enum {
  DHT_IDLE = 0,
  DHT_TRIGGERING,
  DHT_WAITING_ACK,
  DHT_ACK_LOW,
  DHT_ACK_HIGH,
  DHT_IN_BIT_LOW,
  DHT_IN_BIT_HIGH,
} dht_current_state;

volatile RTC_C_Calendar newTime;

void dht_start_read();
int dht_get_temp();
int dht_get_rh();
uint8_t Debouncer(uint_fast8_t portPtr, uint_fast8_t pinPtr);
void InitFunction(void);
int binaryToDecimal(int bin1, int bin2);
int EncoderDecipher(uint8_t encoder1[], uint8_t encoder2[], uint8_t pushbutton[]);
void calculateLighting(uint16_t value, char stringOUT[30], int8_t *index);



#endif /* SUPPORT_H_ */
