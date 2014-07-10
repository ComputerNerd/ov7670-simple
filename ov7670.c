#include "ov7670.h"
#define F_CPU 16000000UL
#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
static void error_led(void){
	DDRB|=32;//make sure led is output
	while(1){//wait for reset
		PORTB^=32;// toggle led
		_delay_ms(100);
	}
}
static void twiStart(void){
	TWCR=_BV(TWINT)| _BV(TWSTA)| _BV(TWEN);//send start
	while(!(TWCR & (1<<TWINT)));//wait for start to be transmitted
	if((TWSR & 0xF8)!=TW_START)
		error_led();
}
static void twiWriteByte(uint8_t DATA,uint8_t type){
	TWDR = DATA;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & (1<<TWINT))) {}
	if ((TWSR & 0xF8) != type)
		error_led();
}
static void twiAddr(uint8_t addr,uint8_t typeTWI){
	TWDR = addr;//send address
	TWCR = _BV(TWINT) | _BV(TWEN);		/* clear interrupt to start transmission */
	while ((TWCR & _BV(TWINT)) == 0);	/* wait for transmission */
	if ((TWSR & 0xF8) != typeTWI)
		error_led();
}
void wrReg(uint8_t reg,uint8_t dat){
	//send start condition
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg,TW_MT_DATA_ACK);
	twiWriteByte(dat,TW_MT_DATA_ACK);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
}
static uint8_t twiRd(uint8_t nack){
	if (nack){
		TWCR=_BV(TWINT) | _BV(TWEN);
		while ((TWCR & _BV(TWINT)) == 0);	/* wait for transmission */
	if ((TWSR & 0xF8) != TW_MR_DATA_NACK)
		error_led();
		return TWDR;
	}else{
		TWCR=_BV(TWINT) | _BV(TWEN) | _BV(TWEA);
		while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
		if ((TWSR & 0xF8) != TW_MR_DATA_ACK)
			error_led();
		return TWDR;
	}
}
uint8_t rdReg(uint8_t reg){
	uint8_t dat;
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg,TW_MT_DATA_ACK);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
	twiStart();
	twiAddr(camAddr_RD,TW_MR_SLA_ACK);
	dat=twiRd(1);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
	return dat;
}
