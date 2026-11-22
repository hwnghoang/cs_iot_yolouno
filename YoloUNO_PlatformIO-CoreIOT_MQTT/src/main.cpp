// Bài 1 - YoloUNO đọc DHT20 và publish telemetry lên CoreIOT qua MQTT
//
// Kết nối DHT20: SDA GPIO11, SCL GPIO12, địa chỉ I2C 0x38
// MQTT  : host COREIOT_HOST:1883, username = ACCESS_TOKEN (password trống)
// Topic : v1/devices/me/telemetry
// Payload: {"temperature": 30.5, "humidity": 65.0}
//
// Điều khiển LED NeoPixel (GPIO45) từ dashboard bằng RPC:
//   Nhận   : v1/devices/me/rpc/request/{id}   {"method":"setState","params":true}
//   Trả lời: v1/devices/me/rpc/response/{id}  true / false
//   Method hỗ trợ: getState / setState (widget Switch), getValue / setValue
//
// Task:
//   - Network   – priority 2 – giữ kết nối Wi-Fi + MQTT, gọi mqtt.loop() (nhận RPC)
//   - Telemetry – priority 1 – mỗi 5 s đọc DHT20 và publish lên CoreIOT
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
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
const char *ATTRIBUTES_TOPIC = "v1/devices/me/attributes";
const char *RPC_REQUEST_TOPIC = "v1/devices/me/rpc/request/+";
const char *RPC_REQUEST_PREFIX = "v1/devices/me/rpc/request/";

constexpr uint8_t NEO_PIN = 45;              // LED RGB NeoPixel trên board YoloUNO

DHT20 dht20;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_NeoPixel neoLed(1, NEO_PIN, NEO_GRB + NEO_KHZ800);
// Mutex bảo vệ đối tượng mqtt (PubSubClient không an toàn khi 2 task cùng gọi)
SemaphoreHandle_t xMqttMutex;

volatile bool ledState = false;

// ---------- LED NeoPixel ----------
static void setLed(bool on) {
    ledState = on;
    neoLed.setPixelColor(0, on ? neoLed.Color(0, 80, 255) : 0);   // xanh dương khi ON
    neoLed.show();
    Serial.printf("[LED] NeoPixel -> %s\n", on ? "ON" : "OFF");
}

// ---------- RPC từ CoreIOT ----------
// Hàm này được gọi bên trong mqtt.loop() (task Network đang giữ xMqttMutex),
// nên publish trực tiếp, KHÔNG take mutex lần nữa.
static void onMqttMessage(char *topic, byte *payload, unsigned int length) {
    String t(topic);
    if (!t.startsWith(RPC_REQUEST_PREFIX)) return;
    String requestId = t.substring(strlen(RPC_REQUEST_PREFIX));

    JsonDocument req;
    if (deserializeJson(req, payload, length)) {
        Serial.println("[RPC] Invalid JSON");
        return;
    }
    const char *method = req["method"] | "";
    Serial.printf("[RPC] #%s method=%s params=", requestId.c_str(), method);
    serializeJson(req["params"], Serial);
    Serial.println();

    if (!strcmp(method, "setState") || !strcmp(method, "setValue")) {
        JsonVariant p = req["params"];
        // params có thể là true/false, "true"/"false" hoặc 1/0 tùy widget
        bool on = p.is<bool>() ? p.as<bool>()
                : p.is<const char *>() ? !strcmp(p.as<const char *>(), "true")
                : p.as<int>() != 0;
        setLed(on);
        // Cập nhật trạng thái LED lên CoreIOT dưới dạng attribute
        mqtt.publish(ATTRIBUTES_TOPIC, on ? "{\"ledState\":true}" : "{\"ledState\":false}");
    } else if (strcmp(method, "getState") && strcmp(method, "getValue")) {
        Serial.printf("[RPC] Unknown method: %s\n", method);
    }

    // Trả lời RPC (cả get và set) bằng trạng thái hiện tại để widget hết "Request timeout"
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    mqtt.publish(responseTopic.c_str(), ledState ? "true" : "false");
}

// ---------- Wi-Fi ----------
static void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        if (millis() - start > 15000) {
            // Thường do: sai SSID/mật khẩu, hoặc mạng 5 GHz (ESP32 chỉ hỗ trợ 2.4 GHz)
            Serial.printf("\n[WiFi] Timeout (status=%d). Check SSID/password; network must be 2.4 GHz. Retrying...\n",
                          WiFi.status());
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            start = millis();
        }
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
            // Đăng ký nhận lệnh RPC từ dashboard (phải subscribe lại sau mỗi lần kết nối)
            mqtt.subscribe(RPC_REQUEST_TOPIC);
            Serial.printf("[MQTT] Subscribed %s\n", RPC_REQUEST_TOPIC);
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
            const bool wifiOk = WiFi.status() == WL_CONNECTED;
            const bool mqttOk = mqtt.connected();
            const bool ok = wifiOk && mqttOk && mqtt.publish(TELEMETRY_TOPIC, payload);
            xSemaphoreGive(xMqttMutex);
            Serial.printf("[MQTT] publish %s %s -> %s%s\n", TELEMETRY_TOPIC, payload,
                          ok ? "OK" : "FAILED",
                          ok ? "" : (!wifiOk ? " (WiFi not connected)"
                                   : !mqttOk ? " (MQTT not connected)" : " (publish error)"));
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);   // chờ Serial Monitor (USB CDC)
    Serial.println("\n===== Lab 1: YoloUNO -> CoreIOT telemetry =====");

    neoLed.begin();
    neoLed.setBrightness(80);
    setLed(false);

    mqtt.setServer(COREIOT_HOST, COREIOT_PORT);
    mqtt.setCallback(onMqttMessage);
    xMqttMutex = xSemaphoreCreateMutex();

    xTaskCreate(task_network,   "Network",   4096, NULL, 2, NULL);
    xTaskCreate(task_telemetry, "Telemetry", 4096, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
