#include "uart.h"
#include <avr/io.h>

void UART_init(uint16_t baud_prescaler)
{
    UBRR0H = (uint8_t)(baud_prescaler >> 8);
    UBRR0L = (uint8_t)baud_prescaler;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << USBS0) | (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_send(uint8_t data)
{
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = data;
}

void UART_putstring(const char *string_ptr)
{
    while (*string_ptr != '\0') {
        UART_send((uint8_t)*string_ptr);
        string_ptr++;
    }
}

void UART_putu8(uint8_t value)
{
    if (value >= 100U) {
        UART_send((uint8_t)('0' + (value / 100U)));
        value %= 100U;
        UART_send((uint8_t)('0' + (value / 10U)));
        UART_send((uint8_t)('0' + (value % 10U)));
        return;
    }

    if (value >= 10U) {
        UART_send((uint8_t)('0' + (value / 10U)));
        UART_send((uint8_t)('0' + (value % 10U)));
        return;
    }

    UART_send((uint8_t)('0' + value));
}
