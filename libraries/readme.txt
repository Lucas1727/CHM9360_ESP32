For information on installing libraries, see: http://www.arduino.cc/en/Guide/Libraries

Libraries to download and install to Documents/Arduino/libraries:

Adafruit_GFX_Library - https://github.com/adafruit/Adafruit-GFX-Library
ArduinoJson - https://github.com/bblanchon/ArduinoJson
arduinoWebSockets-master - https://github.com/Links2004/arduinoWebSockets/
AsyncTCP-master - https://github.com/me-no-dev/AsyncTCP
DNSServer---esp32-master - https://github.com/zhouhan0126/DNSServer---esp32
pubsubclient-master - https://github.com/knolleary/pubsubclient
ESPAsyncWebServer-master - https://github.com/me-no-dev/ESPAsyncWebServer
RGB-matrix-Panel-master - https://github.com/adafruit/RGB-matrix-Panel
WebServer-esp32-master - https://github.com/zhouhan0126/WebServer-esp32
WIFIMANAGER-ESP32-master - https://github.com/zhouhan0126/WIFIMANAGER-ESP32

Installing the ESP32 library files:
In the Arduino IDE go to settings then paste "https://dl.espressif.com/dl/package_esp32_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json" 
into the Additional Boards Manager URLs: box and then press OK at the bottom.

Next install the official ESP32 libraries by Espressif Systems through the board manager in the Arudino IDE.

Once the library files are installed some need to be removed as they conflict with the Wi-Fi manager.

Do this by navigating to "AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.x\libraries\"
Once in this directory remove the two libraries called DNSServer and WebServer.

Once that is all done the project code should all compile correctly without any issues.