#include "Config.h"

ESP8266WebServer server(80);

String apiKey = "1c4a37dc119521e8f3024bacaf52a9d1";
String location = "Mahdia";
bool timeFormat24 = true;

uint16_t hourColor = COLOR_ORANGE;
uint16_t minColor = COLOR_CYAN;
uint16_t dateColor = COLOR_WHITE;
uint16_t locColor = COLOR_CYAN;
uint16_t tempBarColor = COLOR_RED;
uint16_t humBarColor = COLOR_GREEN;

String hourColorHex = "#ffaa00";
String minColorHex = "#00ffff";
String dateColorHex = "#ffffff";
String locColorHex = "#00FFFF";
String tempBarColorHex = "#FF0000";
String humBarColorHex = "#00FF00";

int luminosity = 100;
int lumMode = 0; // 0=Off/Manual, 1=Day, 2=Night

unsigned long lastWeatherUpdate = 0;
int lastMin = -1, lastSec = -1, lastDay = -1;
float lastTemp = -999.0;
int lastHumidity = -999;
String lastLocation = "";
String lastWeatherDesc = "";

String weatherDesc = "Loading...";
float currentTemp = 0.0;
int currentHumidity = 0;
bool locationInvalid = false;

uint16_t hex2RGB(String hex) {
  if(hex.length() < 7 || hex[0] != '#') return COLOR_WHITE; // Fallback against memory corruption
  long num = strtol(&hex[1], NULL, 16);
  return ( ((num >> 16 & 0xF8) << 8) | ((num >> 8 & 0xFC) << 3) | ((num & 0xFF) >> 3) );
}

void forceUIRefresh() {
  lastMin = -1;
  lastSec = -1;
  lastDay = -1;
  lastTemp = -999.0;
  lastHumidity = -999;
  lastLocation = "";
  lastWeatherDesc = "";
}

void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File file = LittleFS.open("/config.json", "r");
    StaticJsonDocument<1024> doc;
    if (!deserializeJson(doc, file)) {
      if (doc["api_key"]) apiKey = doc["api_key"].as<String>();
      if (doc["location"]) location = doc["location"].as<String>();
      if (doc.containsKey("timeFormat24")) timeFormat24 = doc["timeFormat24"].as<bool>();
      if (doc.containsKey("luminosity")) luminosity = doc["luminosity"].as<int>();
      if (doc.containsKey("lumMode")) lumMode = doc["lumMode"].as<int>();

      if (doc.containsKey("hourColorHex")) hourColorHex = doc["hourColorHex"].as<String>();
      if (doc.containsKey("minColorHex")) minColorHex = doc["minColorHex"].as<String>();
      if (doc.containsKey("dateColorHex")) dateColorHex = doc["dateColorHex"].as<String>();
      if (doc.containsKey("locColorHex")) locColorHex = doc["locColorHex"].as<String>();
      if (doc.containsKey("tempBarColorHex")) tempBarColorHex = doc["tempBarColorHex"].as<String>();
      if (doc.containsKey("humBarColorHex")) humBarColorHex = doc["humBarColorHex"].as<String>();

      hourColor = hex2RGB(hourColorHex);
      minColor = hex2RGB(minColorHex);
      dateColor = hex2RGB(dateColorHex);
      if(locColorHex.startsWith("#")) locColor = hex2RGB(locColorHex);
      if(tempBarColorHex.startsWith("#")) tempBarColor = hex2RGB(tempBarColorHex);
      if(humBarColorHex.startsWith("#")) humBarColor = hex2RGB(humBarColorHex);
    }
    file.close();
  }
}

void saveConfig() {
  StaticJsonDocument<1024> doc;
  doc["api_key"] = apiKey;
  doc["location"] = location;
  doc["timeFormat24"] = timeFormat24;
  doc["luminosity"] = luminosity;
  doc["lumMode"] = lumMode;
  
  doc["hourColorHex"] = hourColorHex;
  doc["minColorHex"] = minColorHex;
  doc["dateColorHex"] = dateColorHex;
  doc["locColorHex"] = locColorHex;
  doc["tempBarColorHex"] = tempBarColorHex;
  doc["humBarColorHex"] = humBarColorHex;

  File file = LittleFS.open("/config.json", "w");
  serializeJson(doc, file);
  file.close();
}
