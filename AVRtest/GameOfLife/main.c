#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define SIZE 8 //Matrix Size

#define CLK 5 // 2 //Clock
#define DT 3 // 0 //Data
#define LTC 4 // 1 //Latch

volatile uint32_t tm = 0;

//Display Data
uint8_t tBuffer[SIZE]/* = { 0, 0, 0, 0, 0, 0, 0, 0 }*/;
uint8_t sBuffer[SIZE]/* =
{
	0b00000000,
	0b00000000,
	0b01000000,
	0b00100000,
	0b11100000,
	0b00000000,
	0b00000000,
	0b00000000
}*/;

static void Render() {
	int8_t iY, iX = 0; //For Loops Primary XY

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
}

static void Logic() {
	uint8_t iY, iX = 0; //For Loops Primary XY
	int8_t y0, x0 = 0; //For Loops Secondary XY

	uint8_t n = 0; //Neighbours

	for (iY = 0; iY < SIZE; iY++)
		tBuffer[iY] = sBuffer[iY];

	//Calculate
	for (iY = 0; iY < SIZE; iY++) {
		for (iX = 0; iX < SIZE; iX++) {
			n = 0;

			for (y0 = iY - 1; y0 <= iY + 1; y0++) {
				for (x0 = iX - 1; x0 <= iX + 1; x0++) {
					int wX = x0; if (x0 < 0) wX += SIZE; if (x0 >= SIZE) wX = 0;
					int wY = y0; if (y0 < 0) wY += SIZE; if (y0 >= SIZE) wY = 0;

					if (((tBuffer[wY] & (1 << (7 - (wX)))) >> (7 - (wX))) == 1)
						n++;
				}
			}

			if (n == 3)
				sBuffer[iY] |= (1 << (7 - iX));
			else if (n != 4)
				sBuffer[iY] &= ~(1 << (7 - iX));
		}
	}
}

ISR (TIMER0_COMPA_vect) {
	tm++;
}

ISR (ADC_vect) {
	ADCSRA |= (1 << ADSC);
}

int main(void) {
	DDRB = 0xff; //Set PortB as Output

	//Setup Timer
	TCCR0A |= (1 << WGM02);
	OCR0A = 98;
	TIMSK0 |= (1 << OCIE0A);

	sei();

	TCCR0B |= (1 << CS00) | (1 << CS02);

	//Setup ADC
	ADMUX |= (1<<MUX0) | (1<<MUX1); //Set Input Pin PB3
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); //Set Division Factor To 128
	ADCSRA |= (1<<ADEN); //Enable ADC
	ADCSRA |= (1<<ADSC); //Start Conversion
	while (ADCSRA & (1<<ADSC)); //Wait until conversion ended
	uint8_t seed = ADCL; //Generated Seed

	for (int i=0; i<SIZE; i++)
	{
		sBuffer[i] = seed;
		seed = ((seed*seed)+seed)%100;
	}

	//Setup ADC for Button Input
	ADMUX = 0; //Clear Multiplexer Selection Register
	ADCSRA = 0; //Clear Control and Status Register A

	ADMUX |= (1 << MUX1) | (1 << ADLAR);
	//ADCSRA |= (1<<ADPS2) | (1<<ADPS1); // | (1<<ADPS0); //Set Division Factor To 128
	ADCSRA |= (1 << ADEN); //Enable ADC
	ADCSRA |= (1 << ADIE); //Enable ADC Interupt

	sei();
	//Enable Global Interupt
	ADCSRA |= (1 << ADSC); //Start ADC

	while (1) {
		if (tm > 80) {
			Logic();
			tm = 0;
		}

		Render();
	}

	return 0;
}
