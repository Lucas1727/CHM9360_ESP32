#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <DNSServer.h>    //https://github.com/zhouhan0126/DNSServer---esp32
#include <WebServer.h>    //https://github.com/zhouhan0126/WebServer-esp32
#include <WiFiManager.h>    //https://github.com/zhouhan0126/WIFIMANAGER-ESP32
#include <WebSocketsServer.h>   //https://github.com/Links2004/arduinoWebSockets/
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include <RGBmatrixPanel.h>   //https://github.com/adafruit/RGB-matrix-Panel
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ESPAsyncWebServer.h>    //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFS.h>   //https://github.com/me-no-dev/ESPAsyncWebServer

#define CLK 14
#define OE  13
#define LAT 15
#define A   26
#define B   4
#define C   27

// RGB pins for ESP 32
uint8_t rgbpins[] = { 5, 17, 18, 19, 16, 25 };

RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, true, rgbpins);

#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

char displayText[50] = "";

const char* WiFi_SSID = "ESP32";
const char* WiFi_Password = "ESP32";
const char* ntpServer = "pool.ntp.org";
const char* googleServer = "www.googleapis.com";

const int  mqttPort = 1883;
const int  daylightOffset_sec = 0;

const long gmtOffset_sec = 3600;

boolean transitionModules[5] = {false, false, true, false, false};   //1.Message, 2.Weather, 3.Clock, 4.Sensor, 5.Youtube

char       displayOption[10],
           mqttServer[30],
           weatherServer[] = "api.openweathermap.org";

String     OMWApiKey = "",
           OWMLocation = "",
           weatherType = "",
           ipAddress = "",
           YTApiKey = "",
           YTChannelId = "",
           customMessage = "";

float      sensorTemp = 0;    //ESP8226 MQTT reading value for the sensors temperature reading

long       youtubeSubscribers = 0;

int        hue = 0,   //Colour hue on LED for text (text range 0-7)
           colourRed = 1,   //Colour value on LED for red on text (text range 0-7)
           colourGreen = 1,   //Colour value on LED for green on text (text range 0-7)
           colourBlue = 1,    //Colour value on LED for blue on text (text range 0-7)
           actionTimer = 0,   //Counting up timer for when an action starts
           sideScroll = 1,    //Text sidescrollign active or not
           weatherTemp = 0,   //Weather tempertature
           weatherWindSpd = 0,    //Weather wind speed
           dly = 50,    //Delay before next loop of the code
           updateTime = 300,   // Seconds before the display updates information from internet
           connectedToMQTT = 0,   //States whether ESP32 is connected to MQTT server or not
           transTime = 10,    //Time is takes for transition to next module to display
           transTimer = 0,   //Counting up timer for when an transition starts
           transModeEnabled = 0,   //Enables transition mode if 1
           transPosition = 0;   //Position in the transition queue

int8_t     sat = 255,
           val = 255;

int16_t    textX = matrix.width(),
           textMin = strlen(displayText) * -6;

const unsigned char PROGMEM weatherSunBitmap[] = {
  0x24, 0x0, 0x99, 0x3c, 0x3c, 0x99, 0x0, 0x24,
};

const unsigned char PROGMEM weatherCloudBitmap[] = {
  0x3c, 0x7e, 0xff, 0xff, 0x66, 0x0, 0x0, 0x0,
};

const unsigned char PROGMEM weatherRainBitmap[] = {
  0x0, 0x0, 0x0, 0x0, 0x0, 0x88, 0x21, 0x4,
};

const unsigned char PROGMEM weatherClearBitmap[] = {
  0x0, 0x0, 0x0, 0x0, 0x0, 0x60, 0xf0, 0x0,
};

const unsigned char PROGMEM weatherErrorBitmap[] = {
  0x3c, 0x42, 0x85, 0x89, 0x91, 0xa1, 0x42, 0x3c,
};

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

WiFiClient espClient;
WiFiClientSecure espClientSecure;
WiFiManager wifiManager;
PubSubClient client(espClient);
StaticJsonDocument<6000> JSONDoc;

