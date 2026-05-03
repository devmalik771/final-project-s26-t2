#include "LCD_GFX.h"
#include "ST7735.h"
#include "ASCII_LUT.h"

static void LCD_fillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);
static void LCD_drawCharScaled(uint8_t x, uint8_t y, uint8_t character, uint8_t scale, uint16_t fColor, uint16_t bColor);
static void LCD_drawStringScaled(uint8_t x, uint8_t y, const char *str, uint8_t scale, uint16_t fg, uint16_t bg);
static uint8_t LCD_textWidth(const char *str, uint8_t scale);
static void LCD_drawCenteredString(uint8_t y, const char *str, uint8_t scale, uint16_t fg, uint16_t bg);
static void LCD_formatAmmoText(char *buffer, uint8_t bullets);
static void LCD_drawHudHeader(uint8_t player);
static void LCD_drawHudDecor(void);
static void LCD_drawStatusPanel(const char *status, uint16_t statusColor);
static void LCD_drawCountdownScreen(const char *title, uint8_t seconds, uint16_t titleColor, const char *footer, uint16_t footerColor);
static void LCD_formatCountdownText(char *buffer, uint8_t seconds);

void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
{
    LCD_setAddr(x, y, x, y);
    SPI_ControllerTx_16bit(color);
}

void LCD_drawChar(uint8_t x, uint8_t y, uint8_t character, uint16_t fColor, uint16_t bColor)
{
    uint8_t row;
    uint8_t i, j;

    if (character < 0x20 || character > 0x7F) {
        return;
    }

    if (x > (LCD_WIDTH - 6U) || y > (LCD_HEIGHT - 8U)) {
        return;
    }

    row = character - 0x20;
    LCD_setAddr(x, y, x + 5U, y + 7U);
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    for (j = 0; j < 8U; j++) {
        for (i = 0; i < 5U; i++) {
            uint8_t pixels = ASCII[row][i];
            if (((pixels >> j) & 1U) != 0U) {
                SPI_ControllerTx_16bit_stream(fColor);
            } else {
                SPI_ControllerTx_16bit_stream(bColor);
            }
        }

        // Add one blank column for character spacing.
        SPI_ControllerTx_16bit_stream(bColor);
    }

    set(LCD_PORT, LCD_TFT_CS);
}

void LCD_setScreen(uint16_t color)
{
    uint32_t i;

    LCD_setAddr(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    for (i = 0; i < (uint32_t)LCD_WIDTH * (uint32_t)LCD_HEIGHT; i++) {
        SPI_ControllerTx_16bit_stream(color);
    }

    set(LCD_PORT, LCD_TFT_CS);
}

void LCD_drawString(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg)
{
    while (*str != '\0') {
        LCD_drawChar(x, y, (uint8_t)*str, fg, bg);
        x += 6;
        str++;
    }
}

void LCD_drawAmmo(uint8_t bullets)
{
    char ammoText[10];

    LCD_formatAmmoText(ammoText, bullets);
    LCD_setScreen(BLACK);
    LCD_drawHudHeader(1);
    LCD_drawHudDecor();
    LCD_drawCenteredString(42, ammoText, 2, CYAN, BLACK);

    if (bullets == 0) {
        LCD_drawCenteredString(86, "STATUS: EMPTY", 1, RED, BLACK);
        LCD_drawCenteredString(100, "SHAKE TO RELOAD", 1, YELLOW, BLACK);
    } else {
        LCD_drawCenteredString(86, "STATUS: READY", 1, GREEN, BLACK);
    }
}

void LCD_drawReload(void)
{
    LCD_setScreen(BLACK);
    LCD_drawHudHeader(1);
    LCD_drawHudDecor();
    LCD_drawStatusPanel("RELOAD", YELLOW);
    LCD_drawCenteredString(100, "AMMO REFILLED", 1, GREEN, BLACK);
}

void LCD_drawEmpty(void)
{
    LCD_setScreen(BLACK);
    LCD_drawHudHeader(1);
    LCD_drawHudDecor();
    LCD_drawStatusPanel("EMPTY", RED);
    LCD_drawCenteredString(100, "SHAKE TO RELOAD", 1, YELLOW, BLACK);
}

void LCD_drawStartCountdown(uint8_t seconds)
{
    LCD_drawCountdownScreen("START", seconds, GREEN, "GET READY", CYAN);
}

void LCD_drawReloadCountdown(uint8_t seconds)
{
    LCD_drawCountdownScreen("RELOAD", seconds, YELLOW, "REFILLING AMMO", GREEN);
}

static void LCD_fillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color)
{
    uint16_t count;

    if (width == 0U || height == 0U) {
        return;
    }

    LCD_setAddr(x, y, x + width - 1U, y + height - 1U);
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    count = (uint16_t)width * (uint16_t)height;
    while (count-- > 0U) {
        SPI_ControllerTx_16bit_stream(color);
    }

    set(LCD_PORT, LCD_TFT_CS);
}

