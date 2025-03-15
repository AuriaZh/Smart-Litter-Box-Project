#include <Arduino.h>
#include <HX711.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define HX711_DT 6  // HX711 DT 连接 ESP32 GPIO6
#define HX711_SCK 7 // HX711 SCK 连接 ESP32 GPIO7
#define BUTTON_PIN 9 // 归零按钮 GPIO

HX711 scale;
float calibration_factor = 1000.0;  // 需要根据你的 Load Cell 校准
float currentWeight = 0.0;

// **BLE UUID**
#define SERVICE_UUID        "47786ff2-702b-4457-8d4c-bbce01db278c"
#define CHARACTERISTIC_UUID "2a451022-e0c3-459b-8694-20c0a6aab167"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println("🐱 Load Cell BLE Server Starting...");
    
    // **初始化 Load Cell**
    scale.begin(HX711_DT, HX711_SCK);
    scale.set_scale(calibration_factor);
    scale.tare();  // 去皮

    // **初始化 BLE**
    BLEDevice::init("LoadCell");
    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_NOTIFY);
    
    pCharacteristic->setValue("0");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    BLEDevice::startAdvertising();

    Serial.println("🚀 BLE Service Started!");
}

void loop() {
    if (scale.is_ready()) {
        currentWeight = scale.get_units(5); // 取 5 次平均值
        Serial.print("⚖️  Stable Weight: ");
        Serial.print(currentWeight, 2);
        Serial.println(" kg");

        // 发送数据
        String weightStr = String(currentWeight, 2);
        pCharacteristic->setValue(weightStr.c_str());
        pCharacteristic->notify();
    } else {
        Serial.println("❌ HX711 NOT RESPONDING!");
    }

    // **监听按钮（归零 tare）**
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("🔄 Resetting Scale...");
        scale.tare();
        delay(1000);  // 防止抖动
    }

    delay(500);
}