void setup()
{
  Serial.begin(115200);

  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  matrix.setTextSize(1);
  matrix.setTextColor(matrix.Color333(colourRed, colourGreen, colourBlue));
  matrix.setCursor(1, 1);
  matrix.print(F2("Boot"));

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect(WiFi_SSID, WiFi_Password);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  strncpy(displayOption, "ip", 8);

  // Initialize and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getWeather();
  getSubscribers();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on(
    "/",
    HTTP_POST,
  [](AsyncWebServerRequest * request) {},
  NULL,
  [] (AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {

    for (size_t i = 0; i < len; i++) {
      mqttServer[i] = data[i];
    }

    request->send(200);

    connectToMQTT();
    //request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load main.js file
  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/main.js", "text/javascript");
  });

  ipAddress = (WiFi.localIP().toString());

  // Start server
  server.begin();

  printIPAddress();

  connectToMQTT();
}

//------------------------------------------------------------------------------------

void connectToMQTT() {
  if (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    Serial.print(mqttServer);

    if (client.connect("ESP32Client-1727")) {
      Serial.println("connected");
      connectedToMQTT = 1;
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      connectedToMQTT = 0;
    }
  }
  client.subscribe("SmartDisplay");
  client.subscribe("tele/sonoff/SENSOR");
}

// Replaces placeholder with IP Address
String processor(const String& var) {
  //Serial.println(var);
  if (var == "IPADDRESS") {
    return ipAddress;
  }
  return String();
}

void callback(char* topic, byte* payload, unsigned int length) {

  char PAYLOAD[length];

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    PAYLOAD[i] = (payload[i]);
  }

  char* PAYLOAD_Char = PAYLOAD;

  Serial.println();
  Serial.println("-----------------------");

  DeserializationError error = deserializeJson(JSONDoc, (char*)PAYLOAD_Char);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }


  if (JSONDoc.containsKey("DHT11")) {
    Serial.println("Got temperature reading");
    //Serial.printf(JSONDoc);
    sensorTemp = JSONDoc["DHT11"]["Temperature"];
  } else {
    Serial.println("Got command reading");
    checkJsonForCommand();
  }
}

// Gets any recieved JSON information and interperates the commands and data passed to the ESP32
void checkJsonForCommand() {

  const char* command = JSONDoc["command"];
  const char* text = JSONDoc["text"];
  int value = JSONDoc["value"];

  const char* prevDisplayOption = displayOption;

  memset(displayText, 0, sizeof(displayText));

  transModeEnabled = 0;

  if (strcmp(command, "test") == 0) {
    strncpy(displayOption, "test", 10);
    strcpy(displayText, text);
  } else if (strcmp(command, "clock") == 0) {
    strncpy(displayOption, "clock", 10);
  } else if (strcmp(command, "message") == 0) {
    strncpy(displayOption, "message", 10);
    customMessage = String(text);
    customMessage.toCharArray(displayText, customMessage.length() + 1);
  } else if (strcmp(command, "weather") == 0) {
    strncpy(displayOption, "weather", 10);
    actionTimer = updateTime * 20 - 10;
    OWMLocation = text;
  } else if (strcmp(command, "textCol") == 0) {
    colourRed = JSONDoc["colR"];
    colourGreen = JSONDoc["colG"];
    colourBlue = JSONDoc["colB"];
    matrix.setTextColor(matrix.Color333(colourRed, colourGreen, colourBlue));
    if (strcmp(prevDisplayOption, "message") == 0) {
      customMessage.toCharArray(displayText, customMessage.length() + 1);
    }
  } else if (strcmp(command, "youtube") == 0) {
    strncpy(displayOption, "youtube", 10);
    actionTimer = updateTime * 20 - 10;
    YTChannelId = text;
    Serial.println(YTChannelId);
  } else if (strcmp(command, "sensor") == 0) {
    strncpy(displayOption, "sensor", 10);
    strcpy(displayText, text);
  } else if (strcmp(command, "OWMKey") == 0) {
    OMWApiKey = text;
  } else if (strcmp(command, "YTKey") == 0) {
    YTApiKey = text;
  } else if (strcmp(command, "transMode") == 0) {
    transModeEnabled = 1;
    //transitionModules = text;
    transitionModules[0] = JSONDoc["text"]["Message"];
    transitionModules[1] = JSONDoc["text"]["Weather"];
    transitionModules[3] = JSONDoc["text"]["Sensor"];
    transitionModules[4] = JSONDoc["text"]["YouTube"];
    Serial.println(text);
  } else if (strcmp(command, "transTime") == 0) {
    transTime = value;
    Serial.println(transTime);
  } else {
    strncpy(displayOption, "default", 10);
  }

  Serial.printf("The sentence entered is %u characters long.\n", (unsigned)strlen(displayText));
  textMin = strlen(displayText) * -6;
}

// Creates an access point to setup the ESP32's network
void configModeCallback (WiFiManager *myWiFiManager) {

  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

}

//------------------------------------------------------------------------------------