static void LCD_drawCharScaled(uint8_t x, uint8_t y, uint8_t character, uint8_t scale, uint16_t fColor, uint16_t bColor)
{
    uint8_t row;
    uint8_t i, j;
    uint8_t sx, sy;

    if (scale == 0U || character < 0x20 || character > 0x7F) {
        return;
    }

    if (x > (LCD_WIDTH - (6U * scale)) || y > (LCD_HEIGHT - (8U * scale))) {
        return;
    }

    row = character - 0x20;
    LCD_setAddr(x, y, x + (6U * scale) - 1U, y + (8U * scale) - 1U);
    clear(LCD_PORT, LCD_TFT_CS);
    set(LCD_PORT, LCD_DC);

    for (j = 0; j < 8U; j++) {
        for (sy = 0; sy < scale; sy++) {
            for (i = 0; i < 5U; i++) {
                uint16_t color = (((ASCII[row][i] >> j) & 1U) != 0U) ? fColor : bColor;
                for (sx = 0; sx < scale; sx++) {
                    SPI_ControllerTx_16bit_stream(color);
                }
            }

            for (sx = 0; sx < scale; sx++) {
                SPI_ControllerTx_16bit_stream(bColor);
            }
        }
    }

    set(LCD_PORT, LCD_TFT_CS);
}

static void LCD_drawStringScaled(uint8_t x, uint8_t y, const char *str, uint8_t scale, uint16_t fg, uint16_t bg)
{
    while (*str != '\0') {
        LCD_drawCharScaled(x, y, (uint8_t)*str, scale, fg, bg);
        x += (uint8_t)(6U * scale);
        str++;
    }
}

static uint8_t LCD_textWidth(const char *str, uint8_t scale)
{
    uint8_t length = 0;

    while (*str != '\0') {
        length++;
        str++;
    }

    return (uint8_t)(length * 6U * scale);
}

static void LCD_drawCenteredString(uint8_t y, const char *str, uint8_t scale, uint16_t fg, uint16_t bg)
{
    uint8_t x = 0;
    uint8_t width = LCD_textWidth(str, scale);

    if (width < LCD_WIDTH) {
        x = (uint8_t)((LCD_WIDTH - width) / 2U);
    }

    LCD_drawStringScaled(x, y, str, scale, fg, bg);
}

static void LCD_formatAmmoText(char *buffer, uint8_t bullets)
{
    buffer[0] = 'A';
    buffer[1] = 'm';
    buffer[2] = 'm';
    buffer[3] = 'o';
    buffer[4] = ':';
    buffer[5] = ' ';
    buffer[6] = (char)('0' + ((bullets / 100U) % 10U));
    buffer[7] = (char)('0' + ((bullets / 10U) % 10U));
    buffer[8] = (char)('0' + (bullets % 10U));
    buffer[9] = '\0';
}

static void LCD_drawHudHeader(uint8_t player)
{
    char playerText[] = "Player: 0";

    playerText[8] = (char)('0' + player);
    LCD_fillRect(0, 0, LCD_WIDTH, 14, BLUE);
    LCD_drawCenteredString(2, playerText, 1, WHITE, BLUE);
}

static void LCD_drawHudDecor(void)
{
    LCD_fillRect(8, 20, 24, 2, CYAN);
    LCD_fillRect(14, 26, 12, 2, CYAN);
    LCD_fillRect(128, 20, 24, 2, CYAN);
    LCD_fillRect(134, 26, 12, 2, CYAN);

    LCD_fillRect(18, 112, 124, 2, MAGENTA);
    LCD_fillRect(28, 118, 104, 2, MAGENTA);

    LCD_fillRect(72, 18, 16, 2, YELLOW);
    LCD_fillRect(72, 108, 16, 2, YELLOW);
    LCD_fillRect(66, 24, 2, 12, YELLOW);
    LCD_fillRect(92, 24, 2, 12, YELLOW);
}

static void LCD_drawStatusPanel(const char *status, uint16_t statusColor)
{
    LCD_fillRect(42, 42, 76, 2, CYAN);
    LCD_fillRect(42, 82, 76, 2, CYAN);
    LCD_fillRect(42, 42, 2, 42, CYAN);
    LCD_fillRect(116, 42, 2, 42, CYAN);
    LCD_drawCenteredString(54, status, 2, statusColor, BLACK);
}

static void LCD_drawCountdownScreen(const char *title, uint8_t seconds, uint16_t titleColor, const char *footer, uint16_t footerColor)
{
    char countdownText[2];

    LCD_formatCountdownText(countdownText, seconds);
    LCD_setScreen(BLACK);
    LCD_drawHudHeader(1);
    LCD_drawHudDecor();
    LCD_drawCenteredString(26, title, 2, titleColor, BLACK);
    LCD_drawStatusPanel(countdownText, WHITE);
    LCD_drawCenteredString(100, footer, 1, footerColor, BLACK);
}

static void LCD_formatCountdownText(char *buffer, uint8_t seconds)
{
    buffer[0] = (char)('0' + (seconds % 10U));
    buffer[1] = '\0';
}
