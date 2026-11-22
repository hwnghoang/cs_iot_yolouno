#include "global.h"

float glob_temperature = NAN;
float glob_humidity = NAN;

SemaphoreHandle_t xDataMutex = NULL;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;

bool isWifiConnected = false;