// Calls the youtube API to get the number of subscribers for a given channel
void getSubscribers()
{
  int isJson = 0,
      maxAttempts = 10;
  String readLine, lineBuffer = "";

  if (!espClientSecure.connect(googleServer, 443)) {
    Serial.println("Connection failed");
    return;
  } else {
    Serial.print("Connected to ");
    Serial.println(googleServer);
  }

  espClientSecure.print("GET /youtube/v3/channels?part=statistics&id=" + YTChannelId + "&key=" + YTApiKey + " HTTP/1.1\r\n" + "Host: " + googleServer + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n");

  while (!espClientSecure.available() && maxAttempts--) {
    delay(100);
    Serial.println("connection not available");
  }

  while (espClientSecure.connected() && espClientSecure.available()) {
    readLine = espClientSecure.readStringUntil('\n');

    if (readLine[0] == '{') {
      isJson = 1;
    }

    if (isJson == 1) {
      for (int i = 0; i < readLine.length(); i++)
        if (readLine[i] == '[' || readLine[i] == ']') readLine[i] = ' ';
      lineBuffer += readLine + "\n";
    }
  }

  espClientSecure.stop();

  DeserializationError error = deserializeJson(JSONDoc, lineBuffer);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  youtubeSubscribers = JSONDoc["items"]["statistics"]["subscriberCount"];

  Serial.print("Subs: ");
  Serial.println(youtubeSubscribers);
}

// Prints subscriber information on LED matrix
void printYoutubeLED() {
  matrix.setCursor(1, 0);
  matrix.print("Subs:");

  sprintf(displayText, "%lu", youtubeSubscribers);

  matrix.setCursor(textX, 8);
  matrix.print(displayText);
}

// Prints the IP address of the smart display
void printIPAddress() {
  matrix.setCursor(1, 0);
  matrix.print("IP:");

  matrix.setCursor(textX, 8);
  matrix.print(ipAddress);
}

// Gets a temperature sensor reading from a ESP8266 and displays it on the smart display
void printSensorInfoLED() {
  matrix.setCursor(0, 0);
  matrix.print("Temp:");

  dtostrf(sensorTemp, 4, 1, displayText);

  displayText[4] = 'C';

  matrix.setCursor(0, 8);
  matrix.print(displayText);
}

// Gets the local time from pool.ntp.org and prints it on LED matrix
void printLocalTimeLED() {
  char displayTime[8] = "";

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.print("Failed to obtain time");
    return;
  }

  strftime(displayTime, sizeof(displayTime), "%H:%M", &timeinfo);
  strftime(displayText, sizeof(displayText), "%a %d %B", &timeinfo);
  textMin = strlen(displayText) * -6;

  matrix.setCursor(1, 0);
  matrix.print(F2(displayTime));

  matrix.setCursor(textX, 8);
  matrix.print(F2(displayText));
}

// Sends a request to api.openweathermap.org to get weather info for a location in JSON
void getWeather() {

  String line = "";

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (espClient.connect(weatherServer, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    espClient.print("GET /data/2.5/forecast?");
    espClient.print("q=" + OWMLocation);
    espClient.print("&appid=" + OMWApiKey);
    espClient.print("&cnt=3");
    espClient.println("&units=metric");
    espClient.println("Host: api.openweathermap.org");
    espClient.println("Connection: close");
    espClient.println();

    while (espClient.connected()) {
      if (espClient.available()) {
        line = espClient.readStringUntil('\n');
        Serial.println(line);
      }
    }

    espClient.stop();

    DeserializationError error = deserializeJson(JSONDoc, line);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    } else {
      weatherTemp = JSONDoc["list"][0]["main"]["temp"];
      weatherWindSpd = JSONDoc["list"][0]["wind"]["speed"];
      String temp = JSONDoc["list"][0]["weather"][0]["main"];
      weatherType = temp;

      Serial.print("Weather: ");
      Serial.println(weatherType);

      return;
    }

  } else {
    Serial.println("unable to connect");
    return;
    //espClient.stop();
    //matrix.drawPixel(31, 0, matrix.Color333(0, 0, 7));
  }
}

// Prints the stored weather information on LED matrix
void printWeatherLED() {
  //Serial.println("Printing Weather");

  matrix.setCursor(1, 0);
  matrix.print(weatherTemp);

  matrix.setCursor(13, 0);
  matrix.print(F2("C"));

  matrix.setCursor(1, 8);
  matrix.print(weatherWindSpd);

  matrix.setCursor(13, 8);
  matrix.print(F2("M/S"));

  if (weatherType.equals("Clear")) {
    matrix.drawBitmap(24, 0,  weatherSunBitmap, 8, 8, matrix.Color333(4, 3, 0));
    matrix.drawBitmap(24, 0,  weatherClearBitmap, 8, 8, matrix.Color333(1, 1, 2));
  } else if (weatherType.equals("Clouds")) {
    matrix.drawBitmap(24, 0,  weatherCloudBitmap, 8, 8, matrix.Color333(1, 1, 2));
  } else if (weatherType.equals("Rain")) {
    matrix.drawBitmap(24, 0,  weatherCloudBitmap, 8, 8, matrix.Color333(1, 1, 2));
    matrix.drawBitmap(24, 0,  weatherRainBitmap, 8, 8, matrix.Color333(0, 0, 2));
  } else if (weatherType.equals("Sun")) {
    matrix.drawBitmap(24, 0,  weatherSunBitmap, 8, 8, matrix.Color333(4, 3, 0));
  } else {
    matrix.drawBitmap(24, 0,  weatherErrorBitmap, 8, 8, matrix.Color333(1, 0, 0));
  }
}

