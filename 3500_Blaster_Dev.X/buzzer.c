#include "project_config.h"
#include "buzzer.h"
#include <avr/io.h>
#include <util/delay.h>

// Use PD3 = OC2B for PWM output.
#define BUZ_DDR   DDRD
#define BUZ_PORT  PORTD
#define BUZ_BIT   PD3

void buzzer_init(void)
{
    BUZ_DDR |= (1 << BUZ_BIT);

    // Fast PWM, TOP = OCR2A.
    // Clear OC2B on compare match, set at BOTTOM.
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << WGM22);

    OCR2A = 199;   // default period
    OCR2B = 100;   // ~50% duty
}

void buzzer_on(uint8_t top)
{
    OCR2A = top;
    OCR2B = top / 2;                 // ~50% duty cycle
    TCCR2B = (1 << WGM22) | (1 << CS21);   // prescaler = 8, timer on
}

void buzzer_off(void)
{
    TCCR2B = (1 << WGM22);           // timer stopped
    OCR2B = 0;
    BUZ_PORT &= ~(1 << BUZ_BIT);
}

void buzzer_beep(uint8_t top, uint16_t ms)
{
    buzzer_on(top);
    while (ms--) _delay_ms(1);
    buzzer_off();
}
