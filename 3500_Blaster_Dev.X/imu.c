#include "project_config.h"
#include "imu.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define IMU_ADDR        0x18
#define REG_CTRL1       0x20
#define REG_CTRL6       0x25
#define REG_OUT_X_L     0x28
#define I2C_TIMEOUT     60000UL

static int16_t px, py, pz;
static uint8_t first_sample = 1;
static uint8_t shake_hold = 0;

static int16_t abs16(int16_t x) {
    return x < 0 ? -x : x;
}

static uint8_t twi_wait(void) {
    uint32_t n = 0;
    while (!(TWCR0 & (1 << TWINT))) {
        if (++n > I2C_TIMEOUT) return 0;
    }
    return 1;
}

static void i2c_init(void) {
    TWSR0 = 0;
    TWBR0 = 72;
}

static uint8_t i2c_start(void) {
    TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    return twi_wait();
}

static void i2c_stop(void) {
    TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    _delay_us(10);
}

static uint8_t i2c_write(uint8_t x) {
    TWDR0 = x;
    TWCR0 = (1 << TWINT) | (1 << TWEN);
    if (!twi_wait()) return 0;

    switch (TWSR0 & 0xF8) {
        case 0x18:
        case 0x28:
        case 0x40:
            return 1;
        default:
            return 0;
    }
}

static uint8_t i2c_read(uint8_t *x, uint8_t ack) {
    TWCR0 = (1 << TWINT) | (1 << TWEN) | (ack ? (1 << TWEA) : 0);
    if (!twi_wait()) return 0;
    *x = TWDR0;
    return 1;
}

static uint8_t imu_write(uint8_t reg, uint8_t val) {
    if (!i2c_start()) return 0;
    if (!i2c_write((IMU_ADDR << 1) | 0)) return i2c_stop(), 0;
    if (!i2c_write(reg)) return i2c_stop(), 0;
    if (!i2c_write(val)) return i2c_stop(), 0;
    i2c_stop();
    return 1;
}

static uint8_t imu_xyz(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t xl, xh, yl, yh, zl, zh;

    if (!i2c_start()) return 0;
    if (!i2c_write((IMU_ADDR << 1) | 0)) return i2c_stop(), 0;
    if (!i2c_write(REG_OUT_X_L)) return i2c_stop(), 0;

    if (!i2c_start()) return 0;
    if (!i2c_write((IMU_ADDR << 1) | 1)) return i2c_stop(), 0;

    if (!i2c_read(&xl, 1)) return i2c_stop(), 0;
    if (!i2c_read(&xh, 1)) return i2c_stop(), 0;
    if (!i2c_read(&yl, 1)) return i2c_stop(), 0;
    if (!i2c_read(&yh, 1)) return i2c_stop(), 0;
    if (!i2c_read(&zl, 1)) return i2c_stop(), 0;
    if (!i2c_read(&zh, 0)) return i2c_stop(), 0;

    i2c_stop();

    *x = (int16_t)((xh << 8) | xl);
    *y = (int16_t)((yh << 8) | yl);
    *z = (int16_t)((zh << 8) | zl);
    return 1;
}

void imu_init(void) {
    i2c_init();
    _delay_ms(100);

    (void)imu_write(REG_CTRL1, 0x44);
    (void)imu_write(REG_CTRL6, 0x00);
}

uint8_t imu_shaken(void) {
    int16_t x, y, z, dx, dy, dz;
    int32_t sum;

    if (!imu_xyz(&x, &y, &z)) return 0;

    if (first_sample) {
        px = x;
        py = y;
        pz = z;
        first_sample = 0;
        return 0;
    }

    dx = x - px;
    dy = y - py;
    dz = z - pz;

    px = x;
    py = y;
    pz = z;

    sum = (int32_t)abs16(dx) + (int32_t)abs16(dy) + (int32_t)abs16(dz);

    if (sum > SHAKE_SUM_THRESHOLD &&
        (abs16(dx) > SHAKE_AXIS_THRESHOLD ||
         abs16(dy) > SHAKE_AXIS_THRESHOLD ||
         abs16(dz) > SHAKE_AXIS_THRESHOLD)) {
        shake_hold = SHAKE_HOLD_CYCLES;
    }

    if (shake_hold) {
        shake_hold--;
        return 1;
    }

    return 0;
}