//------------------------------------------------------------------------------------

// Changes the current module on screen to the next one that has been requested to be displayed
void transitionMode() {
  if (transTimer > (transTime * 20) - 1) {
    Serial.println("Transition");

    while (transitionModules[transPosition] == false || transPosition >= 5) {
      transPosition += 1;

      if (transPosition > 4) {
        transPosition = 0;
      }
    }

    if (transitionModules[transPosition] == true) {
      Serial.print("Position = ");
      Serial.println(transPosition);

      memset(displayText, 0, sizeof(displayText));

      if (transPosition == 0) {   //Message
        strncpy(displayOption, "message", 10);
        customMessage.toCharArray(displayText, customMessage.length() + 1);
      } else if (transPosition == 1) {    // Weather
        strncpy(displayOption, "weather", 10);
      } else if (transPosition == 2) {    //Clock
        strncpy(displayOption, "clock", 10);
      } else if (transPosition == 3) {    //Sensor
        strncpy(displayOption, "sensor", 10);
      } else if (transPosition == 4) {    //Youtube
        strncpy(displayOption, "youtube", 10);
      }
      textMin = strlen(displayText) * -6;
    }

    transPosition += 1;
    transTimer = 0;
  } else {
    transTimer += 1;
  }
}

// Updates the current display modes information to be displayed
void  updateAction() {
  if (actionTimer > (updateTime * 20) - 1) {
    if (strcmp(displayOption, "weather") == 0) {
      getWeather();
    } else if (strcmp(displayOption, "youtube") == 0) {
      //String subscribersString = String(getSubscribers());
      getSubscribers();
    }
    actionTimer = 0;
    connectToMQTT();
  } else {
    actionTimer += 1;
    //Serial.println(actionTimer);
  }
}

// Determines which display mode is currently selected
void currentDisplay() {

  if (strcmp(displayOption, "clock") == 0) {
    sideScroll = 1;
    printLocalTimeLED();
  } else if (strcmp(displayOption, "test") == 0) {
    sideScroll = 1;
    matrix.setCursor(textX, 8);
    matrix.print(F2(displayText));
    textMin = strlen(displayText) * -6;
  } else if (strcmp(displayOption, "message") == 0) {
    sideScroll = 1;
    matrix.setCursor(textX, 8);
    matrix.print(F2(displayText));
    textMin = strlen(displayText) * -6;
  } else if (strcmp(displayOption, "weather") == 0) {
    sideScroll = 0;
    printWeatherLED();
  } else if (strcmp(displayOption, "ip") == 0) {
    sideScroll = 1;
    printIPAddress();
    textMin = 13 * -6;
  } else if (strcmp(displayOption, "youtube") == 0) {
    sideScroll = 1;
    printYoutubeLED();
    textMin = strlen(displayText) * -6;
  } else if (strcmp(displayOption, "sensor") == 0) {
    printSensorInfoLED();
  } else {
    sideScroll = 1;
    matrix.print(F2("No Function"));
  }
}

// Side scrolls text which is on LED matrix
void sideScrollText() {
  // Move text left (w/wrap)
  if ((--textX) < textMin) textX = matrix.width();
}

// Changes the hue of the on screen text to created a rainbow effect
void rainbowTextColour() {
  if (colourRed == 0 && colourGreen == 0 && colourBlue == 0) {
    matrix.setTextColor(matrix.ColorHSV(hue, sat, val, true));
    hue += 7;
    if (hue >= 1536) hue -= 1536;
  }
}

// Check if MQTT server is connected, if not show error LED
void checkMQTTconnect() {
  if (connectedToMQTT == 0) {
    matrix.drawPixel(31, 0, matrix.Color333(7, 0, 0));
  }
}

void loop()
{
  // Clear background
  matrix.fillScreen(0);

  if (sideScroll) {
    sideScrollText();
  }

#if !defined(__AVR__)
  // On non-AVR boards, delay slightly so screen updates aren't too quick.
  delay(dly);
#endif

  client.loop();

  if (transModeEnabled) {
    transitionMode();
  }

  rainbowTextColour();
  currentDisplay();
  updateAction();
  checkMQTTconnect();

  // Update display
  matrix.swapBuffers(false);
}
