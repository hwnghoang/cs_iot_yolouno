#include "global.h"
#include <stdarg.h>

float glob_temperature = NAN;
float glob_humidity = NAN;
uint32_t glob_sample_id = 0;

SemaphoreHandle_t xDataReadySem = NULL;
SemaphoreHandle_t xDataMutex = NULL;
SemaphoreHandle_t xSerialMutex = NULL;

void safeLog(const char *fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Chỉ một task được ghi Serial tại một thời điểm.
    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.printf("[%7lu ms][%-10s] %s\n",
                      (unsigned long)millis(), pcTaskGetName(NULL), buf);
        xSemaphoreGive(xSerialMutex);
    }
}
