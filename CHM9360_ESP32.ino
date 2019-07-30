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
#include <SPIFFS.h>

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
const char* mqttServer = "192.168.1.49";

const int mqttPort = 1883;

const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

char       displayOption[8],
           weatherServer[] = "api.openweathermap.org";

String     apiKey = "c557ec940c4b9d765f88eeadc6f223ad",
           location = "aberystwyth,GB",
           weatherType = "",
           ipAddress = "";

int        hue = 0,
           colourRed = 1,
           colourGreen = 1,
           colourBlue = 1,
           actionTimer = 0,
           reader = 1,
           sideScroll = 1,
           weatherTemp = 0,
           weatherWindSpd = 0,
           dly = 50,
           updateTime = 20; // Seconds before the display updates information from internet

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

const unsigned char PROGMEM weatherErrorBitmap[] = {
  0x3c, 0x42, 0x85, 0x89, 0x91, 0xa1, 0x42, 0x3c,
};

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);

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

  strncpy(displayOption, "clock", 8);

  // Initialize and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getWeather();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
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

  printLocalTimeLED();

  connectToMQTT();
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client-1727")) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.subscribe("SmartDisplay");
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

  checkJsonForCommand(PAYLOAD_Char);

}

// Gets any recieved JSON information and interperates the commands and data passed to the ESP32
void checkJsonForCommand(char* payload) {

  StaticJsonDocument<200> JSONDoc;

  DeserializationError error = deserializeJson(JSONDoc, (char*)payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  const char* command = JSONDoc["command"];
  const char* text = JSONDoc["text"];

  if (strcmp(command, "test") == 0) {
    strncpy(displayOption, "test", 8);
    strcpy(displayText, text);
  } else if (strcmp(command, "clock") == 0) {
    strncpy(displayOption, "clock", 8);
  } else if (strcmp(command, "message") == 0) {
    strncpy(displayOption, "message", 8);
    strcpy(displayText, text);
  } else if (strcmp(command, "weather") == 0) {
    //actionTimer = 100;
    strncpy(displayOption, "weather", 8);
  } else if (strcmp(command, "textCol") == 0) {
    colourRed = JSONDoc["colR"];
    colourGreen = JSONDoc["colG"];
    colourBlue = JSONDoc["colB"];

    matrix.setTextColor(matrix.Color333(colourRed, colourGreen, colourBlue));
  } else if (strcmp(command, "youtube") == 0) {
    strncpy(displayOption, "youtube", 8);
  } else {
    strncpy(displayOption, "default", 8);
  }

  Serial.printf("The sentence entered is %u characters long.\n", (unsigned)strlen(displayText));
  textMin = strlen(displayText) * -6;
}

// Creates an access point to setup the ESP32's network
void configModeCallback (WiFiManager *myWiFiManager) {

  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

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

// Prints the stored weather information on LED matrix
void printWeatherLED() {
  sideScroll = 0;

  matrix.setCursor(1, 0);
  matrix.print(weatherTemp);

  matrix.setCursor(13, 0);
  matrix.print(F2("C"));

  matrix.setCursor(1, 8);
  matrix.print(weatherWindSpd);

  matrix.setCursor(13, 8);
  matrix.print(F2("M/S"));

  if (weatherType.equals("Clear")) {
    matrix.drawBitmap(24, 0,  weatherCloudBitmap, 8, 8, matrix.Color333(1, 1, 2));
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

  //matrix.print(actionTimer);
}

// Sends a request to api.openweathermap.org to get weather info for a location in JSON
void getWeather() {
  Serial.println(displayOption);

  String line = "";

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (espClient.connect(weatherServer, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    espClient.print("GET /data/2.5/forecast?");
    espClient.print("q=" + location);
    espClient.print("&appid=" + apiKey);
    espClient.print("&cnt=3");
    espClient.println("&units=metric");
    espClient.println("Host: api.openweathermap.org");
    espClient.println("Connection: close");
    espClient.println();
  } else {
    Serial.println("unable to connect");
  }

  while (espClient.connected()) {
    line = espClient.readStringUntil('\n');
    Serial.println(line);
  }

  StaticJsonDocument<6000> JSONDoc;

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
}

// Prints subscriber information on LED matrix
void printYoutubeLED() {

}

// Updates the current display modes information to be displayed
void  updateAction() {
  if (actionTimer > (updateTime * 20) - 1) {
    if (strcmp(displayOption, "weather") == 0) {
      getWeather();
    }
    actionTimer = 0;
  } else {
    actionTimer += 1;
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
    matrix.setCursor(textX, 8);
    matrix.print(F2(displayText));
    textMin = strlen(displayText) * -6;
  } else if (strcmp(displayOption, "weather") == 0) {
    printWeatherLED();
  } else if (strcmp(displayOption, "youtube") == 0) {
    printYoutubeLED();
    matrix.setCursor(0, 8);
    matrix.print(F2("Subs"));
  } else {
    matrix.print(F2("Default"));
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

void loop()
{
  // Clear background
  matrix.fillScreen(0);

  if (sideScroll)
  {
    sideScrollText();
  }

#if !defined(__AVR__)
  // On non-AVR boards, delay slightly so screen updates aren't too quick.
  delay(dly);
#endif

  client.loop();
  connectToMQTT();

  rainbowTextColour();
  currentDisplay();
  updateAction();

  // Update display
  matrix.swapBuffers(false);
}
