#include "ir_rx.h"

#include <avr/io.h>

void ir_rx_init(void)
{
    /* PD2 is the IR receiver input. */
    DDRD &= ~(1 << DDD2);

    /* No pull-up enabled here. Most IR receiver modules drive the line. */
    PORTD &= ~(1 << PORTD2);
}

uint8_t ir_rx_signal_present(void)
{
    /*
     * Common 38 kHz IR receiver modules idle HIGH and pull LOW
     * when they detect IR light.
     */
    if ((PIND & (1 << PIND2)) == 0) {
        return 1;
    }

    return 0;
}
