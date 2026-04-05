#include <Arduino.h>
#include <SPI.h>
#include <time.h>
#include "Display.h"
#include "Config.h"
#include "WeatherGraphics.h"

// CS=GND, MOSI=13, SCK=14, SPI=MODE3

int16_t pngX = 0, pngY = 0;

inline void sendCmd(uint8_t cmd) {
  digitalWrite(PIN_DC, LOW);
  SPI.transfer(cmd);
}
inline void sendData(uint8_t data) {
  digitalWrite(PIN_DC, HIGH);
  SPI.transfer(data);
}
void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  sendCmd(0x2A);
  sendData(x0 >> 8); sendData(x0 & 0xFF);
  sendData(x1 >> 8); sendData(x1 & 0xFF);
  sendCmd(0x2B);
  sendData(y0 >> 8); sendData(y0 & 0xFF);
  sendData(y1 >> 8); sendData(y1 & 0xFF);
  sendCmd(0x2C);
}
void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  if (x >= 240 || y >= 240) return;
  if (x + w > 240) w = 240 - x;
  if (y + h > 240) h = 240 - y;
  setWindow(x, y, x + w - 1, y + h - 1);
  digitalWrite(PIN_DC, HIGH);
  uint8_t hi = color >> 8, lo = color & 0xFF;
  uint32_t pixels = (uint32_t)w * h;
  for (uint32_t i = 0; i < pixels; i++) {
    SPI.transfer(hi); SPI.transfer(lo);
    if (i % 240 == 0) yield();
  }
}
void fillScreen(uint16_t color) { fillRect(0, 0, 240, 240, color); }
void drawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color) { fillRect(x, y, len, 1, color); }
void drawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color) { fillRect(x, y, 1, len, color); }
void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  drawHLine(x, y, w, color); drawHLine(x, y + h - 1, w, color);
  drawVLine(x, y, h, color); drawVLine(x + w - 1, y, h, color);
}
void drawPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= 240 || y >= 240) return;
  setWindow(x, y, x, y);
  sendData(color >> 8); sendData(color & 0xFF);
}

void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  while (x < y) {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++; ddF_x += 2; f += ddF_x;
    if (cornername & 0x4) { drawPixel(x0+x, y0+y, color); drawPixel(x0+y, y0+x, color); }
    if (cornername & 0x2) { drawPixel(x0+x, y0-y, color); drawPixel(x0+y, y0-x, color); }
    if (cornername & 0x8) { drawPixel(x0-y, y0+x, color); drawPixel(x0-x, y0+y, color); }
    if (cornername & 0x1) { drawPixel(x0-y, y0-x, color); drawPixel(x0-x, y0-y, color); }
  }
}
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) {
  int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  while (x < y) {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++; ddF_x += 2; f += ddF_x;
    if (cornername & 0x1) { drawVLine(x0+x, y0-y, 2*y+1+delta, color); drawVLine(x0+y, y0-x, 2*x+1+delta, color); }
    if (cornername & 0x2) { drawVLine(x0-x, y0-y, 2*y+1+delta, color); drawVLine(x0-y, y0-x, 2*x+1+delta, color); }
  }
}
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  drawVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}
void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  drawHLine(x+r, y, w-2*r, color); drawHLine(x+r, y+h-1, w-2*r, color);
  drawVLine(x, y+r, h-2*r, color); drawVLine(x+w-1, y+r, h-2*r, color);
  drawCircleHelper(x+r, y+r, r, 1, color); drawCircleHelper(x+w-r-1, y+r, r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color); drawCircleHelper(x+r, y+h-r-1, r, 8, color);
}
void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  fillRect(x+r, y, w-2*r, h, color);
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r,     y+r, r, 2, h-2*r-1, color);
}
void drawSmoothDigit(int x, int y, int digit, uint16_t color, uint16_t bg) {
  uint8_t segs[] = { 0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111 };
  if(digit < 0 || digit > 9) return;
  uint8_t d = segs[digit];
  int t = 8;
  int w = 34;
  int h = 34;
  fillRoundRect(x+2, y, w-4, t, t/2, (d & 1) ? color : bg);
  fillRoundRect(x+w-t, y+2, t, h-4, t/2, (d & 2) ? color : bg);
  fillRoundRect(x+w-t, y+h+2, t, h-4, t/2, (d & 4) ? color : bg);
  fillRoundRect(x+2, y+2*h-t, w-4, t, t/2, (d & 8) ? color : bg);
  fillRoundRect(x, y+h+2, t, h-4, t/2, (d & 16) ? color : bg);
  fillRoundRect(x, y+2, t, h-4, t/2, (d & 32) ? color : bg);
  fillRoundRect(x+2, y+h-t/2, w-4, t, t/2, (d & 64) ? color : bg);
}

