import os
import glob
from PIL import Image

workspace = r"c:\Users\heyth\OneDrive\Documents\Arduino\SmartTv-OS_v2.0"
png_files = glob.glob(os.path.join(workspace, "*.png"))

out_str = "#pragma once\n#include <Arduino.h>\n\n"

for file in png_files:
    name = os.path.basename(file).split('.')[0].replace(' ', '_').lower()
    
    img = Image.open(file).convert("RGBA")
    img = img.resize((40, 40))
    pixels = img.load()
    
    rgb565_bytes = []
    
    for y in range(40):
        for x in range(40):
            r, g, b, a = pixels[x, y]
            if a < 128: 
               # Black background for transparent limits securely
               rgb565_bytes.extend([0x00, 0x00])
            else:
               r_5 = (r >> 3) & 0x1F
               g_6 = (g >> 2) & 0x3F
               b_5 = (b >> 3) & 0x1F
               val = (r_5 << 11) | (g_6 << 5) | b_5
               # Big Endian SPI transfer order cleanly
               rgb565_bytes.append(val >> 8)
               rgb565_bytes.append(val & 0xFF)
               
    out_str += f"const uint32_t len_{name} = {len(rgb565_bytes)};\n"
    out_str += f"const uint8_t image_{name}[{len(rgb565_bytes)}] PROGMEM = {{\n"
    
    for i in range(0, len(rgb565_bytes), 16):
         chunk = rgb565_bytes[i:i+16]
         out_str += "  " + ", ".join([f"0x{b:02X}" for b in chunk]) + ",\n"
        
    out_str += "};\n\n"

with open(os.path.join(workspace, "WeatherGraphics.h"), "w") as f:
    f.write(out_str)
print("Conversion complete!")
