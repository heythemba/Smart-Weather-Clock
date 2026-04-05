#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include "Network.h"
#include "Config.h"
#include "Display.h"

ESP8266WiFiMulti wifiMulti;


void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("[WiFi] Entered Configuration Mode");
  // Clean wipe the entire lower half of the screen from the "Scanning..." elements
  fillRect(0, 100, 240, 140, COLOR_BLACK); 
  
  drawString(10, 115, "1. Connect Phone:", COLOR_CYAN, COLOR_BLACK, 2);
  drawString(10, 140, myWiFiManager->getConfigPortalSSID().c_str(), COLOR_YELLOW, COLOR_BLACK, 2);
  
  drawString(10, 180, "2. Go to Browser:", COLOR_GREEN, COLOR_BLACK, 2);
  drawString(10, 205, "192.168.4.1", COLOR_WHITE, COLOR_BLACK, 2);
}

// Lumination is dynamically controlled in the main loop instead

void setupNetwork() {
  pinMode(PIN_BL, OUTPUT); 

  WiFi.mode(WIFI_STA); // Enforce STA lock precisely

  String storedSSID = WiFi.SSID();
  String storedPass = WiFi.psk();
  
  if(storedSSID.length() > 0) {
      wifiMulti.addAP(storedSSID.c_str(), storedPass.c_str());
  }

  if (LittleFS.exists("/wifi.json")) {
    File file = LittleFS.open("/wifi.json", "r");
    if(file) {
        StaticJsonDocument<1024> doc;
        if (!deserializeJson(doc, file)) {
            JsonVariant netVar = doc["networks"];
            if (netVar.is<JsonArray>()) {
               JsonArray arr = netVar.as<JsonArray>();
               for(JsonVariant v : arr) {
                  if (v.containsKey("ssid") && v.containsKey("pass")) {
                     wifiMulti.addAP(v["ssid"].as<String>().c_str(), v["pass"].as<String>().c_str());
                  }
               }
            }
        }
        file.close();
    }
  }

  Serial.println("[Setup] Scanning Known Networks via WiFiMulti...");
  drawString(10, 110, "Scanning Saved WiFi...", COLOR_CYAN, COLOR_BLACK, 2);
  drawRoundRect(20, 140, 200, 12, 6, COLOR_WHITE);
  fillRoundRect(22, 142, 80, 8, 4, COLOR_CYAN); // Progress ~40%
  
  bool connected = false;
  // Shorter waiting threshold so it doesn't freeze for 10 solid seconds
  for(int i=0; i<3; i++) {
     if(wifiMulti.run() == WL_CONNECTED) { 
         connected = true; break; 
     }
     delay(1000);
  }

  // If WiFiMulti failed completely, spin up AP
  if(!connected) {
     Serial.println("[Setup] Launching Captive Portal...");
     WiFiManager wm;
     wm.setAPCallback(configModeCallback);
     
     // Blocks here until user configures natively
     if (!wm.autoConnect("SmartTV_Setup")) {
        delay(3000);
        ESP.restart(); // Recovery
     }



     // Connected! Save new credentials to LittleFS
     String newSsid = WiFi.SSID();
     String newPass = WiFi.psk();
     
     if (newSsid.length() > 0) {
       StaticJsonDocument<1024> wifij;
       File fr = LittleFS.open("/wifi.json", "r");
       if(fr) {
           deserializeJson(wifij, fr);
           fr.close();
       }
       
       JsonArray arr = wifij["networks"];
       if (arr.isNull()) {
           arr = wifij.createNestedArray("networks");
       }
       
       JsonObject net = arr.createNestedObject();
       net["ssid"] = newSsid;
       net["pass"] = newPass;
       
       File fw = LittleFS.open("/wifi.json", "w");
       serializeJson(wifij, fw);
       fw.close();
     }
  }

  // Network secured!
  fillScreen(COLOR_BLACK);
  drawString(20, 110, "Fetching NTP...", COLOR_CYAN, COLOR_BLACK, 2);
  drawRoundRect(20, 140, 200, 12, 6, COLOR_WHITE);
  fillRoundRect(22, 142, 140, 8, 4, COLOR_CYAN); // Progress ~70%

  Serial.println("[Setup] Configuring NTP Time...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
  tzset();
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[Weather] Fetching data for %s...\n", location.c_str());
    WiFiClient client;
    HTTPClient http;
    String safeLocation = location;
    safeLocation.replace(" ", "%20");
    
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + safeLocation + "&appid=" + apiKey + "&units=metric";
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, payload); 
      
      if (doc["cod"] == 200) {
        currentTemp = doc["main"]["temp"];
        currentHumidity = doc["main"]["humidity"];
        weatherDesc = doc["weather"][0]["description"].as<String>();
        
        // Append night explicitly via Icon code checking so our graphic mapper detects it smoothly
        String iconId = doc["weather"][0]["icon"].as<String>();
        if(iconId.indexOf('n') >= 0) weatherDesc += " night";
        else weatherDesc += " day";

        locationInvalid = false;
        
        Serial.printf("[Weather] Success: %.1f C, %d%% Hum, %s\n", currentTemp, currentHumidity, weatherDesc.c_str());

        if (weatherDesc.length() > 0) {
          weatherDesc[0] = toupper(weatherDesc[0]);
        }
      } else {
        locationInvalid = true;
        String apiErrorMsg = doc["message"].as<String>();
        if(apiErrorMsg.length() > 0) {
           weatherDesc = "API Error";
           Serial.printf("[Weather] API Error Code %d: %s\n", doc["cod"].as<int>(), apiErrorMsg.c_str());
        } else {
           weatherDesc = "Invalid Loc";
           Serial.printf("[Weather] Error: Invalid Location '%s'\n", location.c_str());
        }
      }
    } else {
      locationInvalid = true;
      weatherDesc = "Timeout";
      Serial.printf("[Weather] Critical HTTP Connection Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}