static const uint8_t FONT5X7[][5] PROGMEM = {
  {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14}, 
  {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, 
  {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08}, 
  {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02}, 
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, 
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, 
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00}, 
  {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, 
  {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, 
  {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, 
  {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, 
  {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, 
  {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, 
  {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, 
  {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00}, 
  {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40}, 
  {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20}, 
  {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E}, 
  {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00}, 
  {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38}, 
  {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20}, 
  {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x40,0x7C}, {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C}, 
  {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00}, 
  {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08}
};

void drawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
  if (c < 32 || c > 126) c = '?';
  const uint8_t* glyph = FONT5X7[c - 32];
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t line = pgm_read_byte(&glyph[col]);
    for (uint8_t row = 0; row < 7; row++) {
      uint16_t color = (line & (1 << row)) ? fg : bg;
      fillRect(x + col * scale, y + row * scale, scale, scale, color);
    }
  }
  fillRect(x + 5 * scale, y, scale, 7 * scale, bg);
}

void drawString(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale) {
  uint16_t cx = x;
  uint8_t  charW = (5 + 1) * scale;
  while (*str) {
    if (cx + charW > 240) { cx = x; y += 8 * scale; }
    drawChar(cx, y, *str++, fg, bg, scale);
    cx += charW;
  }
}

void drawStringMid(uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale) {
  int len = strlen(str);
  int w = len * ((5 + 1) * scale);
  int x = (240 - w) / 2;
  drawString(x, y, str, fg, bg, scale);
}

void initDisplay() {
  digitalWrite(PIN_RST, HIGH); delay(10);
  digitalWrite(PIN_RST, LOW);  delay(20);
  digitalWrite(PIN_RST, HIGH); delay(150);
  sendCmd(0x01); delay(150);
  sendCmd(0x11); delay(120);
  sendCmd(0x3A); sendData(0x55);
  delay(10);
  sendCmd(0x36); sendData(0x00);
  sendCmd(0x21); delay(10);
  sendCmd(0x29); delay(50);
}

// ── RAW RGB565 DECODER ──
void drawWeatherRAW(const uint8_t *data, uint32_t size, int16_t x, int16_t y) {
  setWindow(x, y, x + 39, y + 39);
  digitalWrite(PIN_DC, HIGH);
  for (uint32_t i = 0; i < size; i++) {
    SPI.transfer(pgm_read_byte(&data[i]));
  }
}

String padZero(int v) { return v < 10 ? "0" + String(v) : String(v); }

// ── OPTIMIZED UI LOOP ──
void drawUI() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  int currentMin = p_tm->tm_min;
  int currentSec = p_tm->tm_sec;
  int currentDay = p_tm->tm_mday;

  // 1. LOCATION (Only updates if physically changed)
  if (lastLocation != location) {
    lastLocation = location;
    drawString(10, 10, "                 ", locColor, COLOR_BLACK, 2); 
    drawString(10, 10, location.c_str(), locColor, COLOR_BLACK, 2);
  }
  
  // 2. WEATHER DESC & IMG (Only changes if state changes)
  if (lastWeatherDesc != weatherDesc) {
    lastWeatherDesc = weatherDesc;
    
    // Wipe exact string area gracefully
    drawString(10, 30, "                 ", COLOR_WHITE, COLOR_BLACK, 2);
    if (locationInvalid) {
      drawString(10, 30, "Check Location", COLOR_RED, COLOR_BLACK, 2);
    } else {
       drawString(10, 30, weatherDesc.substring(0, 15).c_str(), COLOR_WHITE, COLOR_BLACK, 2);
    }

    // Weather Box Label
    fillRoundRect(172, 55, 65, 28, 4, COLOR_WHITE);
    drawString(178, 62, weatherDesc.substring(0, 5).c_str(), COLOR_BLACK, COLOR_WHITE, 2);

    // Weather Icon processing (PROGMEM)
    String dd = weatherDesc;
    dd.toLowerCase();
    fillRect(180, 5, 55, 48, COLOR_BLACK); // clear old image

    if(dd.indexOf("clear") >= 0 && dd.indexOf("night") >= 0) drawWeatherRAW(image_clear_night, len_clear_night, 185, 5);
    else if(dd.indexOf("clear") >= 0) drawWeatherRAW(image_clear_day, len_clear_day, 185, 5);
    else if(dd.indexOf("cloud") >= 0 && dd.indexOf("night") >= 0) drawWeatherRAW(image_cloudy_night, len_cloudy_night, 185, 5);
    else if(dd.indexOf("cloud") >= 0) drawWeatherRAW(image_cloudy_day, len_cloudy_day, 185, 5);
    else if(dd.indexOf("wind") >= 0) drawWeatherRAW(image_weather_windy, len_weather_windy, 185, 5);
    else if(dd.indexOf("rain") >= 0) drawWeatherRAW(image_rainy, len_rainy, 185, 5);
    else drawWeatherRAW(image_cloudy_day, len_cloudy_day, 185, 5); // Default completely safely
  }

  // 3. DATE
  if (lastDay != currentDay) {
     lastDay = currentDay;
     char dateStr[20];
     sprintf(dateStr, "%02d/%02d/%d", p_tm->tm_mday, p_tm->tm_mon + 1, p_tm->tm_year + 1900);
     const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
     drawString(10, 140, "                   ", dateColor, COLOR_BLACK, 2); // blank
     drawString(10, 140, dateStr, dateColor, COLOR_BLACK, 2);
     drawString(150, 140, days[p_tm->tm_wday], dateColor, COLOR_BLACK, 2); 
  }

  // 4. CLOCK 
  if (lastMin != currentMin) {
    lastMin = currentMin;
    
    int hr = p_tm->tm_hour;
    if (!timeFormat24) {
      if (hr > 12) hr -= 12;
      if (hr == 0) hr = 12;
    }
    
    int sx = 8;
    int sy = 60;
    
    fillRect(8, 60, 140, 68, COLOR_BLACK); 
    drawSmoothDigit(sx, sy, hr / 10, hourColor, COLOR_BLACK);
    drawSmoothDigit(sx + 38, sy, hr % 10, hourColor, COLOR_BLACK);
    
    // The colon separator can match the hour color for aesthetics
    fillCircle(sx + 78, sy + 20, 6, hourColor);
    fillCircle(sx + 78, sy + 45, 6, hourColor);

    drawSmoothDigit(sx + 88, sy, currentMin / 10, minColor, COLOR_BLACK);
    drawSmoothDigit(sx + 126, sy, currentMin % 10, minColor, COLOR_BLACK);
  }

  // 5. SECONDS
  if (lastSec != currentSec) {
    lastSec = currentSec;
    drawString(175, 95, padZero(currentSec).c_str(), COLOR_WHITE, COLOR_BLACK, 3);
    
    // Smooth IP Setting Indicator Toggle (2 sec bounds)
    fillRect(10, 160, 230, 16, COLOR_BLACK); 
    if (currentSec % 4 < 2) {
        drawString(10, 160, "Setting Page at:", COLOR_GREEN, COLOR_BLACK, 2);
    } else {
        drawString(10, 160, WiFi.localIP().toString().c_str(), COLOR_WHITE, COLOR_BLACK, 2);
    }
  }

  // 6. PROGRESS BARS
  if (lastTemp != currentTemp) {
    lastTemp = currentTemp;
    fillRect(10, 180, 165, 20, COLOR_BLACK);
    
    int tW = map(currentTemp, -10, 45, 0, 90);
    tW = constrain(tW, 0, 90);
    drawString(10, 180, (String((int)currentTemp) + " C").c_str(), tempBarColor, COLOR_BLACK, 2);
    drawRoundRect(80, 185, 90, 10, 4, COLOR_WHITE);
    if(tW > 0) fillRoundRect(80, 185, tW, 10, 4, tempBarColor);
  }

  if (lastHumidity != currentHumidity) {
    lastHumidity = currentHumidity;
    fillRect(10, 210, 165, 20, COLOR_BLACK);
    
    int hW = map(currentHumidity, 0, 100, 0, 90);
    hW = constrain(hW, 0, 90);
    drawString(10, 210, (String((int)currentHumidity) + " %").c_str(), humBarColor, COLOR_BLACK, 2);
    drawRoundRect(80, 215, 90, 10, 4, COLOR_WHITE);
    if(hW > 0) fillRoundRect(80, 215, hW, 10, 4, humBarColor);
  }
}
