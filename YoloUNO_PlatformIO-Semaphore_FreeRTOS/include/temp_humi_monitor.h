#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "DHT20.h"
#include "global.h"

// Task đọc DHT20 (producer): ghi dữ liệu vào biến toàn cục rồi give xDataReadySem.
void task_sensor_reader(void *pvParameters);
// Task hiển thị (consumer): take xDataReadySem, đọc biến toàn cục và in ra Serial.
void task_display(void *pvParameters);
// Task heartbeat: chạy song song, định kỳ đọc biến toàn cục và log (cạnh tranh mutex).
void task_heartbeat(void *pvParameters);

#endif
