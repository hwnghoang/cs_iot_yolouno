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

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    while (1){
        /* code */
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }

        //Update global variables for temperature and humidity
        glob_temperature = temperature;
        glob_humidity = humidity;

        // Print the results
        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        vTaskDelay(5000);
    }
    
}