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
#define HALF_SCREEN 120
#define HSYNC_BACKPORCH 18

#define  LUM_ON_OFF {DDRB = 0b11000; PORTB = 0b11000;PORTB = 0;}
// cycles i.e. LUM_ON and LUM_OFF must take exactly same time

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

uint8_t symbols[5][5];
uint8_t fonts[5][5];
// although we have uint8_t 8bits wide, the character set will be 5bits wide and we'll only draw 5bits from memory for each element
uint8_t screenMemory[YSIZE][XSIZE];


inline void putSymXY(uint8_t x,uint8_t y, uint8_t symbolNumber)
{
	for (uint8_t i = 0; i < 5; i++)
	{
		screenMemory[y+i][x] = symbols[symbolNumber][i];
	}
}
inline void putCharXY(uint8_t x,uint8_t y, uint8_t charNumber)
{
	for (uint8_t i = 0; i < 5; i++) // the fonts are 5lines high * 5bits wide
	{
		screenMemory[y+i][x] = fonts[charNumber][i];
	}
}

void clearScreen()
{
	for (uint8_t y = 0; y < YSIZE; y++)
	{
		for (uint8_t x = 0; x < XSIZE; x++)
			screenMemory[y][x] = 0;
	}
}

// cbi and sbi (clearbit and set bit both take 2 clock cycles, you can't use ldi on io port reg or from registers
#define  LUM_ON  {__asm__ __volatile__ ("sbi 0x05,4"); __asm__ __volatile__ ("sbi 0x04,4");}
#define  LUM_OFF {__asm__ __volatile__ ("cbi 0x05,4"); __asm__ __volatile__ ("cbi 0x04,4");}



