# Smart Weather Clock 📺☁️

> **Free to use and edit. Not for commercial use.**

## 📖 The Story Behind This Firmware

I bought a cheap "Smart Weather Station" desktop gadget from AliExpress. It worked exactly as intended for a few months, until one day the screen animation completely broke and refused to work. I decided to see if I could update the firmware, only to discover that the product was heavily outdated and no longer officially supported by the manufacturer.

After digging through the internet, I found a repository by GEEKMAGIC, but the installation process for their replacement firmware was incredibly tedious and confusing to follow. I was almost ready to give up and throw the gadget away until I made a major hardware discovery: the entire product is simply powered by a standard **ESP8266** board utilizing the famous NodeMCU 12E serial chip (CH340C)!

Instead of relying on unsupported external files, I completely wiped the old firmware off the chip. Armed with the Arduino IDE and C/C++, I wrote a brand-new, vastly optimized operating system completely from scratch. 

I successfully replicated all of the core functionalities that were present before, but deliberately **dropped the GIF-upload capability**. The original firmware's dynamic GIF processing was secretly the root cause of the memory crashes and display freezes. By pre-encoding raw RGB565 sprites instead, this custom firmware runs incredibly fluidly without ever encountering WatchDog Timer (WDT) hardware panics.

If your cheap smart clock broke and you want to revive it into an ultra-stable gadget, it's very easy to do yourself. Just follow the steps below!

---

## 🛠️ Step-by-Step Installation Tutorial

### 1. Install the CH340 USB Driver
Because the clock utilizes a serial CH340C chip to communicate over USB, your computer needs the correct driver to talk to it.
* **Download:** Search online for the `CH340 Driver` (available for Windows, Mac, and Linux).
* **Install:** Extract the ZIP file and run `SETUP.EXE`. Click "Install". 
* **Verify:** Plug your clock into your computer using a data-capable USB-C cable. Open Windows Device Manager (or `lsusb` on Mac/Linux) and verify you see "USB-SERIAL CH340" listed under your COM ports.

### 2. Download and Setup Arduino IDE
* Download the **Arduino IDE 2.x** from the [official Arduino website](https://www.arduino.cc/en/software) and install it.
* Open Arduino IDE, go to **File > Preferences**, and paste this URL into the *Additional Boards Manager URLs* box:
  `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
* Go to **Tools > Board > Boards Manager**, search for `esp8266`, and install the package.
* Go to **Tools > Board** and select **NodeMCU 1.0 (ESP-12E Module)**.
* **CRITICAL:** Go to **Tools > Flash Size** and select **4MB (FS:2MB OTA:~1019KB)**. This allocates space for the LittleFS filesystem which saves your network configuration!

### 3. Install Required Libraries
Go to **Sketch > Include Library > Manage Libraries** and search for/install the following dependencies:
* `ArduinoJson` (by Benoit Blanchon)
* `WiFiManager` (by tzapu)
* *(Note: SPI, LittleFS, and ESP8266WiFi are built-in native to the ESP8266 Board Package and do not require manual installation).*

### 4. Upload the Firmware
1. Clone or download this repository as a `.ZIP` file and extract it.
2. Open `SmartTv-OS_v2.0.ino` in the Arduino IDE.
3. Select your CH340 COM Port under **Tools > Port**.
4. Press the right-pointing arrow at the top left named **Upload** (`Ctrl+U`).
5. Wait for the compilation and flashing process to reach 100%. The clock display will boot up and say "Scanning Saved WiFi...".

### 5. Setup Wi-Fi & Your Gadget!
1. Because the ESP8266 will naturally fail to find a saved network on its very first run, it will automatically spin up its own internal Server and Access Point!
2. Take your smartphone and look for a Wi-Fi network called **`SmartTV_Setup`**. Connect to it.
3. Your phone should automatically prompt you to "Sign in to network". If it doesn't, open a web browser and type `192.168.4.1`.
4. Click **Configure WiFi**. The background ESP8266 radar will mathematically scan your room and list your actual Wi-Fi networks.
5. Tap your Home Wi-Fi, enter the password, and hit Save.
6. The clock will instantly reboot, successfully connect, and fetch live weather parameters from the internet!

> **Bonus:** Once connected, the clock's display shows its active local IP (e.g., `192.168.1.55`). Type this IP into your computer's browser to open the highly customized **SmartTV Web Dashboard**, where you can uniquely configure clock colors, brightness, timestamps, and 12h/24h intervals completely remotely!

---

## 📜 License
This codebase is completely **100% free to use, modify, and distribute for personal projects**. However, **NO commercial use** is permitted. Have fun reviving your hardware!
