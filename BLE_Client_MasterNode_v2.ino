// Authored by Gurjot Singh as on 05/08/22

#include <WiFi.h>
#include <HTTPClient.h>
#include "BLEDevice.h"
#include <BLEUtils.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#define ONBOARD_LED 2


//---------------------------Wifi Details--------------------------//
const char *wifi_ssid = "Gurjot10A";
const char *wifi_password = "12345678";


//---------------------------HTTP Post stuff---------------------//
// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://192.168.43.226:8000/auxiliary-requests";


//---------------------------BLE pointers setup----------------------//
// The remote service we wish to connect to.
static BLEUUID serviceUUID_1("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
// The characteristic of the remote service we are interested in.
static BLEUUID soilcharUUID_1("f78ebbff-c8b7-4107-93de-889a6a06d408");
static BLEUUID    smokecharUUID_1("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");
//Device connection status
static boolean doConnect_ble = false;
static boolean connected_ble = false;
static boolean doScan_ble = false;
//Client device
BLEClient* pClient = NULL;
static BLEAddress *pServerAddress = NULL;
static BLERemoteCharacteristic* soilCharacteristic = NULL;
static BLERemoteCharacteristic* smokeCharacteristic = NULL;
static BLEAdvertisedDevice* myDevice = NULL;
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};



//---------------------------Sensor values & Data initialisation-----------------//
char* soilChar;
char* smokeChar;
int status = WL_IDLE_STATUS;
std::string val_smoke, val_soil;
char data_out_0,data_out_1;





//--------------------------Client Notify BLE functions------------------------//
//When the BLE Server sends a new smoke reading with the notify property
static void smokeNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                                uint8_t* pData, size_t length, bool isNotify) {
  // store temperature value

  smokeChar = (char *)pData;
  //newsmoke = true;
  // Serial.print(newsmoke);
}
//When the BLE Server sends a new soil reading with the notify property
static void soilNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                               uint8_t* pData, size_t length, bool isNotify) {
  // store humidity value
  soilChar = (char *)pData;
  //newsoil = true;
  // Serial.print(newsoil);
}



//----------------------------Client Callback functions----------------------------------////
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
      connected_ble = true;
      Serial.println("Connected to Server");
    }
    void onDisconnect(BLEClient *pClient)
    {
      connected_ble = false;
      Serial.println("Disconnected from Server");

    }
}
;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /* Called for each advertising BLE server.*/

    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID_1))
      {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect_ble = true;
        doScan_ble = true;

      } // Found our server
    }   // onResult
};    // MyAdvertisedDeviceCallbacks




