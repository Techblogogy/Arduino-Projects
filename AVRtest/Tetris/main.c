//DONE Add Full Row Detection
//DONE Left, Right Piece Movement
//DONE Add Left, Right Collision Detection
//DONE Redefine Pieces For New System
//DONE Add Random Block Spawn
//TODO Add Game Over Logic
//TODO Add Right Edge Detection
//DONE Add Rotation

#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define TMR_TCK 78 //156

#define SIZE 8 //Matrix Size

#define CLK 5 // 2 //Clock
#define DT 3 // 0 //Data
#define LTC 4 // 1 //Latch

//Timers Data
volatile uint32_t tm = 0; //Logic Timer Tick
volatile uint32_t bTm = 0; //Buttons Timer Tick

uint8_t seed;

//Display Data
uint8_t tBuffer[SIZE]; //Temporary Rendering Buffer
uint8_t sBuffer[SIZE]; //Main Rendering Buffer

//Store Tetris Data in EEPROM
//Block Structure { Amount Of Tetrominos, Width Of Tetromino, Tetromino Data }
#define BLN 7
uint8_t EEMEM Blk[BLN][9] = {
		{ 2, 4,0b11110000, 2,0b10101010 },
		{ 4, 4,0b10001110, 2,0b11101000, 4,0b11100010, 2,0b01011100 },
		{ 4, 4,0b00101110, 2,0b10101100, 4,0b11101000, 2,0b11010100 },
		{ 1, 2,0b11110000 },
		{ 2, 4,0b01101100, 2,0b10110100 },
		{ 4, 4,0b01001110, 2,0b10111000, 4,0b11100100, 2,0b01110100 },
		{ 2, 4,0b11000110, 2,0b01111000 }
};

uint8_t trs[9]; //Current Tetris Object

uint8_t oldId = 0;
uint8_t id = 0; //Tetris Object Id

uint8_t xS, yS = 0; //XY Position Of Tetris Object
uint8_t oldX,oldY = 0;

uint8_t b = 0; //Collision Boolean

static void Render() {
	int8_t iY, iX = 0; //For Loops Primary XY

	cli(); //Disable Interrupt

	for (iY = 0; iY < SIZE; iY++) {
		for (iX = 7; iX >= 4; iX--) {
			if (((sBuffer[iX] & (1 << (7 - iY))) >> (7 - iY)) == 1)
				PORTB |= _BV(DT);
			else
				PORTB &= ~_BV(DT);

			PORTB |= _BV(CLK); //Clock HIGH
			PORTB &= ~_BV(CLK); //Clock LOW
		}
		for (iX = 3; iX >= 0; iX--) {
			if (iX == iY)
				PORTB &= ~_BV(DT);
			else
				PORTB |= _BV(DT);

			PORTB |= _BV(CLK); //Clock HIGH
			PORTB &= ~_BV(CLK); //Clock LOW
		}

		for (iX = 0; iX < 4; iX++) {
			if (((sBuffer[iX] & (1 << (7 - iY))) >> (7 - iY)) == 1)
				PORTB |= _BV(DT);
			else
				PORTB &= ~_BV(DT);

			PORTB |= _BV(CLK); //Clock HIGH
			PORTB &= ~_BV(CLK); //Clock LOW
		}
		for (iX = 4; iX < 8; iX++) {
			if (iX == iY)
				PORTB &= ~_BV(DT);
			else
				PORTB |= _BV(DT);

			PORTB |= _BV(CLK); //Clock HIGH
			PORTB &= ~_BV(CLK); //Clock LOW
		}

		PORTB |= _BV(LTC); //Latch HIGH
		PORTB &= ~_BV(LTC); //Latch LOW
	}

	sei();  //Disable Interrupt
}

