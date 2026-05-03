#ifndef IMU_H
#define IMU_H

#include <stdint.h>

#define SHAKE_SUM_THRESHOLD    35000
#define SHAKE_AXIS_THRESHOLD   9000
#define SHAKE_HOLD_CYCLES      1

void imu_init(void);
uint8_t imu_shaken(void);

#endif
