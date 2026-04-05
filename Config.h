#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE  0xFD20
#define COLOR_GRAY    0x8410

extern ESP8266WebServer server;

extern String apiKey;
extern String location;
extern bool timeFormat24;

extern uint16_t hourColor;
extern uint16_t minColor;
extern uint16_t dateColor;
extern uint16_t locColor;
extern uint16_t tempBarColor;
extern uint16_t humBarColor;

extern String hourColorHex;
extern String minColorHex;
extern String dateColorHex;
extern String locColorHex;
extern String tempBarColorHex;
extern String humBarColorHex;

extern int luminosity;
extern int lumMode;

extern unsigned long lastWeatherUpdate;
extern int lastMin, lastSec, lastDay;
extern float lastTemp;
extern int lastHumidity;
extern String lastLocation;
extern String lastWeatherDesc;

extern String weatherDesc;
extern float currentTemp;
extern int currentHumidity;
extern bool locationInvalid;

uint16_t hex2RGB(String hex);
void forceUIRefresh();
void loadConfig();
void saveConfig();
