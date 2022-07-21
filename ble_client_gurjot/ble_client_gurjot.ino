/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
//#include "BLEScan.h"

// The remote service we wish to connect to.
#define bleServerName "SensorNodeESP32"
static BLEUUID serviceUUID_1("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
// The characteristic of the remote service we are interested in.
static BLEUUID    soilcharUUID_1("f78ebbff-c8b7-4107-93de-889a6a06d408");
static BLEUUID    smokecharUUID_1("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

char* soilChar;
char* smokeChar;

const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
boolean newsoil = false;
boolean newsmoke = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* soilCharacteristic;
static BLERemoteCharacteristic* smokeCharacteristic;


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer(BLEAddress pAddress) {
    Serial.print("Forming a connection to ");
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(pAddress);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID_1);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID_1.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    soilCharacteristic = pRemoteService->getCharacteristic(soilcharUUID_1);
    smokeCharacteristic = pRemoteService->getCharacteristic(smokecharUUID_1);
    
    if (soilCharacteristic == nullptr || smokeCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
     
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    soilCharacteristic->registerForNotify(soilNotifyCallback);
    smokeCharacteristic->registerForNotify(smokeNotifyCallback);
    
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
   if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
}; // MyAdvertisedDeviceCallbacks

 
//When the BLE Server sends a new smoke reading with the notify property
static void smokeNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  smokeChar = (char*)pData;
  newsmoke = true;
}

//When the BLE Server sends a new soil reading with the notify property
static void soilNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  soilChar = (char*)pData;
  newsoil = true;
  Serial.print(newsoil);
}
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      soilCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      smokeCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  //if new temperature readings are available, print in the OLED
  if (newsoil && newsmoke){
    newsoil = false;
    newsmoke = false;
    Serial.print(" Soil Moisture:");
    Serial.println(soilChar); 
    
    Serial.print(" Smoke Density:");
    Serial.println(smokeChar); 
  
  }
  delay(1000); // Delay a second between loops.





}
