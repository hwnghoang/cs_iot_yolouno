// Week 8 - Class 2: Counting semaphore demo (ESP32-S3, Arduino framework)
// Every BOOT-button press gives once; the LED task blinks once per Take
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define BOOT_BUTTON_GPIO    GPIO_NUM_0    // ESP32-S3 BOOT button (active-low)
#define LED_GPIO            GPIO_NUM_48   // LED on GPIO48 (change if your board differs)
#define SEMAPHORE_MAX_COUNT 10

SemaphoreHandle_t xCountingSemaphore;

void task_monitor_button(void *pvParameters) {
  while (1) {
    if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {          // pressed (active-low)
      vTaskDelay(pdMS_TO_TICKS(50));                      // simple debounce
      if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        // One Give per press (fails once the count reaches SEMAPHORE_MAX_COUNT)
        if (xSemaphoreGive(xCountingSemaphore) == pdTRUE)
          Serial.println("Give -> OK");
        else
          Serial.println("Give ignored (semaphore full)");
        // Wait for release: 1 press = 1 Give
        while (gpio_get_level(BOOT_BUTTON_GPIO) == 0)
          vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// void task_blink_led(void *pvParameters) {
//   while (1) {
//     // Wait for one token = one press
//     if (xSemaphoreTake(xCountingSemaphore, portMAX_DELAY) == pdTRUE) {
//       Serial.printf("Blink! remaining = %u\n",
//                     (unsigned)uxSemaphoreGetCount(xCountingSemaphore));
//       gpio_set_level(LED_GPIO, 1);                        // ONE blink per Take
//       vTaskDelay(pdMS_TO_TICKS(200));
//       gpio_set_level(LED_GPIO, 0);
//       vTaskDelay(pdMS_TO_TICKS(200));
//     }
//   }
// }

void task_blink_led(void *pvParameters) {
  while (1) {
    // Blocked here (0% CPU) until the button task gives
    if (xSemaphoreTake(xCountingSemaphore, portMAX_DELAY) == pdTRUE) {
      for (int i = 0; i < 3; i++) {                       // blink 3 times
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
        // Serial.printf("Blink! remaining = %u\n",
        //   (unsigned)uxSemaphoreGetCount(xCountingSemaphore));
      }
      Serial.printf("Blink! remaining = %u\n",
        (unsigned)uxSemaphoreGetCount(xCountingSemaphore));
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_GPIO, INPUT_PULLUP);
  pinMode(LED_GPIO, OUTPUT);

  // max count = SEMAPHORE_MAX_COUNT, initial count = 0
  xCountingSemaphore = xSemaphoreCreateCounting(SEMAPHORE_MAX_COUNT, 0);
  if (xCountingSemaphore == NULL) {
    Serial.println("Semaphore creation failed (out of heap)");
    return;
  }

  xTaskCreate(task_monitor_button, "MonitorButton", 4096, NULL, 10, NULL);
  xTaskCreate(task_blink_led,      "BlinkLED",      4096, NULL,  5, NULL);
}

void loop() {
  // empty: the FreeRTOS tasks do all the work
}