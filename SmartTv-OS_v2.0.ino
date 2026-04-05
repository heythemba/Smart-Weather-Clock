#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "Display.h"
#include "Network.h"

unsigned long lastTickUpdate = 0;

void handleIndex() {
  String html = F("<!DOCTYPE html><html><head><meta charset=UTF-8><meta name=viewport content='width=device-width,initial-scale=1'><title>SmartTV</title>"
    "<style>*{box-sizing:border-box}body{font:14px sans-serif;background:#111;color:#eee;max-width:400px;margin:auto;padding:12px}"
    "h2{color:#0ff;text-align:center}label{display:block;margin:6px 0 2px;color:#aaa}"
    "input,select{width:100%;padding:7px;background:#222;color:#fff;border:1px solid #444;border-radius:4px;margin-bottom:3px}"
    "ul{margin:0;padding-left:20px;color:#bbb;font-size:13px}"
    "input[type=color]{height:34px;padding:2px}"
    ".btn{width:100%;padding:9px;margin-top:6px;border:0;border-radius:4px;font-weight:bold;cursor:pointer;background:#0ff;color:#000}"
    ".r{background:#c00;color:#fff}#s{color:#0f0;text-align:center;margin-top:6px}</style></head><body>"
    "<h2>SmartTV Settings</h2>");

  String netsHTML = "";
  if (LittleFS.exists("/wifi.json")) {
    File file = LittleFS.open("/wifi.json", "r");
    if(file) {
      StaticJsonDocument<1024> doc;
      if (!deserializeJson(doc, file)) {
        JsonArray arr = doc["networks"].as<JsonArray>();
        for(JsonVariant v : arr) {
          if (v.containsKey("ssid")) {
            netsHTML += "<li>" + v["ssid"].as<String>() + "</li>";
          }
        }
      }
      file.close();
    }
  }
  if (netsHTML.length() > 0) netsHTML = "<button type=button class='btn r' style='background:#f90' onclick='document.getElementById(\"nl\").style.display=\"block\";this.style.display=\"none\"'>Show Saved Networks</button><ul id=nl style='display:none;margin-top:10px'>" + netsHTML + "</ul>";
  else netsHTML = "<label>Saved Networks</label><ul><li>None</li></ul>";

  html += "<label>API Key</label><input id=a value='" + apiKey + "'>";
  html += "<label>Location</label><input id=l value='" + location + "'>";
  html += F("<label>Time</label><select id=t><option value=true");
  if(timeFormat24) html += F(" selected");
  html += F(">24h</option><option value=false");
  if(!timeFormat24) html += F(" selected");
  html += F(">12h</option></select>");
  html += "<label>Lum Mode</label><select id=m onchange='b.disabled=(this.value==\"1\")'><option value=0";
  if(lumMode==0) html += F(" selected");
  html += F(">Manual</option><option value=1");
  if(lumMode==1) html += F(" selected");
  html += F(">Auto</option></select>");
  html += "<label>Brightness Level</label><input id=b type=range min=0 max=100 style='width:100%' value='" + String(luminosity) + "'>";
  html += "<label>Location Color</label><input type=color id=lc value='" + locColorHex + "'>";
  html += "<label>Hour Color</label><input type=color id=hrc value='" + hourColorHex + "'>";
  html += "<label>Minute Color</label><input type=color id=mnc value='" + minColorHex + "'>";
  html += "<label>Date Color</label><input type=color id=dc value='" + dateColorHex + "'>";
  html += "<label>Temp Color</label><input type=color id=tc value='" + tempBarColorHex + "'>";
  html += "<label>Humidity Color</label><input type=color id=hc value='" + humBarColorHex + "'>";
  html += netsHTML;
  html += F("<button class=btn onclick=save()>Save Settings</button>"
    "<button class='btn r' onclick=rst()>Reset Wi-Fi</button>"
    "<div id=s></div>"
    "<script>"
    "window.onload=()=>{b.disabled=(m.value=='1');};"
    "function save(){fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},"
    "body:JSON.stringify({api_key:a.value,location:l.value,timeFormat24:t.value==='true',"
    "luminosity:+b.value,lumMode:+m.value,locColorHex:lc.value,hourColorHex:hrc.value,minColorHex:mnc.value,"
    "dateColorHex:dc.value,tempBarColorHex:tc.value,humBarColorHex:hc.value})})"
    ".then(r=>{document.getElementById('s').textContent=r.ok?'Saved!':'Error';});}"
    "function rst(){if(confirm('Forget Wi-Fi?'))fetch('/resetWiFi',{method:'POST'})"
    ".then(()=>{document.body.innerHTML='<h2>Rebooting...</h2><p>Connect to SmartTV_Setup</p>';});}"
    "</script></body></html>");
  server.send(200, "text/html", html);
}


