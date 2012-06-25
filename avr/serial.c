// Written by Pierre Michael (c)2012
// for Atmega328P
//
#define F_CPU 16000000UL 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <avr/pgmspace.h>
#include "serial.h"

#define BAUD 9600
#define UBBR (F_CPU / 16 / BAUD - 1)  //Asynchronous Normal Mode

#define TX_BUFF_SHIFT 7
#define RX_BUFF_SHIFT 6
#define TX_BUFF_LEN (1 << TX_BUFF_SHIFT)
#define RX_BUFF_LEN (1 << RX_BUFF_SHIFT)

volatile uint8_t tx_buff[TX_BUFF_LEN];
volatile uint8_t tx_head = 0;
volatile uint8_t tx_tail = 0;
volatile uint8_t rx_buff[RX_BUFF_LEN];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

void serial_init(void)
{
	cli();
    //Set baud rate 
    UBRR0H = (unsigned char)(UBBR>>8); 
    UBRR0L = (unsigned char)UBBR; 
    // Enable transmitter 
    UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); //enable Tx,Rx and interrupts
    // Set frame format: 8data, 1stop bit  
    UCSR0C = (3<<UCSZ00)|(0<<USBS0); 
	sei();
}

void pwm_init(void)
{
   TCCR1A |= (1<<WGM12)|(1<<WGM11)|(0<<WGM10); //Fast PWM 10-bit
   TCCR1B |= (0<<CS12)|(0<<CS11)|(1<<CS10);  //prescaler: clk / 1
}

void serial_tx(char *data)
{
	while(*data){
		tx_buff[tx_head++] = *data++;
		tx_head &= (TX_BUFF_LEN - 1);
	}
	if (!(UCSR0B & (1 <<TXCIE0))) {
		UDR0 = tx_buff[tx_tail++];
		tx_tail &= (TX_BUFF_LEN - 1);
		UCSR0B |= (1<<TXCIE0);
	}
}
#define MAX 128 
void dprintf(const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    char buffer[MAX];
    //char *ptr = buffer;
    vsnprintf(buffer, MAX, fmt, va);
    va_end (va);
    serial_tx(buffer);
}

int getData(void) {
	if (rx_head == rx_tail) {
		return -1;
	}
	else {
		int ret = rx_buff[rx_head];
		rx_head++;
		rx_head &= (RX_BUFF_LEN - 1);
		return ret;
	}
}

int x_zero(void) {
        int8_t i;
	for(i = rx_head; i != rx_tail; i++, i &= (RX_BUFF_LEN-1)){
		if (rx_buff[i] == 'x') {
			return 1;
		}
	}
	return 0;
}

int y_zero(void) {
        int8_t i;
	for(i = rx_head; i != rx_tail; i++, i &= (RX_BUFF_LEN-1)){
		if (rx_buff[i] == 'y') {
			return 1;
		}
	}
	return 0;
}
int z_zero(void) {
        int8_t i;
	for(i = rx_head; i != rx_tail; i++, i &= (RX_BUFF_LEN-1)){
		if (rx_buff[i] == 'z') {
			return 1;
		}
	}
	return 0;
}
int return_present(void) {
        int8_t i;
	for(i = rx_head; i != rx_tail; i++, i &= (RX_BUFF_LEN-1)){
		if (rx_buff[i] == '\r') {
			return 1;
		}
	}
	return 0;
}

void serial_rx(char *dst) {
	char tmp;
	
	while (rx_head != rx_tail) {
		tmp = rx_buff[rx_head];
		rx_buff[rx_head++] = 0;
		rx_head &= (RX_BUFF_LEN - 1);
		if (tmp == '\r') {
			*dst = 0;
			return;
		}
		*dst++ = tmp;
	}
}

ISR(USART_TX_vect) {
	// do as little as possible here
	if (tx_head == tx_tail) {
		//DISABLE_TX_INTERRUPTS
		UCSR0B &= ~(1<<TXCIE0);
	}
	else {
		UDR0 = tx_buff[tx_tail++];
		tx_tail &= (TX_BUFF_LEN - 1);
	}
}

ISR(USART_RX_vect) {
	// do as little as possible here
	rx_buff[rx_tail++] = UDR0;
	rx_tail &= (RX_BUFF_LEN - 1);
}
