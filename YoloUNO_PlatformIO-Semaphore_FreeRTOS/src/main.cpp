// Bài 4 - Lập trình Semaphore trên YoloUNO (ESP32-S3, FreeRTOS, Arduino)
//
// Loại semaphore sử dụng:
//   - Binary semaphore (xDataReadySem): SensorRead báo cho Display có dữ liệu mới.
//   - Mutex (xDataMutex)  : bảo vệ biến toàn cục nhiệt độ / độ ẩm.
//   - Mutex (xSerialMutex): bảo vệ cổng Serial, tránh log bị chen lẫn.
//
// Các task:
//   - SensorRead – priority 3 – đọc DHT20 mỗi 3 s, ghi biến toàn cục, give xDataReadySem
//   - Display    – priority 2 – take xDataReadySem, đọc biến toàn cục, in kết quả
//   - Heartbeat  – priority 1 – mỗi 1 s take/give xDataMutex để đọc mẫu mới nhất
#include "global.h"
#include "temp_humi_monitor.h"

void setup() {
    Serial.begin(115200);
    delay(2000);   // chờ Serial Monitor kết nối (USB CDC)
    Serial.println("\n===== Lab 4: FreeRTOS Semaphore demo =====");

    // ---- Khởi tạo semaphore ----
    xDataReadySem = xSemaphoreCreateBinary();   // khởi tạo ở trạng thái "empty" (0)
    xDataMutex    = xSemaphoreCreateMutex();    // khởi tạo ở trạng thái "available"
    xSerialMutex  = xSemaphoreCreateMutex();

    if (xDataReadySem == NULL || xDataMutex == NULL || xSerialMutex == NULL) {
        Serial.println("Semaphore creation failed (out of heap)");
        while (1) { delay(1000); }
    }
    Serial.println("Semaphores created: xDataReadySem (binary), xDataMutex, xSerialMutex");

    // ---- Tạo task ----
    xTaskCreate(task_sensor_reader, "SensorRead", 4096, NULL, 3, NULL);
    xTaskCreate(task_display,       "Display",    4096, NULL, 2, NULL);
    xTaskCreate(task_heartbeat,     "Heartbeat",  4096, NULL, 1, NULL);
}

void loop() {
    // Trống: mọi công việc do các task FreeRTOS đảm nhiệm.
    vTaskDelay(portMAX_DELAY);
}
