// Written by Pierre Michael (c)2012
// for Atmega328P

#ifndef SERIAL_H
#define SERIAL_H

void      serial_init(void);
void 	  pwm_init(void);
void      serial_tx(char *);
void	  serial_rx(char *);
int       return_present(void);
int       x_zero(void);
int       y_zero(void);
int       z_zero(void);
int	      getData(void);

void      dprintf(const char *fmt, ...);

#endif
