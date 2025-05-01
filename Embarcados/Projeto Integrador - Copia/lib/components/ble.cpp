#include "ble.hpp"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

// UUIDs (podem ser personalizados)
#define SERVICE_UUID           "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_TX_UUID "12345678-1234-1234-1234-1234567890ac" // ESP32 -> App
#define CHARACTERISTIC_RX_UUID "12345678-1234-1234-1234-1234567890ad" // App -> ESP32

bool deviceConnected = false;

// Simulação do vibracall (pino 2)
const int vibracallPin = 2;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        const uint8_t* value = pCharacteristic->getData();
        size_t length = pCharacteristic->getLength();
    
        if (*value == (uint8_t)1) {
          digitalWrite(vibracallPin, HIGH);
        } else {
        digitalWrite(vibracallPin, LOW);
        }
    }
};


void ble_config(BLECharacteristic **txCharacteristic) {
    Serial.begin(115200);
    pinMode(vibracallPin, OUTPUT);
    digitalWrite(vibracallPin, LOW);

    BLEDevice::init("ESP32-ControleGeometria");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    *txCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_TX_UUID,
                        BLECharacteristic::PROPERTY_NOTIFY
                        );


    (*txCharacteristic)->addDescriptor(new BLE2902());

    BLECharacteristic *rxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_RX_UUID,
                                            BLECharacteristic::PROPERTY_WRITE
                                            );
    rxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();

    Serial.println("Esperando conexão BLE...");
}