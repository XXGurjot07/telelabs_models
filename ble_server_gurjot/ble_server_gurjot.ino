/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

BLECharacteristic *pCharacteristic;

#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
BLECharacteristic soilCharacteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor soilDescriptor(BLEUUID((uint16_t)0x2901));
BLECharacteristic smokeCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor smokeDescriptor(BLEUUID((uint16_t)0x2902));


#define smoke_gpio 15
#define soil_gpio 2
int soilval;
int smokeval;

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

bool deviceConnected = false;
int soil_sens = 0;
int smoke_sens = 0;

class myServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
    };
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
    };
  };

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  
  //creating server
  BLEDevice::init("SensorNodeESP32");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new myServerCallbacks());
  
  //creating service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //creating 2 characteristics
  pService->addCharacteristic(&soilCharacteristics);
  soilDescriptor.setValue("Soil Moisture");
  soilCharacteristics.addDescriptor(new BLE2902());

  pService->addCharacteristic(&smokeCharacteristics);
  smokeDescriptor.setValue("Smoke Density");
  smokeCharacteristics.addDescriptor(new BLE2902());


  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {

      analogReadResolution(10);
      soilval = analogRead(soil_gpio);
      smokeval = analogRead(smoke_gpio);
      
      static char soilvall[8];
      dtostrf(soilval, 4, 2, soilvall);
      static char smokevall[8];
      dtostrf(smokeval, 4, 2, smokevall);
      
      
      soilCharacteristics.setValue(soilvall);
      soilCharacteristics.notify();   
      Serial.print(" - Soil Moisture: ");
      Serial.println(soilval);

      
      smokeCharacteristics.setValue(smokevall);
      smokeCharacteristics.notify();   
      Serial.print(" - Smoke Density: ");
      Serial.println(smokeval);
      
      
      lastTime = millis();
    }
  }
}
