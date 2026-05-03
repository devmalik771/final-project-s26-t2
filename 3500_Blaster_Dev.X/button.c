#include "button.h"
#include <avr/io.h>

#define BTN_DDR   DDRD
#define BTN_PORT  PORTD
#define BTN_PINR  PIND
#define BTN_BIT   PD2

#define DEBOUNCE_MAX 5

static uint8_t stable_state = 1;
static uint8_t last_raw = 1;
static uint8_t count = 0;
static uint8_t pressed_event = 0;

void button_init(void)
{
    BTN_DDR &= ~(1 << BTN_BIT);
    BTN_PORT |= (1 << BTN_BIT);   // internal pull-up
}

void button_update(void)
{
    uint8_t raw = (BTN_PINR & (1 << BTN_BIT)) ? 1 : 0;

    if (raw == last_raw) {
        if (count < DEBOUNCE_MAX) count++;
    } else {
        count = 0;
    }

    if (count >= DEBOUNCE_MAX && raw != stable_state) {
        stable_state = raw;
        if (stable_state == 0) pressed_event = 1;   // active low press
    }

    last_raw = raw;
}

uint8_t button_pressed(void)
{
    if (pressed_event) {
        pressed_event = 0;
        return 1;
    }
    return 0;
}

uint8_t button_is_down(void)
{
    return (stable_state == 0U) ? 1U : 0U;
}
