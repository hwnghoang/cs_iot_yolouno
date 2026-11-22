#ifndef ___MAIN_SERVER__
#define ___MAIN_SERVER__
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "global.h"

#define LED1_PIN 48  // LED trên board (Bài 2)
#define LED2_PIN 45  // LED RGB NeoPixel trên board (WS2812)
#ifndef BOOT_PIN
#define BOOT_PIN 0   // nút BOOT (active-low)
#endif
//extern WebServer server;

//extern bool isAPMode;




String mainPage();
String settingsPage();

void startAP();
void setupServer();
void connectToWiFi();

void main_server_task(void *pvParameters);

#endif