static void RenderB() {
	b = 0;

	//Old Params
	uint8_t s = trs[id + 1] * trs[id + 1]; //Mask Increment Value
	uint8_t sM = s - 1; //Mask Value

	uint8_t sA = 8 - trs[id + 1]; //Shift Ammont
	uint8_t h = (8 / trs[id + 1]); //Height

	cli();
	//Rows Rendering Loop
	while (sM) {
		uint8_t rw = ((trs[id + 2] & sM) << sA) >> oldX; //Apply Mask To Row

		if (id==oldId)
			tBuffer[oldY + h - 1] &= ~rw; //Clear Previous Row

		if (xS >= oldX)
			rw = rw >> (xS-oldX);
		else if (xS < oldX)
			rw = rw << (oldX-xS);

		//Collision Check
		if ( ((tBuffer[yS + h - 1] & rw) != 0) || ( rw != 0 && (yS+h-1) >= 8) || ( (rw&1) != 0 && xS+trs[id+1]-1 >= 8 ) ) {
			if (oldY != yS) {
				xS = 0; //Reset X
				oldX = 0;

				yS = 0; //Reset Y
				oldY = 0;

				id = 0;
				oldId = 0;

				//Get New Random Block
				eeprom_read_block((void*) &trs, (const void*) &Blk[rand()%7], 9);

				//Clean Rows
				int i=SIZE-1;
				while (i>=0)
				{
					if (sBuffer[i] == 0xff){
						for (int j=i; j>=0; j--){
							if (j!=0)
								sBuffer[j] = sBuffer[j-1];
							else
								sBuffer[j] = 0;
						}
					}else{
						i--;
					}
				}
			}

			if (oldX != xS) {
				xS = oldX;
			}

			if (id != oldId) {
				id = oldId;
			}

			b = 1;
			break;
		}

		tBuffer[yS + h - 1] |= rw; //Set Temporary Buffer Row

		sA -= trs[id + 1]; //Decrement Shift Amount By Width
		sM *= s; //Increment Mask
		h--; //Decrement Height
	}

	oldId = id;

	oldX = xS; //Save Current X
	oldY = yS; //Save Current Y

	sei();

	//Swap Buffers
	for (int i=0; i<SIZE; i++) {
		if (b==0)
			sBuffer[i] = tBuffer[i];
		else
			tBuffer[i] = sBuffer[i];
	}
}

static void Logic() {
	if (b == 0) {
		yS++;
	}
}

ISR (TIMER0_COMPA_vect) {
	tm++;
	bTm++;
}

int main(void) {
	DDRB = 0xff; //Set PortB Pins
	DDRD = 0; //Set PortD as output

	//Setup Timer
	TCCR0A |= (1 << WGM01); //Set Timer To CTC Mode
	TCCR0B |= (1 << CS00) | (1 << CS02); //Set Scale Factor To Clk/1024
	OCR0A = TMR_TCK; //Set Timer Interval
	TIMSK0 |= (1 << OCIE0A); //Set Timer Interrupt On OCR0A Match

	//Setup ADC for Randomization
	ADMUX |= (1 << MUX0) | (1 << MUX1); //Set Input Pin PB3
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //Set Division Factor To 128
	ADCSRA |= (1 << ADEN); //Enable ADC
	ADCSRA |= (1 << ADSC); //Start Conversion
	while (ADCSRA & (1 << ADSC)); //Wait until conversion ended
	seed = ADCL; //Generated Random Seed

	srand(seed); //Random Seed

	sei();

	//seed = ((seed*seed)+seed)%100;
	eeprom_read_block((void*) &trs, (const void*) &Blk[rand()%BLN], 9);

	//xS = 5;

	RenderB();

	while (1) {
		//Button Check
		if ((PIND & (1 << 1)) && (bTm >= 25)) { //Move Right
			xS++;
			bTm = 0;
		} else if ((PIND & (1 << 0)) && (bTm >= 25) && ((xS-1) >= 0) ) { //Move Left
			xS--;
			bTm = 0;
		} else if ((PIND & (1 << 2)) && (bTm >= 25) ) { //Rotate
			oldId = id;

			if (id+2 >= trs[0]*2)
				id=0;
			else
				id+=2;

			//Init Clean Rows Data
			uint8_t s = trs[oldId + 1] * trs[oldId + 1]; //Mask Increment Value
			uint8_t sM = s - 1; //Mask Value

			uint8_t sA = 8 - trs[oldId + 1]; //Shift Ammont
			uint8_t h = (8 / trs[oldId + 1]); //Height

			cli();
			//Clean Old Rows
			while (sM) {
				uint8_t rw = ((trs[oldId + 2] & sM) << sA) >> oldX; //Apply Mask To Row
				tBuffer[oldY + h - 1] &= ~rw; //Clear Previous Row

				sA -= trs[oldId + 1]; //Decrement Shift Amount By Width
				sM *= s; //Increment Mask
				h--; //Decrement Height
			}
			sei();

			bTm = 0;
		}

		if (oldX != xS || oldY != yS || oldId != id || b==1)
			RenderB();

		if (tm > 100) {
			Logic();
			tm = 0;
		}

		Render();
	}

	return 0;
}