void handleSave() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, server.arg("plain"));
    
    apiKey = doc["api_key"].as<String>();
    location = doc["location"].as<String>();
    timeFormat24 = doc["timeFormat24"].as<bool>();
    luminosity = doc["luminosity"].as<int>();
    lumMode = doc["lumMode"].as<int>();
    
    locColorHex = doc["locColorHex"].as<String>();
    hourColorHex = doc["hourColorHex"].as<String>();
    minColorHex = doc["minColorHex"].as<String>();
    dateColorHex = doc["dateColorHex"].as<String>();
    tempBarColorHex = doc["tempBarColorHex"].as<String>();
    humBarColorHex = doc["humBarColorHex"].as<String>();

    locColor = hex2RGB(locColorHex);
    hourColor = hex2RGB(hourColorHex);
    minColor = hex2RGB(minColorHex);
    dateColor = hex2RGB(dateColorHex);
    tempBarColor = hex2RGB(tempBarColorHex);
    humBarColor = hex2RGB(humBarColorHex);

    saveConfig();
    
    // Instant Enforcement Mechanism
    forceUIRefresh(); // force all logic variables into reset to instantly redraw shapes
    fetchWeatherData(); // get weather mapped to instantly grab the current location bounds
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleResetWiFi() {
  server.send(200, "text/plain", "Resetting");
  delay(100);
  LittleFS.remove("/wifi.json");
  WiFi.disconnect(true);
  ESP.eraseConfig(); // Utterly obliterate physical SDK NVS flash memory so Captive Portal MUST respawn
  delay(1000);
  ESP.restart();
}

void setup() {
  // Force backlight ON!
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, LOW);
  
  Serial.begin(74880);
  delay(10);
  Serial.println("\n[System] Booting SmartTV OS V2.0...");

  if (!LittleFS.begin()) Serial.println("[System] LittleFS Mount Failed");

  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  SPI.begin();
  SPI.setFrequency(20000000);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  
  initDisplay();
  fillScreen(COLOR_BLACK);
  
  loadConfig();
  
  // Bind WiFiMulti modules gracefully
  setupNetwork();
  
  fillScreen(COLOR_BLACK); // Provide clean layout loop natively

  server.on("/", handleIndex);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/resetWiFi", HTTP_POST, handleResetWiFi);
  server.begin();
  
  fetchWeatherData();
}

void loop() {
  server.handleClient();
  
  unsigned long nowMillis = millis();
  
  // Native OpenWeatherMap checks every 10 min
  if (nowMillis - lastWeatherUpdate >= 600000) {
    lastWeatherUpdate = nowMillis;
    fetchWeatherData();
  }
  
  // Protect visual rendering loops cleanly via bounds check preventing aggressive 52mhz CPU load
  if (nowMillis - lastTickUpdate >= 50) {
      lastTickUpdate = nowMillis;
      drawUI();
      
      // Update LED Backlight Hardware PWM safely mapped to non-cutoff voltage
      int targetLum = luminosity;
      if (lumMode == 1) { // Auto Mode Night-vs-Day checks
         if (weatherDesc.indexOf("night") >= 0) targetLum = 30;
         else targetLum = 100;
      }
      
      // Map active-low backlight smoothly. 100% UI = 0 PWM (Fully ON). 0% UI = 204 PWM (~80% cutoff boundary).
      int pwmOut = map(targetLum, 0, 100, 204, 0);
      analogWrite(PIN_BL, pwmOut);
  }
}