int main()
{
	symbols[0][0] = 0b01010;
	symbols[0][1] = 0b10100;
	symbols[0][2] = 0b01010;
	symbols[0][3] = 0b10100;
	symbols[0][4] = 0b01010;

	symbols[1][0] = 0b11111;
	symbols[1][1] = 0b10001;
	symbols[1][2] = 0b10101;
	symbols[1][3] = 0b10101;
	symbols[1][4] = 0b10101;

	symbols[2][0] = 0b00000;
	symbols[2][1] = 0b01010;
	symbols[2][2] = 0b01010;
	symbols[2][3] = 0b00100;
	symbols[2][4] = 0b00100;

	symbols[3][0] = 0b11111;
	symbols[3][1] = 0b11111;
	symbols[3][2] = 0b11111;
	symbols[3][3] = 0b11111;
	symbols[3][4] = 0b11111;

	symbols[4][0] = 0b00000;
	symbols[4][1] = 0b00000;
	symbols[4][2] = 0b00000;
	symbols[4][3] = 0b00000;
	symbols[4][4] = 0b00000;

	fonts[0][0] = 0b00100; fonts[1][0] = 0b11110;fonts[2][0] = 0b01110;fonts[3][0] = 0b11110;
	fonts[0][1] = 0b01010; fonts[1][1] = 0b10001;fonts[2][1] = 0b10001;fonts[3][1] = 0b10001;
	fonts[0][2] = 0b11111; fonts[1][2] = 0b11110;fonts[2][2] = 0b10000;fonts[3][2] = 0b10001;
	fonts[0][3] = 0b10001; fonts[1][3] = 0b10001;fonts[2][3] = 0b10001;fonts[3][3] = 0b10001;
	fonts[0][4] = 0b10001; fonts[1][4] = 0b11110;fonts[2][4] = 0b01110;fonts[3][4] = 0b11110;

	fonts[4][0] = 0b11111; fonts[5][0] = 0b11111;fonts[6][0] = 0b01110;fonts[7][0] = 0b10001;
	fonts[4][1] = 0b10000; fonts[5][1] = 0b10000;fonts[6][1] = 0b10000;fonts[7][1] = 0b10001;
	fonts[4][2] = 0b11110; fonts[5][2] = 0b11110;fonts[6][2] = 0b10011;fonts[7][2] = 0b11111;
	fonts[4][3] = 0b10000; fonts[5][3] = 0b10000;fonts[6][3] = 0b10001;fonts[7][3] = 0b10001;
	fonts[4][4] = 0b11111; fonts[5][4] = 0b10000;fonts[6][4] = 0b01110;fonts[7][4] = 0b10001;

	fonts[4][0] = 0b11111; fonts[5][0] = 0b00111;fonts[6][0] = 0b10001;fonts[7][0] = 0b10000;
	fonts[4][1] = 0b00100; fonts[5][1] = 0b00001;fonts[6][1] = 0b10010;fonts[7][1] = 0b10000;
	fonts[4][2] = 0b00100; fonts[5][2] = 0b00001;fonts[6][2] = 0b11100;fonts[7][2] = 0b10000;
	fonts[4][3] = 0b00100; fonts[5][3] = 0b00001;fonts[6][3] = 0b10010;fonts[7][3] = 0b10000;
	fonts[4][4] = 0b11111; fonts[5][4] = 0b11110;fonts[6][4] = 0b10001;fonts[7][4] = 0b11111;

	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawLine = 0;
	uint8_t yPos = 0;

	clearScreen();

	for (uint8_t y = 2; y < YSIZE-10; y+=10)
	{
		for (uint8_t x = 0; x < XSIZE-1; x+=2)
		{
			putSymXY(x,y,3); // solid
			putSymXY(x+1,y,4); // space
			putSymXY(x,y+5,4);// space
			putSymXY(x+1,y+5,3); // solid
		}
	}


	// looking at the data sheet not sure if you have to do this in 3 instructions or not
	CLKPR = 0b10000000; // set the CLKPCE bit to enable the clock prescaler to be changed
	CLKPR = 0b10000000; // zero the CLKPS3 CLKPS2 CLKPS1 CLKPS0 bits to disable and clock division
	CLKPR = 0b00000000; // clear CLKPCE bit

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
			yPos++; // maintain a counter that starts at 1 for first line drawn
			{
				if ((yPos == 1) || (yPos == LAST_LINE_DRAWN-FIRST_LINE_DRAWN))
				{
					// delay some to bring the pixels into the visible screen area form the left
					for (i = 0; i < 20; i++)
					{
						__asm__ __volatile__ ("nop");
					}
					LUM_ON
					for (i = 0; i < 151; i++)
					{
						__asm__ __volatile__ ("nop");
					}
					LUM_OFF
				}
				else
				{
					// delay some to bring the pixels into the visible screen area form the left
					for (i = 0; i < 20; i++)
					{
						__asm__ __volatile__ ("nop");
					}
					LUM_ON
					// draw the smallest width line possible with these ddrb and portb setting macros (is still 2mm wide!)
					LUM_OFF

					// draw Pixels
					if (yPos < YSIZE)
					{
						for (uint8_t x = 0; x < XSIZE; x++)
						{
							uint8_t thebits = screenMemory[yPos][x];
							if (thebits & 0b10000) LUM_ON else LUM_OFF
							if (thebits & 0b01000) LUM_ON else LUM_OFF
							if (thebits & 0b00100) LUM_ON else LUM_OFF
							if (thebits & 0b00010) LUM_ON else LUM_OFF
							if (thebits & 0b00001) LUM_ON else LUM_OFF
						}
					}
					LUM_OFF
					// put another vertical bar at end to see what time is used processing screen memory
					for (i = 0; i < 10; i++)
					{
						__asm__ __volatile__ ("nop");
					}
					LUM_ON
					// draw the smallest width line possible with these ddrb and portb setting macros (is still 2mm wide!)
					LUM_OFF
				}
			}
		}

		lineCounter++;
		switch (lineCounter)
		{
			case FIRST_LINE_DRAWN: drawLine = 1; yPos = 0; break;
			case LAST_LINE_DRAWN: drawLine = 0; break;
			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; break;
			case MAX_LINE_BEFORE_BLANK: vSync = 0; break;
			case MAX_LINE_BEFORE_BLANK+6:lineCounter = 0; break;
		}
	}
}
