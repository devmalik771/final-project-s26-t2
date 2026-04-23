#include "project_config.h"
#include <util/delay.h>
#include <stdint.h>

#include "uart.h"
#include "imu.h"
#include "button.h"
#include "buzzer.h"
#include "ST7735.h"
#include "LCD_GFX.h"

// ---------------- Settings ----------------
#define BAUD_PRESCALER       103
#define MAIN_LOOP_DELAY_MS   10

#define PLAYER_ID            1
#define MAX_BULLETS          12

#define BUZZER_TONE_SHOT1    150
#define BUZZER_TONE_SHOT2    200
#define BUZZER_TONE_SHOT3    250

#define BUZZER_TONE_RELOAD1  250
#define BUZZER_TONE_RELOAD2  175

#define BUZZER_TONE_EMPTY    255

#define BUZZER_MS_SHOT       100
#define BUZZER_MS_RELOAD     200
#define BUZZER_MS_EMPTY      500
#define IR_PULSE_MS          500
#define START_COUNTDOWN_SECONDS   5
#define RELOAD_COUNTDOWN_SECONDS  3

#define IR_DDR               DDRD
#define IR_PORT              PORTD
#define IR_BIT               PD4

typedef enum {
    GAME_STATE_START_GAME = 0,
    GAME_STATE_ACTIVE,
    GAME_STATE_RELOAD
} game_state_t;

// ---------------- State ----------------
static uint8_t bullets = MAX_BULLETS;
static game_state_t game_state = GAME_STATE_START_GAME;

// ---------------- Helpers ----------------
static void handle_trigger(void);
static void handle_reload(void);
static void play_shot_sound(void);
static void play_reload_sound(void);
static void play_empty_sound(void);
static void pulse_irled(void);
static void uart_send_player_event(const char *event_name);
static void run_start_game_countdown(void);
static void run_reload_countdown(void);
static void irled_init(void);
static void irled_set(uint8_t enabled);

int main(void)
{
    UART_init(BAUD_PRESCALER);
    imu_init();
    button_init();
    buzzer_init();
    irled_init();

    lcd_init();
    LCD_rotate(3);

    run_start_game_countdown();
    game_state = GAME_STATE_ACTIVE;
    LCD_drawAmmo(bullets);
    uart_send_player_event("AMMO");

    while (1) {
        if (game_state != GAME_STATE_ACTIVE) {
            irled_set(0);
            _delay_ms(MAIN_LOOP_DELAY_MS);
            continue;
        }

        button_update();

        if (button_pressed()) {
            handle_trigger();
        }

        if (imu_shaken()) {
            handle_reload();
        }

        _delay_ms(MAIN_LOOP_DELAY_MS);
    }
}

static void handle_trigger(void)
{
    if (bullets > 0) {
        bullets--;
        uart_send_player_event("AMMO");

        play_shot_sound();
        LCD_drawAmmo(bullets);
        pulse_irled();
    } else {
        uart_send_player_event("EMPTY");

        play_empty_sound();
        LCD_drawEmpty();
        irled_set(0);
    }
}

static void handle_reload(void)
{
    if (bullets < MAX_BULLETS) {
        irled_set(0);
        game_state = GAME_STATE_RELOAD;
        run_reload_countdown();
        bullets = MAX_BULLETS;
        uart_send_player_event("RELOAD");
        play_reload_sound();
        LCD_drawAmmo(bullets);
        game_state = GAME_STATE_ACTIVE;
    }
}

static void play_shot_sound(void)
{
    buzzer_beep(BUZZER_TONE_SHOT1, BUZZER_MS_SHOT);
    buzzer_beep(BUZZER_TONE_SHOT2, BUZZER_MS_SHOT);
    buzzer_beep(BUZZER_TONE_SHOT3, BUZZER_MS_SHOT);
}

static void play_reload_sound(void)
{
    buzzer_beep(BUZZER_TONE_RELOAD1, BUZZER_MS_RELOAD);
    buzzer_beep(BUZZER_TONE_RELOAD2, BUZZER_MS_RELOAD);
    buzzer_beep(BUZZER_TONE_RELOAD1, BUZZER_MS_RELOAD);
    buzzer_beep(BUZZER_TONE_RELOAD2, BUZZER_MS_RELOAD);
}

static void play_empty_sound(void)
{
    buzzer_beep(BUZZER_TONE_EMPTY, BUZZER_MS_EMPTY);
    _delay_ms(100);
    buzzer_beep(BUZZER_TONE_EMPTY, BUZZER_MS_EMPTY);
}

static void pulse_irled(void)
{
    irled_set(1);
    _delay_ms(IR_PULSE_MS);
    irled_set(0);
}

static void uart_send_player_event(const char *event_name)
{
    UART_putstring("P");
    UART_putu8(PLAYER_ID);
    UART_putstring(",");
    UART_putstring(event_name);
    UART_putstring(",");
    UART_putu8(bullets);
    UART_putstring("\r\n");
}

static void irled_init(void)
{
    IR_DDR |= (1 << IR_BIT);
    IR_PORT &= ~(1 << IR_BIT);
}

static void irled_set(uint8_t enabled)
{
    if (enabled) {
        IR_PORT |= (1 << IR_BIT);
    } else {
        IR_PORT &= ~(1 << IR_BIT);
    }
}

static void run_reload_countdown(void)
{
    uint8_t second;

    for (second = RELOAD_COUNTDOWN_SECONDS; second > 0; second--) {
        LCD_drawReloadCountdown(second);
        play_reload_sound();
        _delay_ms(1000);
    }
}

static void run_start_game_countdown(void)
{
    uint8_t second;

    for (second = START_COUNTDOWN_SECONDS; second > 0; second--) {
        LCD_drawStartCountdown(second);
        _delay_ms(1000);
    }
}
