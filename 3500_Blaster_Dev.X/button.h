#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

void button_init(void);
void button_update(void);
uint8_t button_pressed(void);
uint8_t button_is_down(void);

#endif
