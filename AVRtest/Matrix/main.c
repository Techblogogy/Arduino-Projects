#define F_CPU 10000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

#define SIZE 8

//Stores Coluoms
uint8_t tBuffer[SIZE] = {0,0,0,0,0,0,0,0};
uint8_t sBuffer[SIZE] =
{
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00000000
};

int8_t iY, iX = 0; //For Loops Primary XY
int8_t y0, x0 = 0; //For Loops Secondary XY
int8_t n = 0; //Neighbours

volatile uint32_t tm = 0;

static void Render()
{
	for (iY = 0; iY < SIZE; iY++)
	{
		for (iX = 7; iX >= 4; iX--)
		{
			if ( ((sBuffer[iX] & (1 << (7-iY))) >> (7-iY)) == 1)
				PORTB |= _BV(0);
			else
				PORTB &= ~_BV(0);

			PORTB |= _BV(2); //Clock HIGH
			PORTB &= ~_BV(2); //Clock LOW
		}

		for (iX = 3; iX >= 0; iX--)
		{
			if (iX == iY)
				PORTB &= ~_BV(0);
			else
				PORTB |= _BV(0);

			PORTB |= _BV(2); //Clock HIGH
			PORTB &= ~_BV(2); //Clock LOW
		}

		for (iX = 0; iX < 4; iX++)
		{
			if ( ((sBuffer[iX] & (1 << (7-iY))) >> (7-iY)) == 1)
				PORTB |= _BV(0);
			else
				PORTB &= ~_BV(0);

			PORTB |= _BV(2); //Clock HIGH
			PORTB &= ~_BV(2); //Clock LOW
		}
		for (iX = 4; iX < 8; iX++)
		{
			if (iX == iY)
				PORTB &= ~_BV(0);
			else
				PORTB |= _BV(0);

			PORTB |= _BV(2); //Clock HIGH
			PORTB &= ~_BV(2); //Clock LOW
		}

		PORTB |= _BV(1); //Latch HIGH
		PORTB &= ~_BV(1); //Latch LOW
	}
}

static void Logic()
{
	//Copy temp buffer with screen buffer
	for (iY = 0; iY < SIZE; iY++)
		tBuffer[iY] = sBuffer[iY];

	//Calculate
	for (iY = 0; iY < SIZE; iY++) {
		for (iX = 0; iX < SIZE; iX++) {
			n = 0;

			//Calculate Neigbours
			for (y0 = iY - 1; y0 <= iY + 1; y0++) {
				for (x0 = iX - 1; x0 <= iX + 1; x0++) {
					if ((iY != y0 || iX != x0) && (x0 >= 0 && x0 < SIZE && y0 >= 0 && y0 < SIZE)) {
						if ( ((tBuffer[y0] & (1 << (7-x0))) >> (7-x0))  == 1)
							n++;
					}
				}
			}

			//Apply Rules
			if ( ((tBuffer[iY] & (1 << (7-iX))) >> (7-iX)) == 1) {
				if (n < 2)
					sBuffer[iY] &= ~(1 << (7-iX));
				else if (n == 2 || n == 3)
					sBuffer[iY] |= (1 << (7-iX));
				else if (n > 3)
					sBuffer[iY] &= ~(1 << (7-iX));
			} else {
				if (n == 3)
					sBuffer[iY] |= (1 << (7-iX));
			}
		}
	}
}

ISR (TIM0_OVF_vect)
{
    tm++;
}

int main(void)
{
	DDRB = 0xFF; //Set PortB as Output

	TCCR0B |= (1<<CS02) | (1<<CS00);
	TIMSK0 |= 1<<TOIE0;

	sei();

	while (1)
	{
//		if (tm > 1)
//		{
//			Logic();
//			tm = 0;
//		}

		Render();
	}

	return 0;
}
