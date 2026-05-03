#ifndef IR_RX_H
#define IR_RX_H

#include <stdint.h>

void ir_rx_init(void);
uint8_t ir_rx_signal_present(void);

#endif
