/*
 * Author: Raju Subramanian <raju@subramanian.in>
 * Forked & Modified from https://github.com/acrobotic/Ai_Demos_ESP8266/blob/master/epd_showerthoughts/epd_showerthoughts.ino
 * 
 * Arduino Code for displaying date/time and a random funny 
 * message on an e-paper display with an esp8266 driver board
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ezTime.h>
#include "Bitmaps.h"

// We won't need the GFX base class so disable it
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "FreeMonoBold36pt7b.h"

// Instantiate the GxEPD2 class for our display type
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(SS, 4, 5, 16));

////////////////////////////////////////////////////////////////
// MODIFY THESE TO SETUP THE WIFI CONNECTION
// Define WiFi and web server variables
const char* ssid = "...";
const char* password = "...";
///////////////////////////////////////////////////////////////

// Instantiate the WiFiClientSecure class to use it for creating a TLS connection
WiFiClientSecure client;

// Define web client constants
const char* host = "icanhazdadjoke.com";
const int httpsPort = 443;
// The Host changes SSL Certs every 6 months. So this will need to be updated
// and new firmware burnt to the board evertime the fingerprint changes.
const char fingerprint[] = "DF 8C 76 62 95 B2 0B E0 A0 1F C4 ED 39 77 09 F1 9F 51 30 57";

Timezone LosAngeles;

time_t currTime;
uint8_t pastMinute, currMinute;

String quote = "I am a hackable e-paper display, driven by an arduino compatible board (esp8266)."
               "\n\n"
               "Check out github.com/ra-ju/esp8266-epaper-frame to get started.";

void setup() {
  Serial.begin(115200);
  // Initialize communication with the display
  delay(100);
  display.init(115200);
  drawBitmap(STARTUP_LOGO);
  delay(1000);
  
  WiFi.begin(ssid, password);
  for (int retries = 0; retries <= 10; ++retries) {
    Serial.print(".");
    delay(1000);
  }
  if (WiFi.status() != WL_CONNECTED) {
    showWifiNotConnectedMessage();
    return;
  }
  Serial.println();
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  // Initialize ezTime
  setInterval(86400);
  waitForSync();
  LosAngeles.setLocation("America/Los_Angeles");
  LosAngeles.setDefault();
 
  delay(1000);

  // Configure the web client
  client.setFingerprint(fingerprint);
  // Get initial joke
  getDadJoke();

  currTime = now();
  pastMinute = minute(currTime);
  updateDisplay();  
}

void loop() {
  // put your main code here, to run repeatedly:
  // server.handleClient();
  if (WiFi.status() == WL_CONNECTED) {
    currTime = now();
    currMinute = minute(currTime);
    if (currMinute != pastMinute) {
      updateDisplay();
      pastMinute = currMinute;
    }
  
    // Get Random joke at the top of the hour between 7AM and 6PM.
    uint8_t currHour = hour(currTime);
    if (currMinute == 0 && currHour >= 7 && currHour <= 18) {
      getDadJoke();
    }
  }
  delay(1000);
}

void getDadJoke() {
  Serial.print("Heap (1): ");Serial.println(ESP.getFreeHeap());
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed!");
    return;
  }

  String url = "/";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +   
               "User-Agent: ESP8266 WaveShare E-paper (https://github.com/ra-ju)\r\n" +
               "Accept: text/plain\r\n" + 
               "Connection: close\r\n\r\n");
  String line = client.readStringUntil('\n');
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  // Read the response body
  while(client.connected()) {
    if(client.available()) {
      quote = client.readString();
      break;
    }
  }
  client.stop();
  Serial.print("Quote: ");
  Serial.println(quote);
}

void showWifiNotConnectedMessage() {
  int16_t tbx, tby; uint16_t x, y, tbw, tbh;

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold12pt7b);
  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  String msg = "Wifi Not connected :(\n\nCheck out https://github.com/ra-ju/esp8266-epaper-frame to get started";
  display.getTextBounds(msg, 0, 0, &tbx, &tby, &tbw, &tbh);
  x = (display.width() - tbw) / 2;
  y = (display.height() - tbh) / 2;

  do {
    display.setCursor(x, y);
    display.print(msg);
  } while(display.nextPage());
  // Conserve power
  display.powerOff();
}

void updateDisplay() {
  int16_t tbx, tby; uint16_t x, y, tbw, tbh;
  String dateStr = dateTime(currTime, "l, F jS");
  String timeStr = dateTime(currTime, "h:i");
  String amPm = dateTime(currTime, "A");


  display.setTextColor(GxEPD_BLACK);
  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  char msg[100];
  do {
    display.setFont(&FreeMonoBold36pt7b);
    display.setCursor(50, 60);
    display.print(timeStr);

    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(290, 60);
    display.print(amPm);

    display.setFont(&FreeMonoBold12pt7b);
    display.getTextBounds(dateStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = (display.width() - tbw) / 2;
    display.setCursor(x, 90);
    display.print(dateStr);

    display.setCursor(5, 100);
    display.print("____________________________");

    display.setFont(&FreeMonoBold9pt7b);
    display.getTextBounds(quote, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = (display.width() - tbw) / 2;
    display.setCursor(x, 150);
    display.print(quote);

  } while(display.nextPage());
  // Conserve power
  display.powerOff();
}

void drawBitmap(const unsigned char *bitmap) {
  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, bitmap, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_BLACK);
  } while(display.nextPage());
}
