#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ---------------- Dữ liệu cảm biến dùng chung ----------------
extern float glob_temperature;
extern float glob_humidity;

// Mutex bảo vệ glob_temperature / glob_humidity (task DHT20 ghi, webserver đọc)
extern SemaphoreHandle_t xDataMutex;

// ---------------- Wi-Fi ----------------
extern String ssid;            // SSID của Access Point do YoloUNO phát
extern String password;        // mật khẩu Access Point
extern String wifi_ssid;       // SSID mạng Wi-Fi (STA) nhập từ /settings
extern String wifi_password;   // mật khẩu mạng Wi-Fi (STA)
extern bool isWifiConnected;   // true khi đã kết nối STA thành công
// Binary semaphore: give khi kết nối Internet (STA) thành công,
// dùng cho các task cần Internet (ví dụ gửi dữ liệu lên CoreIOT).
extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif
