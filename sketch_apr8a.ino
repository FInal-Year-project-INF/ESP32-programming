#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// === CONFIGURATION ===
#define ONE_WIRE_BUS 23
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-ab12-cd34-ef56-abcdef123456"

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Client Connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Client Disconnected");
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting setup...");

  // Init temp sensor
  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    Serial.println("No DS18B20 sensors found!");
  } else {
    sensors.getAddress(tempDeviceAddress, 0);
    Serial.print("DS18B20 found at address: ");
    for (uint8_t i = 0; i < 8; i++) {
      if (tempDeviceAddress[i] < 16) Serial.print("0");
      Serial.print(tempDeviceAddress[i], HEX);
    }
    Serial.println();
  }

  // Init BLE
  BLEDevice::init("ESP32-Thermo");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE Service started, now advertising...");
}

void loop() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read from DS18B20 sensor!");
  } else {
    Serial.print("🌡 Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    if (deviceConnected) {
      char buffer[10];
      dtostrf(temperature, 4, 2, buffer);
      pCharacteristic->setValue(buffer);
      pCharacteristic->notify();
      Serial.println("Sent temp via BLE: " + String(buffer));
    } else {
      Serial.println(" No BLE client connected.");
    }
  }

  delay(2000); // Every 2 seconds
}
