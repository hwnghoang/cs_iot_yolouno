// Bài 1 - YoloUNO đọc DHT20 và publish telemetry lên CoreIOT qua MQTT
//
// Kết nối DHT20: SDA GPIO11, SCL GPIO12, địa chỉ I2C 0x38
// MQTT  : host COREIOT_HOST:1883, username = ACCESS_TOKEN (password trống)
// Topic : v1/devices/me/telemetry
// Payload: {"temperature": 30.5, "humidity": 65.0}
//
// Task:
//   - Network   – priority 2 – giữ kết nối Wi-Fi + MQTT, gọi mqtt.loop()
//   - Telemetry – priority 1 – mỗi 5 s đọc DHT20 và publish lên CoreIOT
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "DHT20.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Missing include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill in your values"
#endif

constexpr uint8_t DHT20_SDA_PIN = 11;
constexpr uint8_t DHT20_SCL_PIN = 12;
constexpr uint32_t PUBLISH_PERIOD_MS = 5000;
const char *TELEMETRY_TOPIC = "v1/devices/me/telemetry";

DHT20 dht20;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
// Mutex bảo vệ đối tượng mqtt (PubSubClient không an toàn khi 2 task cùng gọi)
SemaphoreHandle_t xMqttMutex;

// ---------- Wi-Fi ----------
static void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    Serial.printf("\n[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
}

// ---------- MQTT ----------
static void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.printf("[MQTT] Connecting to %s:%d ... ", COREIOT_HOST, COREIOT_PORT);
        // clientId tùy ý; CoreIOT xác thực bằng username = access token
        String clientId = "YoloUNO-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        if (mqtt.connect(clientId.c_str(), COREIOT_ACCESS_TOKEN, NULL)) {
            Serial.println("OK");
        } else {
            // state 5 = not authorized -> sai access token
            Serial.printf("failed, state=%d, retry in 3 s\n", mqtt.state());
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}

// Task Network: duy trì kết nối Wi-Fi và MQTT
void task_network(void *pvParameters) {
    while (1) {
        connectWiFi();
        if (xSemaphoreTake(xMqttMutex, portMAX_DELAY) == pdTRUE) {
            connectMQTT();
            mqtt.loop();                       // xử lý keep-alive của MQTT
            xSemaphoreGive(xMqttMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Task Telemetry: đọc DHT20 và publish dữ liệu
void task_telemetry(void *pvParameters) {
    // Khởi tạo I2C với chân SDA/SCL của YoloUNO, sau đó mới khởi tạo DHT20.
    // (Tương thích cả thư viện DHT20 cũ lẫn bản mới chỉ có begin() không tham số.)
    Wire.begin(DHT20_SDA_PIN, DHT20_SCL_PIN);
    const bool sensorOk = dht20.begin() && dht20.isConnected();
    Serial.println(sensorOk ? "[DHT20] Sensor found at 0x38"
                            : "[DHT20] Sensor NOT found, check wiring (SDA 11, SCL 12)");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));

        if (!sensorOk || dht20.read() != DHT20_OK) {
            Serial.println("[DHT20] Read failed, skip this cycle");
            continue;
        }
        const float temperature = dht20.getTemperature();
        const float humidity = dht20.getHumidity();

        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);

        if (xSemaphoreTake(xMqttMutex, portMAX_DELAY) == pdTRUE) {
            const bool ok = mqtt.connected() && mqtt.publish(TELEMETRY_TOPIC, payload);
            xSemaphoreGive(xMqttMutex);
            Serial.printf("[MQTT] publish %s %s -> %s\n", TELEMETRY_TOPIC, payload, ok ? "OK" : "FAILED");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);   // chờ Serial Monitor (USB CDC)
    Serial.println("\n===== Lab 1: YoloUNO -> CoreIOT telemetry =====");

    mqtt.setServer(COREIOT_HOST, COREIOT_PORT);
    xMqttMutex = xSemaphoreCreateMutex();

    xTaskCreate(task_network,   "Network",   4096, NULL, 2, NULL);
    xTaskCreate(task_telemetry, "Telemetry", 4096, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
