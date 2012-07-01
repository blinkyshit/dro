//
// Copyright Pierre Michael (c) 2012
// for ATmega328P Adafruit Boarduino
// DRO reader for interface with WinDRO by Rolf Strand
//
//  Parts Used:
//  GS30.0200 Linear Scales

#define F_CPU 16000000UL 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include "serial.h"

// Bit manipulation macros
#define sbi(a, b) ((a) |= 1 << (b))       //sets bit B in variable A
#define cbi(a, b) ((a) &= ~(1 << (b)))    //clears bit B in variable A
#define tbi(a, b) ((a) ^= 1 << (b))       //toggles bit B in variable A

#define BAUD 9600
#define UBR (F_CPU / 16 / BAUD - 1)  //Asynchronous Normal Mode

volatile uint32_t xcount=0,ycount=0,zcount=0;
uint32_t temp_x_A=0,temp_x_B=0;
uint32_t temp_y_A=0,temp_y_B=0;
uint32_t temp_z_A=0,temp_z_B=0;
uint32_t old_temp_x_B=0,old_temp_y_B=0,old_temp_z_B=0;
uint32_t old_y_A=0,old_y_B=0,old_z_A=0,old_z_B=0;

//  Digital2   PORTD2	Analog0  PORTC0          
//  Digital3   PORTD3	Analog1  PORTC1
//  Digital4   PORTD4	Analog2  PORTC2
//  Digital5   PORTD5	Analog3	 PORTC3
//  Digital6   PORTD6	Analog4	 PORTC4
//  Digital7   PORTD7	Analog5	 PORTC5
//  Digital8   PORTB0		
//  Digital9   PORTB1  	
//  Digital10  PORTB2


void initialize(void)
{
	DDRB |= (1<<PB5); //LED ouput pin
	DDRD &= ~(1<<PD5) & ~(1<<PD6); //Inputs
	DDRC &= ~(1<<PC4) & ~(1<<PC5) & ~(1<<PC3) & ~(1<<PC2); //Inputs
	//DDRB &= ~(1<<PB1) & ~(1<<PB2); //Inputs

	PCICR |= (1<<PCIE2) | (1<<PCIE1) | (1<<PCIE0); //Enable pin change interrupts pg73

	PCMSK2 |= (1<<PCINT21) | (1<<PCINT22); //Listen D5 & D6
	PCMSK1 |= (1<<PCINT12) | (1<<PCINT13); //Listen C4 & C5
	PCMSK1 |= (1<<PCINT10) | (1<<PCINT11); //Listen C2 & C3

  	serial_init();
}
 
void LED_on(void){
	PORTB |= (1<<PB5);
}
void LED_off(void){
	PORTB &= ~(1<<PB5);
}
void blink_LED(int n){
	int i;
	for(i=0;i<n;i++){
		LED_on();
		_delay_ms(500);
		LED_off();
		_delay_ms(500);
	}
}

int main (void)
{
	int data;
	initialize();

	while(1){
		dprintf("%ld;%ld;%ld\r\n",xcount,ycount,zcount);
		_delay_ms(100);		

		do {
			data = getData();
			switch(data) {
				case 'x':
					cli();
					xcount = 0;
					sei();
					break;
				case 'y':
					cli();
					ycount = 0;
					sei();
					break;
				case 'z':
					cli();
					zcount = 0;
					sei();
					break;
			}
		} while (data != -1);

	}
}

ISR(PCINT2_vect){
	temp_x_A = PIND & (1<<5);
	temp_x_B = PIND & (1<<6);
	temp_x_A >>= 5;	
	temp_x_B >>= 6;		

	if(temp_x_A && old_temp_x_B){
		xcount++;
	}
	else if(temp_x_A && ~old_temp_x_B ){
		xcount--;
	}
	else if(~temp_x_A && old_temp_x_B ){
		xcount--;
	}
	else if(~temp_x_A && ~old_temp_x_B ){
		xcount++;
	}
	else
		xcount=777;
	old_temp_x_B=temp_x_B;
}
ISR(PCINT1_vect){
	temp_y_A = PINC & (1<<4);
	temp_y_B = PINC & (1<<5);
	temp_y_A >>= 4;	
	temp_y_B >>= 5;		

	temp_z_A = PINC & (1<<2);
	temp_z_B = PINC & (1<<3);
	temp_z_A >>= 2;	
	temp_z_B >>= 3;	
	
	if ((temp_y_A != old_y_A) || (temp_y_B != old_y_B)){
		if(temp_y_A && old_y_B){
			ycount++;
		}
		else if(temp_y_A && ~old_y_B ){
			ycount--;
		}
		else if(~temp_y_A && old_y_B ){
			ycount--;
		}
		else if(~temp_y_A && ~old_y_B ){
			ycount++;
		}
		else
			ycount=777;
	}
	else{
		if(temp_z_A && old_z_B){
			zcount++;
		}
		else if(temp_z_A && ~old_z_B ){
			zcount--;
		}
		else if(~temp_z_A && old_z_B ){
			zcount--;
		}
		else if(~temp_z_A && ~old_z_B ){
			zcount++;
		}
		else
			zcount=0x555;
	}	
	old_y_A=temp_y_A;
	old_y_B=temp_y_B;
	old_z_A=temp_z_A;
	old_z_B=temp_z_B;
}
/*ISR(PCINT0_vect){
	temp_z_A = PINB & (1>>1);
	temp_z_B = PINB & (1>>2);
	temp_z_A >>= 1;	
	temp_z_B >>= 2;		

	if(temp_z_A && old_temp_z_B){
		zcount++;
	}
	else if(temp_y_A && ~old_temp_z_B ){
		zcount--;
	}
	else if(~temp_y_A && old_temp_z_B ){
		zcount--;
	}
	else if(~temp_y_A && ~old_temp_z_B ){
		zcount++;
	}
	else
		zcount=777;
	old_temp_z_B=temp_z_B;
}*/



