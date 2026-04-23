#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart.h"

#define PLAYER_NUMBER 1
#define STARTING_LIVES 3
#define HIT_COOLDOWN_MS 1000
#define RESET_DEBOUNCE_MS 50
#define UART_BAUD_PRESCALER 103

/* Reset input pin: PD4 goes HIGH to reset lives back to 3. */
#define RESET_DDR DDRD
#define RESET_PORT PORTD
#define RESET_PIN_REG PIND
#define RESET_BIT DDD4
#define RESET_PORT_BIT PORTD4
#define RESET_INPUT_BIT PIND4

static void leds_init(void);
static void update_life_leds(uint8_t lives);
static void set_hit_indicator(uint8_t on);
static void ir_rx_input_init(void);
static uint8_t ir_rx_signal_detected(void);
static void reset_input_init(void);
static uint8_t reset_signal_present(void);
static void send_status_packet(uint8_t lives);

static void leds_init(void)
{
    /* Green life LEDs: PB0, PB1, PB2 */
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);

    /* Separate hit indicator LED: PB3 */
    DDRB |= (1 << DDB3);

    /* Red LEDs: PC0, PC1, PC2 */
    DDRC |= (1 << DDC0) | (1 << DDC1) | (1 << DDC2);

    PORTB &= ~(1 << PORTB3);
}

static void reset_input_init(void)
{
    /* PD4 is a reset input from another board or control signal. */
    RESET_DDR &= ~(1 << RESET_BIT);

    /*
     * Leave pull-up off because this input is meant to reset on a HIGH signal.
     * If your source can ever float, add an external pulldown resistor.
     */
    RESET_PORT &= ~(1 << RESET_PORT_BIT);
}

static uint8_t reset_signal_present(void)
{
    if (RESET_PIN_REG & (1 << RESET_INPUT_BIT)) {
        return 1;
    }

    return 0;
}

static void update_life_leds(uint8_t lives)
{
    /* Turn everything off first. */
    PORTB &= ~((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2));
    PORTC &= ~((1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2));

    if (lives >= 3) {
        PORTB |= (1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2);
    } else if (lives == 2) {
        PORTB |= (1 << PORTB0) | (1 << PORTB1);
        PORTC |= (1 << PORTC2);
    } else if (lives == 1) {
        PORTB |= (1 << PORTB0);
        PORTC |= (1 << PORTC1) | (1 << PORTC2);
    } else {
        PORTC |= (1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2);
    }
}

static void set_hit_indicator(uint8_t on)
{
    if (on) {
        PORTB |= (1 << PORTB3);
    } else {
        PORTB &= ~(1 << PORTB3);
    }
}

static void ir_rx_input_init(void)
{
    /* PD2 is the IR receiver input. */
    DDRD &= ~(1 << DDD2);

    /* No pull-up enabled here. Most IR receiver modules drive the line. */
    PORTD &= ~(1 << PORTD2);
}

static uint8_t ir_rx_signal_detected(void)
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

static void send_status_packet(uint8_t lives)
{
    UART_putstring("player");
    UART_putu8(PLAYER_NUMBER);
    UART_putstring(", health, ");
    UART_putu8(lives);
    UART_putstring("\r\n");
}

int main(void)
{
    uint8_t lives = STARTING_LIVES;

    leds_init();
    ir_rx_input_init();
    reset_input_init();
    UART_init(UART_BAUD_PRESCALER);
    update_life_leds(lives);
    send_status_packet(lives);

    while (1) {
        if (reset_signal_present()) {
            lives = STARTING_LIVES;
            update_life_leds(lives);
            set_hit_indicator(0);
            send_status_packet(lives);

            _delay_ms(RESET_DEBOUNCE_MS);

            while (reset_signal_present()) {
            }
        } else if (ir_rx_signal_detected()) {
            set_hit_indicator(1);

            if (lives > 0) {
                lives--;
                update_life_leds(lives);
                send_status_packet(lives);
            }

            /* Cooldown so one hit does not count multiple times. */
            _delay_ms(HIT_COOLDOWN_MS);
            set_hit_indicator(0);
        } else {
            set_hit_indicator(0);
        }
    }
}
