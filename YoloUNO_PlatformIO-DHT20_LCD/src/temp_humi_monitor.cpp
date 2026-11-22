#include "temp_humi_monitor.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

namespace {
constexpr uint8_t DHT20_SDA_PIN = 11;
constexpr uint8_t DHT20_SCL_PIN = 12;

bool scanI2CBus() {
    bool dht20Found = false;

    Serial.println("Scanning I2C bus...");
    for (uint8_t address = 1; address < 0x7F; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("I2C device found at 0x");
            if (address < 0x10) {
                Serial.print('0');
            }
            Serial.println(address, HEX);
            if (address == dht20.getAddress()) {
                dht20Found = true;
            }
        }
    }

    if (!dht20Found) {
        Serial.println("DHT20 was not found at its expected address 0x38.");
    }
    return dht20Found;
}
}

void temp_humi_monitor(void *pvParameters){
    (void)pvParameters;

    // Give the serial monitor time to attach after the board resets.
    vTaskDelay(pdMS_TO_TICKS(2000));
    Serial.println("Starting DHT20 I2C check...");

    // This initializes Wire with the board's SDA/SCL pins and checks address 0x38.
    const bool sensorConnected = dht20.begin(DHT20_SDA_PIN, DHT20_SCL_PIN);
    const bool dht20Found = scanI2CBus();
    if (!sensorConnected || !dht20Found) {
        Serial.println("DHT20 initialization failed; check SDA, SCL, power, and wiring.");
        vTaskDelete(nullptr);
        return;
    }

    while (1){
        const int readStatus = dht20.read();
        if (readStatus != DHT20_OK) {
            Serial.print("DHT20 read failed, status: ");
            Serial.println(readStatus);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        const float temperature = dht20.getTemperature();
        const float humidity = dht20.getHumidity();
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("DHT20 returned invalid temperature or humidity.");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        glob_temperature = temperature;
        glob_humidity = humidity;

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println(" C");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}