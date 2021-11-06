// PAL video output using atmega328p.
// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312
#define FIRST_LINE_DRAWN 32
#define LAST_LINE_DRAWN 272
#define HALF_SCREEN 110
#define HSYNC_BACKPORCH 18

#define  LUM_ON_OFF {DDRB = 0b11000; PORTB = 0b11000;PORTB = 0;}
// cycles i.e. LUM_ON and LUM_OFF must take exactly same time
#define  LUM_ON { 	__asm__ __volatile__ ("sbi 0x05,4"); __asm__ __volatile__ ("sbi 0x04,4");}

// cycles i.e. LUM_ON and LUM_OFF must take exactly same time
#define  LUM_OFF {__asm__ __volatile__ ("cbi 0x05,4"); __asm__ __volatile__ ("cbi 0x04,4"); }

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

uint8_t symbols[5][8];
uint8_t screenMemory[YSIZE][XSIZE];


inline void putSymXY(uint8_t x,uint8_t y, uint8_t symbolNumber)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		screenMemory[y+i][x] = symbols[symbolNumber][i];
	}
}

void clearScreen()
{
	memset (&screenMemory[0][0],0,sizeof(uint8_t) * YSIZE * XSIZE );
}

int main()
{
	symbols[0][0] = 0b00000000;
	symbols[0][1] = 0b01111110;
	symbols[0][2] = 0b01000010;
	symbols[0][3] = 0b01000010;
	symbols[0][4] = 0b01000010;
	symbols[0][5] = 0b01000010;
	symbols[0][6] = 0b01111110;
	symbols[0][7] = 0b00000000;

	symbols[1][0] = 0b11111111;
	symbols[1][1] = 0b10000001;
	symbols[1][2] = 0b10111101;
	symbols[1][3] = 0b10100101;
	symbols[1][4] = 0b10100101;
	symbols[1][5] = 0b10111101;
	symbols[1][6] = 0b10000001;
	symbols[1][7] = 0b11111111;

	symbols[2][0] = 0b11011011;
	symbols[2][1] = 0b00000000;
	symbols[2][2] = 0b01100110;
	symbols[2][3] = 0b00000000;
	symbols[2][4] = 0b00011000;
	symbols[2][5] = 0b00011000;
	symbols[2][6] = 0b10000001;
	symbols[2][7] = 0b11111111;

	symbols[3][0] = 0b11111111;
	symbols[3][1] = 0b11111111;
	symbols[3][2] = 0b11111111;
	symbols[3][3] = 0b11111111;
	symbols[3][4] = 0b11111111;
	symbols[3][5] = 0b11111111;
	symbols[3][6] = 0b11111111;
	symbols[3][7] = 0b11111111;

	symbols[4][0] = 0b00000000;
	symbols[4][1] = 0b00000000;
	symbols[4][2] = 0b00000000;
	symbols[4][3] = 0b00000000;
	symbols[4][4] = 0b00000000;
	symbols[4][5] = 0b00000000;
	symbols[4][6] = 0b00000000;
	symbols[4][7] = 0b00000000;


	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawLine = 0;

	for (uint8_t y = 0; y < YSIZE-16; y+=16)
	{
		for (uint8_t x = 0; x < XSIZE; x+=2)
		{
			putSymXY(x,y,3);
			putSymXY(x+1,y,4);
			putSymXY(x,y+8,4);
			putSymXY(x+1,y+8,3);
		}
	}


	// looking at the data sheet not sure if you have to do this in 3 instructions or not
	CLKPR = 0b10000000; // set the CLKPCE bit to enable the clock prescaler to be changed
	CLKPR = 0b10000000; // zero the CLKPS3 CLKPS2 CLKPS1 CLKPS0 bits to disable and clock division
	CLKPR = 0b00000000; // clear CLKPCE bit

	clearScreen();

	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10);
	OCR1A = 1025; 	   //timer interrupt cycles which gives rise to 64usec line sync time
	TCNT1 = 0;
	while(1)
	{
		// wait for hsync timer interrupt to trigger
		while((TIFR1 & (1<<OCF1A)) == 0)
		{
			// wait till the timer overflow flag is SET
		}


		// immediately reset the interrupt timer, previous version had this at the end
		// which made the whole thing be delayed when we're trying to maintain the 64us PAL line timing
		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag

		if (vSync == 1) // invert the line sync pulses when in vsync part of screen
		{
			DDRB &= ~(1 << COMPOSITE_PIN);
			for (i = 0; i < HSYNC_BACKPORCH+10; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB &= ~(1 << COMPOSITE_PIN);
			DDRB |= (1 << COMPOSITE_PIN);

		}
		else // hsync
		{
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB = 0;
			PORTB = 1;
			for (i = 0; i < 1; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < HSYNC_BACKPORCH; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1;
			for (i = 0; i < HSYNC_BACKPORCH; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}
		}

		if (drawLine)
		{
			{
				// delay some to bring the pixels into the visible screen area form the left
				for (i = 0; i < 20; i++)
				{
					__asm__ __volatile__ ("nop");
				}
				LUM_ON

				// draw the smallest width line possible with these ddrb and portb setting macros (is still 2mm wide!)

				LUM_OFF
			}

		}

		lineCounter++;
		switch (lineCounter)
		{
			case FIRST_LINE_DRAWN: drawLine = 1; break;
			case LAST_LINE_DRAWN: drawLine = 0; break;
			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; break;
			case MAX_LINE_BEFORE_BLANK+6: lineCounter = 0; vSync = 0; break;
		}
	}
}


#if 0
else
{
	DDRB |= (1 << LUMINANCE_PIN); // pixel on
	PORTB |= (1 << LUMINANCE_PIN); // pixel on
	for (i = 0; i < 205; i++)
	{
		__asm__ __volatile__ ("nop");
	}

	DDRB &= ~(1 << LUMINANCE_PIN); // pixel off
	PORTB &= ~(1 << LUMINANCE_PIN); // pixel off
}

#endif
