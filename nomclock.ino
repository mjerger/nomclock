/* 
 *  nomclock
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <uptime.h>
#include <uptime_formatter.h>
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h>
#include <time.h>


// WIFI HTTP server
#include "config.h"
const String methods[8]{ "ANY", "GET", "HEAD", "POST", "PUT", "PATCH", "DELETE", "OPTIONS" };
ESP8266WebServer server(80);

// NTP

#define MY_NTP_SERVER "at.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0,M1128.0/3"
time_t now;
tm tm;
const String weekdays[7]{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// LEDs

#define BRIGHTNESS 32
#define FPS 30
#define NUM_LEDS (24+12)
CRGB leds[NUM_LEDS];
#define LED_PIN 2


void setup(void) {

  Serial.begin(115200);

  delay(300);
  Serial.println("\n\n--------------");
  Serial.println(" nomclock 1.0");
  Serial.println("--------------");

  Serial.print("Initializing SPIFFS ... ");
  if (SPIFFS.begin()) {
    Serial.println("ok");
  } else {
    Serial.println("failed");
  }

  // LEDs
  Serial.println("Have " + String(NUM_LEDS) +" LEDs ");
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,2000);
  FastLED.show();
  for (int i=0; i<NUM_LEDS; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[i] = ColorFromPalette(RainbowColors_p, (255/NUM_LEDS)*i, 255, LINEARBLEND);
    FastLED.delay(30);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Connect to WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ok");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}


void loop(void) {

  //fill_solid(leds, NUM_LEDS, CRGB::Black);
   
  // NTP time
  time(&now);
  localtime_r(&now, &tm);
  
  const double ms = (millis() / (1000/24)) % 24;   // 0..23
  const double s  = (double)tm.tm_sec / 2.5;       // 0..23
  const double m  = (double)tm.tm_min / 2.5;       // 0..23
  const double h  = 0;//(double)(tm.tm_hour % 12); // 0..12

  const int mi  = m >=12 ? m -12 : 12+m;
  const int si  = s >=12 ? s -12 : 12+s;
  const int msi = ms>=12 ? ms-12 : 12+ms;
  const int hi  = h >=6  ? h -6  :  6+h; 

  leds[msi]          = blend(leds[msi],          CRGB::white, 64);
  leds[(mi+1)%24]    = blend(leds[(mi+1)%24],    ColorFromPalette(RainbowColors_p, (256/60)*tm.tm_min + 128, 255, LINEARBLEND), 128);
  leds[mi]           = blend(leds[mi],           ColorFromPalette(RainbowColors_p, (256/60)*tm.tm_min + 128, 255, LINEARBLEND), 128);
  leds[(si+1)%24]    = blend(leds[(si+1)%24],    ColorFromPalette(RainbowColors_p, (256/60)*tm.tm_sec,       255, LINEARBLEND), 128);
  leds[si]           = blend(leds[si],           ColorFromPalette(RainbowColors_p, (256/60)*tm.tm_sec,       255, LINEARBLEND), 128);
  leds[24+(hi+1)%12] = blend(leds[24+(hi+1)%12], ColorFromPalette(RainbowColors_p, (256/24)*tm.tm_hour,      255, LINEARBLEND), 128);
  leds[24+hi]        = blend(leds[24+hi],        ColorFromPalette(RainbowColors_p, (256/24)*tm.tm_hour,      255, LINEARBLEND), 128);

  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = blend(leds[i], CRGB::Black, 32); 
  }
  // Output
  FastLED.delay(1000/FPS);
  yield();
}
