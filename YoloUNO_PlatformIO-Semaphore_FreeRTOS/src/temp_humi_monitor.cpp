// SDA: GPIO 11   SCL: GPIO 12   Địa chỉ I2C DHT20: 0x38
#include <Wire.h>
#include "temp_humi_monitor.h"

DHT20 dht20;

namespace {
constexpr uint8_t DHT20_SDA_PIN = 11;
constexpr uint8_t DHT20_SCL_PIN = 12;
constexpr TickType_t SENSOR_PERIOD = pdMS_TO_TICKS(3000);
constexpr TickType_t HEARTBEAT_PERIOD = pdMS_TO_TICKS(1000);
}

// ============================================================
// Task 1: SensorRead - priority 3 - đọc DHT20, cập nhật biến toàn cục
// ============================================================
void task_sensor_reader(void *pvParameters) {
    (void)pvParameters;

    // Nếu không có cảm biến thật, dùng dữ liệu giả lập để vẫn minh họa được semaphore.
    // Khởi tạo I2C với chân SDA/SCL của YoloUNO, sau đó mới khởi tạo DHT20.
    // (Tương thích cả thư viện DHT20 cũ lẫn bản mới chỉ có begin() không tham số.)
    Wire.begin(DHT20_SDA_PIN, DHT20_SCL_PIN);
    const bool sensorOk = dht20.begin() && dht20.isConnected();
    safeLog(sensorOk ? "DHT20 found at 0x38" : "DHT20 NOT found -> using SIMULATED data");

    while (1) {
        float t, h;
        if (sensorOk && dht20.read() == DHT20_OK) {
            t = dht20.getTemperature();
            h = dht20.getHumidity();
        } else {
            t = 25.0f + (float)random(-20, 21) / 10.0f;   // 23.0 .. 27.0 C
            h = 60.0f + (float)random(-50, 51) / 10.0f;   // 55.0 .. 65.0 %
        }

        // --- Vùng găng: ghi biến toàn cục, được bảo vệ bởi mutex ---
        safeLog("TAKE  xDataMutex   (waiting...)");
        if (xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE) {
            safeLog("TAKE  xDataMutex   OK -> write shared data");
            glob_temperature = t;
            glob_humidity = h;
            glob_sample_id++;
            const uint32_t id = glob_sample_id;
            xSemaphoreGive(xDataMutex);
            safeLog("GIVE  xDataMutex   (sample #%lu: %.1f C, %.1f %%)",
                    (unsigned long)id, t, h);
        }

        // --- Báo cho task Display biết đã có dữ liệu mới ---
        safeLog("GIVE  xDataReadySem -> notify Display");
        xSemaphoreGive(xDataReadySem);

        vTaskDelay(SENSOR_PERIOD);
    }
}

// ============================================================
// Task 2: Display - priority 2 - chờ dữ liệu mới rồi hiển thị
// ============================================================
void task_display(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        safeLog("TAKE  xDataReadySem (blocked, waiting for new data)");
        // Block vô thời hạn cho tới khi SensorRead give semaphore.
        if (xSemaphoreTake(xDataReadySem, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        safeLog("TAKE  xDataReadySem OK -> new data available");

        float t, h;
        uint32_t id;
        safeLog("TAKE  xDataMutex   (waiting...)");
        if (xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE) {
            // Sao chép nhanh rồi trả mutex ngay, không giữ mutex khi in log.
            t = glob_temperature;
            h = glob_humidity;
            id = glob_sample_id;
            xSemaphoreGive(xDataMutex);
            safeLog("GIVE  xDataMutex   (copied sample #%lu)", (unsigned long)id);

            safeLog(">>> DISPLAY #%lu: Temperature = %.1f C | Humidity = %.1f %%",
                    (unsigned long)id, t, h);
        }
    }
}

// ============================================================
// Task 3: Heartbeat - priority 1 - đọc định kỳ biến toàn cục (cạnh tranh mutex)
// ============================================================
void task_heartbeat(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        // Chờ tối đa 100 ms; nếu mutex đang bị giữ thì báo timeout.
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            const uint32_t id = glob_sample_id;
            const float t = glob_temperature;
            xSemaphoreGive(xDataMutex);
            safeLog("heartbeat: TAKE/GIVE xDataMutex, last sample #%lu (%.1f C)",
                    (unsigned long)id, t);
        } else {
            safeLog("heartbeat: xDataMutex busy (timeout)");
        }
        vTaskDelay(HEARTBEAT_PERIOD);
    }
}
