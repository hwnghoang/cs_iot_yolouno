// SDA: GPIO 11   SCL: GPIO 12   Địa chỉ I2C DHT20: 0x38
#include "temp_humi_monitor.h"

DHT20 dht20;

namespace {
constexpr uint8_t DHT20_SDA_PIN = 11;
constexpr uint8_t DHT20_SCL_PIN = 12;
constexpr TickType_t SENSOR_PERIOD = pdMS_TO_TICKS(2000);
}

void temp_humi_monitor(void *pvParameters) {
    (void)pvParameters;

    // Nếu không có cảm biến thật thì dùng dữ liệu giả lập để vẫn kiểm tra được webserver.
    const bool sensorOk = dht20.begin(DHT20_SDA_PIN, DHT20_SCL_PIN) && dht20.isConnected();
    Serial.println(sensorOk ? "[DHT20] Sensor found at 0x38"
                            : "[DHT20] Sensor NOT found -> using SIMULATED data");

    while (1) {
        float t, h;
        if (sensorOk && dht20.read() == DHT20_OK) {
            t = dht20.getTemperature();
            h = dht20.getHumidity();
        } else {
            t = 25.0f + (float)random(-20, 21) / 10.0f;   // 23.0 .. 27.0 C
            h = 60.0f + (float)random(-50, 51) / 10.0f;   // 55.0 .. 65.0 %
        }

        // Ghi dữ liệu dùng chung dưới sự bảo vệ của mutex
        if (xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE) {
            glob_temperature = t;
            glob_humidity = h;
            xSemaphoreGive(xDataMutex);
        }
        Serial.printf("[DHT20] Temperature: %.1f C  Humidity: %.1f %%\n", t, h);

        vTaskDelay(SENSOR_PERIOD);
    }
}
