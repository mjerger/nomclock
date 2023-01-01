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

#include "config.h"
ESP8266WebServer server(80);

// NTP
#define MY_NTP_SERVER "at.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0,M1128.0/3"
time_t now;
tm tm;

// LEDs
#define BRIGHTNESS 128
#define FPS 30
#define NUM_LEDS (24+12)
CRGB leds[NUM_LEDS];
#define LED_PIN 2


void setup(void)
{
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
  FastLED.setMaxPowerInVoltsAndMilliamps(5,2000);
  FastLED.setBrightness(255);
  for (int i=0; i<NUM_LEDS; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[i] = ColorFromPalette(RainbowColors_p, (255/NUM_LEDS)*i, 255, LINEARBLEND);
    FastLED.delay(30);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(BRIGHTNESS);
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

void blur(CRGB *leds, int num) {
  for (int i=0; i<num; i++) {
    leds[(i+1)%num] = blend(leds[(i+1)%num], leds[i], 128);
  }
}

void loop(void)
{  
  // NTP time
  time(&now);
  localtime_r(&now, &tm);
  
  const double ms = (millis() / (1000/24)) % 24;   // 0..23
  const double s  = (double)tm.tm_sec / 2.5;       // 0..23
  const double m  = (double)tm.tm_min / 2.5;       // 0..23
  const double h  = (double)(tm.tm_hour % 12); // 0..12

  const int mi  = m >=12 ? m -12 : 12+m;
  const int si  = s >=12 ? s -12 : 12+s;
  const int msi = ms>=12 ? ms-12 : 12+ms;
  const int hi  = h >=6  ? h -6  :  6+h; 

  // Fade out linearly
  const int st = 8;
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i].r = leds[i].r > st ? leds[i].r - st : 0;
    leds[i].g = leds[i].g > st ? leds[i].g - st : 0;
    leds[i].b = leds[i].b > st ? leds[i].b - st : 0;
  }

  // Set LEDs
  leds[mi]           = blend(leds[mi],           ColorFromPalette(RainbowColors_p,  ((256/60)*tm.tm_min + 128) % 256, 255, LINEARBLEND), 128);
  leds[si]           = blend(leds[si],           ColorFromPalette(RainbowColors_p,  ((256/60)*tm.tm_sec+(millis()%500)/4) % 256,  255, LINEARBLEND), 128);
  leds[24+hi]        = blend(leds[24+hi],        ColorFromPalette(RainbowColors_p,  ((256/24)*tm.tm_hour + 64) % 256,      255, LINEARBLEND), 128);
  leds[msi]          = blend(leds[msi],          CRGB::White, 32);

  // Output
  FastLED.delay(1000/FPS);
  yield();
}
