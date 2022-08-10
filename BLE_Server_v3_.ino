/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Final ver authored by Gurjot as on 26-07-2022
    Final ver2.0 by as on 05/07/2022

*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>


//----------Preprocessor Definations-----------//
#define serviceUUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define smoke_gpio 15
#define soil_gpio 2
#define ONBOARD_LED 2



//----------BLE Pointers Declaration---------//
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic soilCharacteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_READ);
BLEDescriptor soilDescriptor(BLEUUID((uint16_t)0x2901));
BLECharacteristic smokeCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_READ);
BLEDescriptor smokeDescriptor(BLEUUID((uint16_t)0x2902));



//------------Global Var Decls-----------//
int soilval,smokeval;
double data_val[2];
bool deviceConnected = false;






//----------Callback Functions-----------//
class myServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
      deviceConnected = true;
      
    };
    void onDisconnect(BLEServer *pServer)
    {
      deviceConnected = false;
      delay(150);// give the bluetooth stack the chance to get things ready
      Serial.println("//// Server Disconnected ////");
      Serial.println("\n------------------------------------------------");
      Serial.println(" \t:::: Re-Advertising ON :::: ");
      Serial.println("------------------------------------------------\n");
      Serial.println("");
      BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
      pAdvertising->setScanResponse(true);
      pAdvertising->addServiceUUID(serviceUUID);
      pServer->startAdvertising(); // restart advertising

    };
};



//-- BLE Architecture = (Device)--> Server--> Service--> Charachteristics --> Descriptors //


void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Bluetooth LowEnergy Server!");
  pinMode(ONBOARD_LED, OUTPUT);
  analogReadResolution(10);
  digitalWrite(ONBOARD_LED, HIGH);

  // creating server
  BLEDevice::init("");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new myServerCallbacks());

  // creating service
  BLEService *pService = pServer->createService(serviceUUID);

  // creating 2 characteristics
  pService->addCharacteristic(&soilCharacteristics);
  soilDescriptor.setValue("Soil Moisture");
  soilCharacteristics.addDescriptor(new BLE2902());

  pService->addCharacteristic(&smokeCharacteristics);
  smokeDescriptor.setValue("Smoke Density");
  smokeCharacteristics.addDescriptor(new BLE2902());

  //start service, start advertising service through getAdvertising->start()
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(serviceUUID);
  pAdvertising->setScanResponse(false);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}



void loop()
{
  BLE();
  delay(50);
}



void BLE()
{
  if (deviceConnected)
  {
    soilval = analogRead(soil_gpio);
    smokeval = analogRead(smoke_gpio);
    data_val[0] = soilval/1024.0;
    data_val[1] = smokeval/1024.0;
    

    static char soilvall[8];
    dtostrf(soilval, 4, 2, soilvall);
    static char smokevall[8];
    dtostrf(smokeval, 4, 2, smokevall);

    soilCharacteristics.setValue(soilvall);
    //soilCharacteristics.notify();
    
    //Normalise Data Values [0,1]
    Serial.print(" - Soil Moisture: ");
    Serial.println(data_val[0]);

    smokeCharacteristics.setValue(smokevall);
   // smokeCharacteristics.notify();
    Serial.print(" - Smoke Density: ");
    Serial.println(data_val[1]);
    Serial.println("Connection Status : True");
    

    // Serial.println(deviceConnected);
  }

}
