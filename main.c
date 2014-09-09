#define F_CPU 16000000UL
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "ov7670.h"
//#define qqvga
//#define qvga
//#define rgb565
#define rawRGB
static inline void spiCSt(void){//selects the RAM chip and resets it
	//toggles spi CS used for reseting sram
	PORTB|=4;//cs high
	PORTB&=~4;//cs low
}
static inline void spiWrB(uint8_t dat){
	SPDR=dat;
	while(!(SPSR & (1<<SPIF)));// Wait for transmission complete
}
static inline void serialWrB(uint8_t dat){
	while(!( UCSR0A & (1<<UDRE0)));//wait for byte to transmit
	UDR0=dat;
	while(!( UCSR0A & (1<<UDRE0)));//wait for byte to transmit
}
static void StringPgm(char * str){
	do{
		serialWrB(pgm_read_byte_near(str));
	}while(pgm_read_byte_near(++str));
}
static void captureImg(uint16_t ws,uint16_t hs,uint16_t wg,uint8_t hg){
	//first wait for vsync it is on pin 3 (counting from 0) portD
	uint16_t ls2,lg2;
	spiCSt();
	spiWrB(2);/* Configure the spi ram to use sequential write mode */
	spiWrB(0);/* Because there is 128kb of ram the chip uses a 24 bit address  so that is why there are three byte writes */
	spiWrB(0);
	spiWrB(0);

	/* Skip pixels */
	while(!(PIND&8));//wait for high
	while((PIND&8));//wait for low
	if(hs){
		while(hs--){
			ls2=ws;
			while(ls2--){
				while((PIND&4));//wait for low
				while(!(PIND&4));//wait for high
			}
		}
	}
	
	/* Read pixels to SPI ram */
	while(hg--){
		lg2=wg;
		while(lg2--){
			while((PIND&4));//wait for low
			SPDR=(PINC&15)|(PIND&240);
			while(!(PIND&4));//wait for high
		}
	}
}
static void sendRam(uint16_t w,uint16_t h){
	spiCSt();
	spiWrB(3);//sequential read mode
	spiWrB(0);//24 bit address
	spiWrB(0);
	spiWrB(0);
	//_delay_ms(2000);
	uint16_t wl,hl;
	for (hl=0;hl<h;++hl){
		StringPgm((char *)PSTR("RDY"));
		for (wl=0;wl<w;++wl){
			while (!(UCSR0A & (1<<UDRE0)));//wait for byte to transmit
			SPDR=0;//send dummy value to get byte back
			while(!(SPSR & (1<<SPIF)));
			UDR0=SPDR;
		}
	}
	while(!(UCSR0A&(1<<UDRE0))); //wait for byte to transmit
}
int main(void){
	
	cli();//disable interrupts
	DDRB|=47;//clock as output and SPI pins as output except MISO
	PORTB|=6;//set both CS pins to high
	DDRC&=~15;//low d0-d3 camera
	DDRD&=~252;//d7-d4 and interrupt pins
	//set up twi for 100khz
	TWSR&=~3;//disable prescaler for TWI
	TWBR=72;//set to 100khz
	//enable serial
	UBRR0H=0;
	UBRR0L=3;//3 = 0.5M 2M baud rate = 0 7 = 250k 207 is 9600 baud rate
	UCSR0A|=2;//double speed aysnc
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);//Enable receiver and transmitter
	UCSR0C=6;//async 1 stop bit 8bit char no parity bits
	//enable spi
	SPCR=80;//spi enable master
	SPSR=1;//double speed
	//set up camera
	wrReg(0x15,32);//pclk does not toggle on HBLANK COM10
	//wrReg(0x11,32);//Register 0x11 is for pixel clock divider
	wrReg(REG_RGB444, 0x00);// Disable RGB444
	wrReg(REG_COM11,226);//enable night mode 1/8 frame rate COM11*/
	//wrReg(0x2E,63);//Longer delay
	wrReg(REG_TSLB,0x04);				// 0D = UYVY  04 = YUYV	 
 	wrReg(REG_COM13,0x88);			   // connect to REG_TSLB
	//wrReg(REG_COM13,0x8);			   // connect to REG_TSLB disable gamma
	#ifdef rgb565
		wrReg(REG_COM7, 0x04);		   // RGB + color bar disable 
		wrReg(REG_COM15, 0xD0);		  // Set rgb565 with Full range	0xD0
	#elif defined rawRGB
		wrReg(REG_COM7,1);//raw rgb bayer
		wrReg(REG_COM15, 0xC0);		  //Full range
	#else
		wrReg(REG_COM7, 0x00);		   // YUV
		//wrReg(REG_COM17, 0x00);		  // color bar disable
		wrReg(REG_COM15, 0xC0);		  //Full range
	#endif
	//wrReg(REG_COM3, 0x04);
	#if defined qqvga || defined qvga
		wrReg(REG_COM3,4);	// REG_COM3 
	#else
		wrReg(REG_COM3,0);	// REG_COM3
	#endif
	//wrReg(0x3e,0x00);		//  REG_COM14
	//wrReg(0x72,0x11);		//
	//wrReg(0x73,0xf0);		//
	//wrReg(REG_COM8,0x8F);		// AGC AWB AEC Unlimited step size
	/*wrReg(REG_COM8,0x88);//disable AGC disable AEC
	wrReg(REG_COM1, 3);//manual exposure
	wrReg(0x07, 0xFF);//manual exposure
	wrReg(0x10, 0xFF);//manual exposure*/
	#ifdef qqvga
		wrReg(REG_COM14, 0x1a);		  // divide by 4
		wrReg(0x72, 0x22);			   // downsample by 4
		wrReg(0x73, 0xf2);			   // divide by 4
		wrReg(REG_HSTART,0x16);
		wrReg(REG_HSTOP,0x04);
		wrReg(REG_HREF,0xa4);		   
		wrReg(REG_VSTART,0x02);
		wrReg(REG_VSTOP,0x7a);
		wrReg(REG_VREF,0x0a);	
	#endif
	#ifdef qvga
	wrReg(REG_COM14, 0x19);		 
		wrReg(0x72, 0x11);	
		wrReg(0x73, 0xf1);
		wrReg(REG_HSTART,0x16);
		wrReg(REG_HSTOP,0x04);
		wrReg(REG_HREF,0x24);			
		wrReg(REG_VSTART,0x02);
		wrReg(REG_VSTOP,0x7a);
		wrReg(REG_VREF,0x0a);
	#else
		wrReg(0x32,0xF6);		// was B6  
		wrReg(0x17,0x13);		// HSTART
		wrReg(0x18,0x01);		// HSTOP
		wrReg(0x19,0x02);		// VSTART
		wrReg(0x1a,0x7a);		// VSTOP
		//wrReg(0x03,0x0a);		// VREF
	wrReg(REG_VREF,0xCA);//set 2 high GAIN MSB
	#endif
	//wrReg(0x70, 0x3a);	   // Scaling Xsc
	//wrReg(0x71, 0x35);	   // Scaling Ysc
	//wrReg(0xA2, 0x02);	   // pixel clock delay
	//Color Settings
	//wrReg(0,0xFF);//set gain to maximum possible
	//wrReg(0xAA,0x14);			// Average-based AEC algorithm
	wrReg(REG_BRIGHT,0x00);	  // 0x00(Brightness 0) - 0x18(Brightness +1) - 0x98(Brightness -1)
	wrReg(REG_CONTRAS,0x40);	 // 0x40(Contrast 0) - 0x50(Contrast +1) - 0x38(Contrast -1)
	//wrReg(0xB1,0xB1);			// Automatic Black level Calibration
	wrReg(0xb1,4);//really enable ABLC
	wrReg(MTX1,0x80);
	wrReg(MTX2,0x80);
	wrReg(MTX3,0x00);
	wrReg(MTX4,0x22);
	wrReg(MTX5,0x5e);
	wrReg(MTX6,0x80);
	wrReg(MTXS,0x9e);
	wrReg(AWBC7,0x88);
	wrReg(AWBC8,0x88);
	wrReg(AWBC9,0x44);
	wrReg(AWBC10,0x67);
	wrReg(AWBC11,0x49);
	wrReg(AWBC12,0x0e);
	wrReg(REG_GFIX,0x00);
	//wrReg(GGAIN,0);
	wrReg(AWBCTR3,0x0a);
	wrReg(AWBCTR2,0x55);
	wrReg(AWBCTR1,0x11);
	wrReg(AWBCTR0,0x9f);
	//wrReg(0xb0,0x84);//not sure what this does
	wrReg(REG_COM16,COM16_AWBGAIN);//disable auto denoise and edge enhancement
	//wrReg(REG_COM16,0);
	wrReg(0x4C,0);//disable denoise
	wrReg(0x76,0);//disable denoise
	wrReg(0x77,0);//disable denoise
	wrReg(0x7B,4);//brighten up shadows a bit end point 4
	wrReg(0x7C,8);//brighten up shadows a bit end point 8
	//wrReg(0x88,238);//darken highlights end point 176
	//wrReg(0x89,211);//try to get more highlight detail
	//wrReg(0x7A,60);//slope
	//wrReg(0x26,0xB4);//lower maximum stable operating range for AEC
	//hueSatMatrix(0,100);
	//ov7670_store_cmatrix();
	//wrReg(0x20,12);//set ADC range to 1.5x
	wrReg(REG_COM9,0x6A);//max gain to 128x
	wrReg(0x74,16);//disable digital gain
	//wrReg(0x93,15);//dummy line MSB
	wrReg(0x11,4);
	//wrReg(0x2a,5);//href delay
	spiCSt();
	spiWrB(1);
	spiWrB(64);//sequential mode
	spiCSt();
	spiWrB(2);//sequential write mode
	spiWrB(0);//24 bit address
	spiWrB(0);
	spiWrB(0);
	while (1){
		/* In this example we only have 128kb of ram not enough to hold one image unless you want qqvga
		 * This is very low resolution most people will want a higher resolution
		 * To achieve this we need to divide the image up into multiple parts
		 * A good way to get a 640x480 image without diving the image up into too many parts is to use raw bayer data
		 * This means we only need to send three parts instead of five.
		 * Also there are theoretical quality advantages.
		 * A good demosaicing algorithm may outperform the built-in demosaicing that the ov7670 does */
		#ifdef qqvga
			captureImg(0,0,320,120);
			sendRam(320,120);
		#endif
		#ifdef qvga
			captureImg(0,0,640,120);
			sendRam(640,120);
			captureImg(640,120,640,120);
			sendRam(640,120);
		#else
			#ifdef rawRGB
				captureImg(0,0,640,160);
				sendRam(640,160);
				captureImg(640,160,640,160);
				sendRam(640,160);
				captureImg(640,320,640,160);
				sendRam(640,160);
			#else
				/* This function operates in bytes not pixels in this case pixels are two bytes per pixel
				 * so that is why you see 1280 used instead of 640 */ 
				captureImg(0,0,1280,96);
				sendRam(1280,96);
				captureImg(1280,96,1280,96);
				sendRam(1280,96);
				captureImg(1280,192,1280,96);
				sendRam(1280,96);
				captureImg(1280,288,1280,96);
				sendRam(1280,96);
				captureImg(1280,384,1280,96);
				sendRam(1280,96);
			#endif
		#endif
		
	}
	
}
