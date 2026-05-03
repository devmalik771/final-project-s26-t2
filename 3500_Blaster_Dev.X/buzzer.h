#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void buzzer_init(void);
void buzzer_on(uint8_t top);
void buzzer_off(void);
void buzzer_beep(uint8_t top, uint16_t ms);

#endif