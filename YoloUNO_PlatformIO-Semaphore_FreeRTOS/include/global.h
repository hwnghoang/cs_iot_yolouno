#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ---------------- Tài nguyên dùng chung ----------------
extern float glob_temperature;
extern float glob_humidity;
extern uint32_t glob_sample_id;

// ---------------- Semaphore ----------------
// Binary semaphore: task đọc cảm biến "give" mỗi khi có dữ liệu mới,
// task hiển thị "take" (bị block) cho tới khi có dữ liệu.
extern SemaphoreHandle_t xDataReadySem;
// Mutex: bảo vệ các biến toàn cục glob_temperature / glob_humidity / glob_sample_id.
extern SemaphoreHandle_t xDataMutex;
// Mutex: bảo vệ cổng Serial để các dòng log của nhiều task không bị chen lẫn.
extern SemaphoreHandle_t xSerialMutex;

// In log an toàn (có lấy xSerialMutex), kèm thời gian và tên task đang chạy.
void safeLog(const char *fmt, ...);

#endif
