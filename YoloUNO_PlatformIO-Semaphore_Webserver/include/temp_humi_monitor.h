#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "DHT20.h"
#include "global.h"

// Task đọc DHT20 (SDA GPIO11, SCL GPIO12) định kỳ và cập nhật biến toàn cục.
void temp_humi_monitor(void *pvParameters);

#endif
