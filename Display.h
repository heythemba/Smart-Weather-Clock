#pragma once
#include <Arduino.h>

#define PIN_DC   0
#define PIN_RST  2
#define PIN_BL   5
// CS=GND, MOSI=13, SCK=14, SPI=MODE3

void initDisplay();
void fillScreen(uint16_t color);
void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void drawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void drawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void drawPixel(uint16_t x, uint16_t y, uint16_t color);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void drawSmoothDigit(int x, int y, int digit, uint16_t color, uint16_t bg);
void drawString(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale);
void drawStringMid(uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale);

void drawUI();
