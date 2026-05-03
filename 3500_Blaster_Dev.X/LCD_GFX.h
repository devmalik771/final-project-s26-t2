#ifndef LCD_GFX_H_
#define LCD_GFX_H_

#include <stdint.h>

// colors
#define BLACK     0x0000
#define WHITE     0xFFFF
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0

void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color);
void LCD_drawChar(uint8_t x, uint8_t y, uint8_t character, uint16_t fColor, uint16_t bColor);
void LCD_setScreen(uint16_t color);
void LCD_drawString(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg);

void LCD_drawAmmo(uint8_t bullets);
void LCD_drawReload(void);
void LCD_drawEmpty(void);
void LCD_drawStartCountdown(uint8_t seconds);
void LCD_drawReloadCountdown(uint8_t seconds);

#endif