//----------------Connection Formation Function------------------//
bool connectToServer()
{
  BLERemoteService *pRemoteService = NULL;
  Serial.print(" - Forming a connection to ");
  if (myDevice != NULL)
  {
    Serial.println(myDevice->getAddress().toString().c_str());
    delay(500);

    pClient = BLEDevice::createClient();
    Serial.println(" - Created client");
    delay(500);

    pClient->setClientCallbacks(new MyClientCallback());
    // delay(1000);

    // Connect to the remove BLE Server.
    pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    delay(500);

    // Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService(serviceUUID_1);
  }
  if (pRemoteService == nullptr)
  {
    Serial.print(" - Failed to find our service UUID: ");

    Serial.println(serviceUUID_1.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");
  delay(500);

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  soilCharacteristic = pRemoteService->getCharacteristic(soilcharUUID_1);
  smokeCharacteristic = pRemoteService->getCharacteristic(smokecharUUID_1);

  if (soilCharacteristic == nullptr || smokeCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");
  // Read value from characteristic
  soilCharacteristic->registerForNotify(soilNotifyCallback);
  smokeCharacteristic->registerForNotify(smokeNotifyCallback);
  if (soilCharacteristic->canRead())
  {
    val_soil = soilCharacteristic->readValue();
    Serial.print(" - Soil Moisture: ");
    Serial.println(val_soil.c_str());
  }
  if (smokeCharacteristic->canRead())
  {
    val_smoke = smokeCharacteristic->readValue();
    Serial.print(" - Smoke Density: ");
    Serial.println(val_smoke.c_str());
  }
  pClient->disconnect();
}



//-----------------------Main Setup function - Retrives value from Sensor--------------///
int a = 0;
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  if (a == 0) {
    Serial.begin(115200);
    a = 1;
  }

  pinMode(ONBOARD_LED, OUTPUT);
  Serial.println("\nStarting Arduino BLE Client application...");
  delay(500);

  //-----------------------BluetoothLE On/Off Functions
  BLE_init(); // Bluetooth ON
  BLE_deinit(); // Bluetooth OFF
  
}



//-----------------------Main Loop function - Connects to wifi--------------------------////
void loop()
{ //------------------------Now - Bluetooth is OFF, Turn ON Wifi Connections-----------------//
  Serial.println("\n------------------------------------------------");
  Serial.println(" \t:::: WiFi ON :::: ");
  Serial.println("------------------------------------------------\n");
 
  Serial.print("WiFi status :: ");
  Serial.println(WiFi.status());
  delay(100);




  //---------------------------------- Connect to WiFi----------------------------------------//
  wifi();
  //--------------------When Connected - Post Readings to Flask Server-------------------///
  if (WiFi.status() == WL_CONNECTED)
  { 

    //data_out_0 = val_soil.c_str();
    //data_out_1 = val_smoke.c_str();
    HTTPClient http;
    
    //Insert Server Name
    http.begin(serverName);
    http.addHeader("Content-Type", "text/plain"); 
    int httpResponseCode = http.POST(String(val_soil.c_str()) + ' ' + String(val_smoke.c_str()));
    
    Serial.print("Server Response Code - ");
    Serial.println(httpResponseCode);

    Serial.println("\n------------------------------------------------");
    
    Serial.print(" - Soil Moisture: ");
    Serial.println(val_soil.c_str());
    Serial.println("\n");
    Serial.print(" - Smoke Density: ");
    Serial.println(val_smoke.c_str());;

    

    http.end();
    WiFi.disconnect();
  }
  else
  {
    digitalWrite(ONBOARD_LED, LOW);
    Serial.println("WiFi Disconnected");
  }
  delay(500);
  Serial.println("\n------------------------------------------------");
  Serial.println(" \t:::: WiFi OFF :::: ");
  Serial.println("------------------------------------------------\n");
  // sleep();  ////device sleep and wake up in next 10 sec
  //setup();
  ESP.restart();
}




//---------------------------BLE Repeat Functions------------------------------------////
int k = 0;
void BLE_init()
{
  if (k == 0)
  {
    BLEDevice::init("CentNode");
  }
  Serial.println("\n------------------------------------------------");
  Serial.println(" \t:::: Bluetooth ON :::: ");
  Serial.println("------------------------------------------------\n");
  delay(1000);
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.

  BLEScan *pBLEScan = BLEDevice::getScan();
  Serial.print("pBLEScan ::");
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  //pBLEScan->setWindow(449);
  //pBLEScan->setActiveScan(true);
  pBLEScan->start(0);
  delay(1000);

  // Trying to connect
  if (connectToServer())
  {
    k = 1;
    Serial.println("Received Data from Characteristics/n");
  }
}
void BLE_deinit()
{
  myDevice = NULL;
  delay(500);
  pClient->disconnect();
  delay(500);
  connected_ble = false;
  delay(500);
  //pclient = NULL;
  //delay(100);
  //myDevice = NULL;

  BLEDevice::deinit(ESP.getFreeHeap());
  Serial.println("------------------------------------------------");
  Serial.println(" \t:::: Bluetooth OFF :::: ");
  Serial.println("------------------------------------------------");
  delay(100);
}

//----------------------------Wifi connection----------------------------------------//
void wifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(" WiFi is in connected STATE  :: ");
  }
  else
  {
    Serial.print("Attempting to connect to SSID :: ");
    Serial.println(wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print('.');
      delay(1000);
    }

    Serial.println("\n------------------------------------------------\n");
    Serial.println("Connected to wifi");

    Serial.print("IP address :: ");
    Serial.println(WiFi.localIP());
    Serial.print("WIFI Strength :: ");
    Serial.println(WiFi.RSSI());
    Serial.println("------------------------------------------------\n");
    //  Serial.print("WIFI_data:"); Serial.println(data_packet_str);
  }
}
