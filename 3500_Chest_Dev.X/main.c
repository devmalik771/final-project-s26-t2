#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart.h"

#define PLAYER_NUMBER        2
#define STARTING_LIVES       3
#define HIT_COOLDOWN_MS      1000
#define RESET_DEBOUNCE_MS    50
#define UART_BAUD_PRESCALER  103

/* Reset input pin: PD4 goes HIGH to reset lives (hardware reset). */
#define RESET_DDR            DDRD
#define RESET_PORT           PORTD
#define RESET_PIN_REG        PIND
#define RESET_BIT            DDD4
#define RESET_PORT_BIT       PORTD4
#define RESET_INPUT_BIT      PIND4

static uint8_t lives = STARTING_LIVES;

/* UART receive buffer for incoming commands */
static char rx_buf[16];
static uint8_t rx_idx = 0;

/* ---- Forward declarations ---- */
static void leds_init(void);
static void update_life_leds(uint8_t l);
static void set_hit_indicator(uint8_t on);
static void ir_rx_input_init(void);
static uint8_t ir_rx_signal_detected(void);
static void reset_input_init(void);
static uint8_t reset_signal_present(void);
static void send_event(const char *event);
static void do_reset(void);
static void check_uart_rx(void);
static uint8_t streq(const char *a, const char *b);

/* ---- LEDs ---- */
static void leds_init(void)
{
    /* Green life LEDs: PB0, PB1, PB2 */
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);
    /* Hit indicator LED: PB3 */
    DDRB |= (1 << DDB3);
    /* Red LEDs: PC0, PC1, PC2 */
    DDRC |= (1 << DDC0) | (1 << DDC1) | (1 << DDC2);
    PORTB &= ~(1 << PORTB3);
}

static void update_life_leds(uint8_t l)
{
    PORTB &= ~((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2));
    PORTC &= ~((1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2));

    if (l >= 3) {
        PORTB |= (1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2);
    } else if (l == 2) {
        PORTB |= (1 << PORTB0) | (1 << PORTB1);
        PORTC |= (1 << PORTC2);
    } else if (l == 1) {
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

/* ---- IR Receiver ---- */
static void ir_rx_input_init(void)
{
    DDRD  &= ~(1 << DDD2);
    PORTD &= ~(1 << PORTD2);
}

static uint8_t ir_rx_signal_detected(void)
{
    /* TSOP idles HIGH, goes LOW on detection */
    if ((PIND & (1 << PIND2)) == 0) {
        return 1;
    }
    return 0;
}

/* ---- Hardware reset input ---- */
static void reset_input_init(void)
{
    RESET_DDR  &= ~(1 << RESET_BIT);
    RESET_PORT &= ~(1 << RESET_PORT_BIT);
}

static uint8_t reset_signal_present(void)
{
    if (RESET_PIN_REG & (1 << RESET_INPUT_BIT)) {
        return 1;
    }
    return 0;
}

/* ---- UART send: P2,EVENT,lives ---- */
static void send_event(const char *event)
{
    UART_putstring("P2,");
    UART_putstring(event);
    UART_putstring(",");
    UART_putu8(lives);
    UART_putstring("\r\n");
}

/* ---- Reset handler ---- */
static void do_reset(void)
{
    lives = STARTING_LIVES;
    update_life_leds(lives);
    set_hit_indicator(0);
    send_event("HEALTH");
}

/* ---- UART RX: check for RESET command from Feather ---- */
static uint8_t streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static void check_uart_rx(void)
{
    /* Check if byte available */
    while (UCSR0A & (1 << RXC0)) {
        char c = UDR0;

        if (c == '\n') {
            rx_buf[rx_idx] = '\0';

            if (streq(rx_buf, "RESET")) {
                do_reset();
            }

            rx_idx = 0;
        } else if (c == '\r') {
            /* skip */
        } else {
            if (rx_idx < sizeof(rx_buf) - 1) {
                rx_buf[rx_idx++] = c;
            }
        }
    }
}

/* ---- Main ---- */
int main(void)
{
    leds_init();
    ir_rx_input_init();
    reset_input_init();
    UART_init(UART_BAUD_PRESCALER);

    update_life_leds(lives);
    send_event("HEALTH");

    while (1) {
        /* Check for RESET command from Feather */
        check_uart_rx();

        /* Check hardware reset button */
        if (reset_signal_present()) {
            do_reset();
            _delay_ms(RESET_DEBOUNCE_MS);
            while (reset_signal_present()) {
            }
        }
        /* Check IR hit */
        else if (ir_rx_signal_detected()) {
            set_hit_indicator(1);

            if (lives > 0) {
                lives--;
                update_life_leds(lives);
                send_event("HIT");

                if (lives == 0) {
                    send_event("DEAD");
                }

                /* Only cooldown while alive — keeps main loop responsive
                   for UART RESET commands when eliminated */
                _delay_ms(HIT_COOLDOWN_MS);
            }

            set_hit_indicator(0);
        } else {
            set_hit_indicator(0);
        }
    }
}
