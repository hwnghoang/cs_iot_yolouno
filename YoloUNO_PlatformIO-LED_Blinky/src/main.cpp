/*
 * Bai 2 - LED Blinky tren YoloUNO (ESP32-S3)
 * Chan LED : GPIO 48 (LED tren mach YoloUNO)
 * Chu ky   : sang 1000 ms, tat 1000 ms (chu ky day du 2000 ms)
 * Moi lan LED doi trang thai se in log ra Serial Monitor (115200 baud).
 */
#include <Arduino.h>

#define LED_PIN       GPIO_NUM_48   // Chan GPIO noi voi LED
#define BLINK_MS      1000          // Thoi gian sang / tat (ms)

// Task FreeRTOS dieu khien LED: dung vTaskDelay() thay vi delay()
// de CPU duoc nhuong cho cac task khac trong luc cho.
void TaskLEDControl(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);           // Cau hinh chan LED la ngo ra
  bool ledOn = false;                 // Trang thai hien tai cua LED
  uint32_t toggleCount = 0;           // So lan doi trang thai

  while (1) {
    ledOn = !ledOn;                               // Dao trang thai LED
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);    // Xuat ra chan GPIO
    toggleCount++;

    // In trang thai LED moi lan thay doi
    Serial.printf("[%8lu ms] #%lu LED (GPIO %d): %s\n",
                  millis(), toggleCount, LED_PIN, ledOn ? "ON" : "OFF");

    vTaskDelay(pdMS_TO_TICKS(BLINK_MS));          // Doi BLINK_MS (doi ms -> tick)
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Cho USB CDC san sang de khong mat log dau tien
  Serial.println("=== Bai 2: LED Blinky tren YoloUNO ===");
  Serial.printf("LED pin: GPIO %d, chu ky: %d ms ON / %d ms OFF\n",
                LED_PIN, BLINK_MS, BLINK_MS);

  // Tao task dieu khien LED: stack 2048 byte, do uu tien 2
  xTaskCreate(TaskLEDControl, "LED Control", 2048, NULL, 2, NULL);
}

void loop() {
  // Khong lam gi: viec nhap nhay LED do task FreeRTOS dam nhan
  vTaskDelay(pdMS_TO_TICKS(1000));
}
