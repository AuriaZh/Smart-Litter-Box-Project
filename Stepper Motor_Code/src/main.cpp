#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <AccelStepper.h>

// **Motor 引脚**
#define MOTOR_PIN1 7
#define MOTOR_PIN2 21
#define MOTOR_PIN3 6
#define MOTOR_PIN4 5
#define BUTTON_PIN 9 // 归零按钮 GPIO

AccelStepper stepper(AccelStepper::FULL4WIRE, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);

static BLEUUID serviceUUID("47786ff2-702b-4457-8d4c-bbce01db278c");
static BLEUUID charUUID("2a451022-e0c3-459b-8694-20c0a6aab167");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
bool shouldReset = false;  // 是否需要复位指针

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    String receivedData = String((char*)pData);
    receivedData.trim();
    float receivedWeight = receivedData.toFloat();

    Serial.print("📡 Received Weight: ");
    Serial.print(receivedWeight, 2);
    Serial.println(" kg");

    if (receivedWeight > 0.1) { // **收到体重数据（猫进入）**
        Serial.println("🐱 Cat Detected! Rotating Stepper Motor...");
        stepper.move(200);
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("❌ Disconnected.");
  }
};

bool connectToServer() {
    Serial.println("🔍 Connecting to LoadCell...");
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println("✅ Connected!");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.println("❌ Service Not Found!");
      return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.println("❌ Characteristic Not Found!");
      return false;
    }

    pRemoteCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Motor BLE Client Starting...");
    BLEDevice::init("Motor");
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->start(5, false);

    stepper.setMaxSpeed(500);
    stepper.setAcceleration(100);

    Serial.println("🔄 Ready to receive weight data!");
}

void loop() {
    if (doConnect) connectToServer();
    stepper.run();

    // **监听按钮（复位 Motor 位置）**
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("🔄 Resetting Motor...");
        stepper.move(-stepper.currentPosition());  // 回到 0 位置
        shouldReset = true;
        delay(1000);  // 防止抖动
    }

    if (shouldReset && stepper.distanceToGo() == 0) {
        shouldReset = false;
        Serial.println("✅ Motor Reset Complete.");
    }
}
