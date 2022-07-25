#include <WiFi.h>
#include <HTTPClient.h>
#include "BLEDevice.h"
#include <BLEUtils.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//---------------------------Wifi Details--------------------------//
const char *wifi_ssid = "Gurjot10A";
const char *wifi_password = "12345678";

//---------------------------ThingsSpeak stuff---------------------//
// Domain Name with full URL Path for HTTP POST Request
const char *serverName = "http://api.thingspeak.com/update";
// Service API Key
String apiKey = "PP8C3Q4M3WC88OYA";

//---------------------------BLE pointers setup----------------------//
// The remote service we wish to connect to.
#define bleServerName "esp32S"
static BLEUUID serviceUUID_1("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
// The characteristic of the remote service we are interested in.
static BLEUUID soilcharUUID_1("f78ebbff-c8b7-4107-93de-889a6a06d408");
static BLEUUID    smokecharUUID_1("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");
//Device connection status
static boolean doConnect_ble = false;
static boolean connected_ble = false;
static boolean doScan_ble = false;
//Client device
BLEClient* pclient = NULL;
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
boolean newsoil = false;
boolean newsmoke = false;
#define ONBOARD_LED 2
int status = WL_IDLE_STATUS;



//--------------------------NOTIFY BLE functions------------------------//
//When the BLE Server sends a new smoke reading with the notify property
static void smokeNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  // store temperature value
  smokeChar = (char *)pData;
  newsmoke = true;
  // Serial.print(newsmoke);
}
//When the BLE Server sends a new soil reading with the notify property
static void soilNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  // store humidity value
  soilChar = (char *)pData;
  newsoil = true;
  // Serial.print(newsoil);
}



//----------------------------More BLE functions----------------------------------////
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
      connected_ble = true;
      Serial.println("Connected to Server");
}
void onDisconnect(BLEClient *pClient)
{
  connected_ble = false;
  Serial.println("Disconnected from Server");
  exit;
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
  pClient->disconnect();
}



//-----------------------Main Setup function - Retrives value from Sensor--------------///
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.begin(115200);

  pinMode(ONBOARD_LED, OUTPUT);

  while (!Serial)
    continue;

  Serial.println("\nStarting Arduino BLE Client application...");
  delay(1000);

  //-----------------------BluetoothLE On/Off Functions
  BLE_init(); // Bluetooth ON
  // ble_reconnect();
  BLE_deinit(); // Bluetooth OFF
  Serial.println("\n----------------------------------------------------------------");
  Serial.print("::::End of Setup Function::::: ");
  Serial.println("\n----------------------------------------------------------------");
}



//-----------------------Main Loop function - Connects to wifi--------------------------////
void loop()
{ //------------------------Now - Bluetooth is OFF, Turn ON Wifi Connections-----------------//
  Serial.println("\n----------------------------------------------------------------");
  Serial.print("WiFi status :: ");
  Serial.println(status);

  //---------------------------------- Connect to WiFi----------------------------------------//
  wifi();
  //--------------------When Connected - Publish reading to Thingsspeak-------------------///
  if (status == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);

    /*
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // Data to send with HTTP POST
    String httpRequestData = "api_key=" + apiKey + "&field1=" + String(random(40));
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);  */

    // If you need an HTTP request with a content type: application/json, use the following:
    http.addHeader("Content-Type", "application/json");
    // JSON data to send with HTTP POST
    String httpRequestData = "{\"api_key\":\"" + apiKey + "\",\"field1\":\"" + String(random(40)) + "\"}";
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
  delay(1000);
  Serial.println("\n########################################\n########################################");
  // sleep();  ////device sleep and wake up in next 10 sec
  setup();
}

//---------------------------BLE Repeat Functions------------------------------------////
int k = 0;
void BLE_init()
{
  if (k == 0)
  {
    BLEDevice::init("SensorNodeESP32");
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
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(100, false);
  delay(1000);

  // Trying to connect
  if (connectToServer())
  {
    k = 1;
    Serial.println("Received Data from Characteristics");
  }
}
void BLE_deinit()
{
  connected_ble = false;
  delay(500);
  myDevice = NULL;
  delay(500);
  pClient->disconnect();
  delay(500);
  pclient = NULL;
  delay(100);
  myDevice = NULL;

  BLEDevice::deinit(ESP.getFreeHeap());
  Serial.println("------------------------------------------------");
  Serial.println(" \t:::: Bluetooth OFF :::: ");
  Serial.println("------------------------------------------------");
  delay(100);
}

//----------------------------Wifi connection----------------------------------------//
void wifi()
{
  if (status == WL_CONNECTED)
  {
    Serial.print(" WiFi is in connected STATE  :: ");
  }
  else
  {
    while (status != WL_CONNECTED)
    {
      Serial.print("Attempting to connect to SSID :: ");
      Serial.println(wifi_ssid);
      WiFi.mode(WIFI_MODE_STA);
      status = WiFi.begin(wifi_ssid, wifi_password);
      delay(1000);
    }

    wifi_mac = WiFi.macAddress();
    Serial.print("BRIDGE Device ID :: ");
    Serial.println(wifi_mac);
    delay(100);

    Serial.println("------------------------------------------------\n");
    Serial.println("Connected to wifi");

    Serial.print("IP address :: ");
    Serial.println(WiFi.localIP());
    Serial.print("WIFI Strength :: ");
    Serial.println(WiFi.RSSI());
    Serial.println("------------------------------------------------\n");
    //  Serial.print("WIFI_data:"); Serial.println(data_packet_str);
  }
}0p0-o00
