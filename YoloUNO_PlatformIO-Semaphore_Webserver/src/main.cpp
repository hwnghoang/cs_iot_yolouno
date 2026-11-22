// Bài 5 - Thiết kế webserver trên YoloUNO (ESP32-S3, FreeRTOS, Arduino)
//
// Task:
//   - TempHumi  – priority 2 – đọc DHT20 mỗi 2 s, ghi biến toàn cục (bảo vệ bằng mutex)
//   - WebServer – priority 1 – phát Wi-Fi AP / kết nối STA, phục vụ trang web và API
//
// Cách dùng: kết nối điện thoại/laptop vào Wi-Fi "YoloUNO_Lab5" (mật khẩu 12345678),
// mở trình duyệt tới http://192.168.4.1
#include "global.h"
#include "mainserver.h"
#include "temp_humi_monitor.h"

void setup() {
    Serial.begin(115200);
    delay(2000);   // chờ Serial Monitor kết nối (USB CDC)
    Serial.println("\n===== Lab 5: Webserver on YoloUNO =====");

    xDataMutex = xSemaphoreCreateMutex();
    xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    if (xDataMutex == NULL || xBinarySemaphoreInternet == NULL) {
        Serial.println("Semaphore creation failed (out of heap)");
        while (1) { delay(1000); }
    }

    xTaskCreate(temp_humi_monitor, "TempHumi",  4096, NULL, 2, NULL);
    xTaskCreate(main_server_task,  "WebServer", 8192, NULL, 1, NULL);
}

void loop() {
    // Trống: mọi công việc do các task FreeRTOS đảm nhiệm.
    vTaskDelay(portMAX_DELAY);
}
