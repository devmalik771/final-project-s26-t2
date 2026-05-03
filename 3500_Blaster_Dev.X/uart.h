#ifndef UART_H
#define UART_H

#include <stdint.h>

void UART_init(uint16_t prescale);

void UART_send(uint8_t data);

void UART_putstring(const char *string_ptr);

void UART_putu8(uint8_t value);

#endif
