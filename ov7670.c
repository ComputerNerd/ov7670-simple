#include "ov7670.h"
#define F_CPU 16000000UL
#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
/*register uint8_t nerd1 asm("r16");
register uint8_t nerd2 asm("r29");
register uint8_t nerd3 asm("r31");*/
void error_led(void)
{
	DDRB|=32;//make sure led is output
	PORTB|=32;//led on
	while (1) {}//wait for reset
}
void twiStart(void)
{
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);//send start
	while (!(TWCR & (1<<TWINT))) {}//wait for start to be transmitted
	if ((TWSR & 0xF8) != TW_START)
		error_led();
}
void twiWriteByte(uint8_t DATA)
{
	TWDR = DATA;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & (1<<TWINT))) {}
	if ((TWSR & 0xF8) != TW_MT_DATA_ACK)
		error_led();
}
void twiAddr(uint8_t addr,uint8_t typeTWI)
{
	//This function does not do error checking
	TWDR = camAddr_WR;//send address
	TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
	while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
	if ((TWSR & 0xF8) != typeTWI)
		error_led();
}
void wrReg(uint8_t reg,uint8_t dat)
{
	//send start condition
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg);
	twiWriteByte(dat);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
}
uint8_t rdReg(uint8_t reg)
{
	/*
	Wire.beginTransmission(sensor_addr >> 1);
	Wire.write(regID & 0x00FF); 	
	Wire.endTransmission();
	Wire.requestFrom((sensor_addr >> 1),1);
	if(Wire.available())
		*regDat = Wire.read(); 	
	delay(1);
  	return(1);
	*/
	uint8_t dat;
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	twiStart();
	twiAddr(camAddr_RD,TW_MR_SLA_ACK);
	TWCR = (1 << TWINT) | (1 << TWEN);//nack
	while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
	dat = TWDR;
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
	return dat;
}

