#include<stdio.h>
#include "esp_attr.h"
#include<stdlib.h>
#include <ArduinoJson.h>
#include <AWS_IOT.h>
AWS_IOT myiot;   // AWS_IOT instance
#include <WiFi.h>

#include "time.h"
#include <Wire.h>//I2C
#include <string.h> //string lib

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <math.h>
#include "ens210.h"// ENS210 library
ENS210 ens210;


//tsp data
#include <WiFiUdp.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
WiFiUDP udp;

unsigned long epoch;//String utime;
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

//*****************************************


//Broker Address: ahrcxnah0j2zm-ats.iot.us-east-1.amazonaws.com
//Broker Port: 8883


char WIFI_SSID[] = "XXX";
char WIFI_PASSWORD[] = "XX@1111";
char HOST_ADDRESS[] = "xxxxxxxxxxxxxxxxxxxxxxxxxx";
char CLIENT_ID[] = "xxxxxxxxxxxxxxxxxxxxxxx";
char TOPIC_NAME[] = "monitor";

const int ledpin_4 = 4; // led status
int status = WL_IDLE_STATUS; //wifi status
int aws_status = 0;
int data_cnt = 0;

int tick = 0, msgCount = 0, msgReceived = 0;

//-------------------------------------------------------------------
// How many times we should attempt to connect to AWS
//-------------------------------------------------------------------
#define AWS_MAX_RECONNECT_TRIES 5

char payload[500]; //aws cloud data

String wifi_mac;
String type_str;
String tsp_str;
String temp_str;
String hum_str;
String heart_str;
String posx_str;
String posy_str;
String posz_str;
String bat_str;
String devid_str;
String spo2_str;
String data_packet_str; //watch value stored


//sleep an restart ESP32
#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  10        // Time ESP32 will go to sleep (in seconds)


/*
    BLE Advertised Device found:
    Name: BABY_WATCH,
    Address: 4c:11:ae:71:28:fe
*/

#include "BLEDevice.h"
#include <BLEUtils.h>
//#include <BLEServer.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

//Client device
BLEClient* pclient = NULL;
BLEClient*  pClient = NULL;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice = NULL;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.println("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length :: ");
  Serial.println(length);
  Serial.print(" data: ");
  Serial.println((char*)pData);
  delay(1000);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
      connected = true;
      Serial.println(" onConnect Connected to Server");
    }
    void onDisconnect(BLEClient* pClient)
    {
      connected = false;
      Serial.println("Disconnected from Server");
      exit;
    }
};

bool connectToServer() {
  BLERemoteService* pRemoteService = NULL;
  Serial.print(" - Forming a connection to ");
  if (myDevice != NULL)
  {
    Serial.println(myDevice->getAddress().toString().c_str());
    delay(1000);

    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");
    //  delay(1000);

    pClient->setClientCallbacks(new MyClientCallback());
    //delay(1000);

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    delay(1000);

    // Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService(serviceUUID);
  }
  if (pRemoteService == nullptr) {
    Serial.print(" - Failed to find our service UUID: ");

    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");
  delay(1000);

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  delay(7000);

  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }  else {
    Serial.println(" - Found our characteristic");
    delay(1000);
  }
  //********   READ DATA PACKET FROM BLE_WATCH *******************
  // Read the value of the characteristic.

  if (pRemoteCharacteristic->canRead())
  {
    Serial.println("Reading packet from watch");
    delay(2000);
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("BLE_data_packet:: "); //temp value RX
    Serial.println(value.c_str());
    delay(5000);

    data_packet_str = value.c_str(); //store "WATCH" data

    Serial.print("\nWATCH data:");
    Serial.println(data_packet_str);
    delay(1000);

    //*********  split string start  ************************
    float commaIndex = data_packet_str.indexOf(','); //temp
    unsigned long secondCommaIndex = data_packet_str.indexOf(',', commaIndex + 1); //hum
    float thirdCommaIndex = data_packet_str.indexOf(',', secondCommaIndex + 1);  // HRM
    float fourthCommaIndex = data_packet_str.indexOf(',', thirdCommaIndex + 1);  //spo2
    float fifthCommaIndex = data_packet_str.indexOf(',', fourthCommaIndex + 1);  //x-axis
    float sixthCommaIndex = data_packet_str.indexOf(',', fifthCommaIndex + 1);  //y-axis
    float seventhCommaIndex = data_packet_str.indexOf(',', sixthCommaIndex + 1); //z-axis

    //float eighthCommaIndex = data_packet_str.indexOf(',', seventhCommaIndex + 1);
    //float ninthCommaIndex = data_packet_str.indexOf(',', eighthCommaIndex + 1);
    // float tenthCommaIndex = data_packet_str.indexOf(',', ninthCommaIndex + 1);


    temp_str = data_packet_str.substring(0, commaIndex);
    hum_str = data_packet_str.substring(commaIndex + 1, secondCommaIndex);
    heart_str = data_packet_str.substring(secondCommaIndex + 1, thirdCommaIndex);
    spo2_str = data_packet_str.substring(thirdCommaIndex + 1, fourthCommaIndex);
    posx_str  = data_packet_str.substring(fourthCommaIndex + 1, fifthCommaIndex);
    posy_str = data_packet_str.substring(fifthCommaIndex + 1, sixthCommaIndex);
    posz_str = data_packet_str.substring(sixthCommaIndex + 1);

    //     = data_packet_str.substring(seventhCommaIndex + 1, eighthCommaIndex);
    //    bat_str = data_packet_str.substring(eighthCommaIndex + 1, ninthCommaIndex);
    //    spo2_str = data_packet_str.substring(ninthCommaIndex + 1);
    //    //par_str = data_packet_str.substring(tenthCommaIndex + 1 );


    Serial.println("-------------------------------------------");
    Serial.println("ble datapacket after split :");
    Serial.println(type_str); Serial.println(tsp_str);
    Serial.println(temp_str); Serial.println(hum_str);
    Serial.println(heart_str); Serial.println(posx_str);
    Serial.println(posy_str); Serial.println(posz_str);
    Serial.println(bat_str); Serial.println(spo2_str);
    Serial.println("-------------------------------------------");
    delay(1000);
    //****** split string end  *************************
  }
  pClient->disconnect();
}
/*
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    /* Called for each advertising BLE server.*/

    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
      {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup()  // start of setup.
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);

  pinMode(ledpin_4, OUTPUT);      //led out

  //for capacitive
  pinMode(15, INPUT);            //touch 2
  pinMode(14, INPUT);            //touch 3

  while (!Serial) continue;
  Serial.println("\nStarting Arduino BLE Client application...");
  delay(1000);
  ble_init();              // bluetooth ON
  //ble_reconnect();
  ble_deinit();
  Serial.println("\n----------------------------------------------------------------");
  Serial.print("::::End of Setup Function::::: ");
  Serial.println("\n----------------------------------------------------------------");
} // End of setup.

//******************************************************************************************************************************
//******************************** PLEASE SEE THIS FUNCTION  ********************************************************************
//*********************************************************************************************************************************

// This is the Arduino main loop function.
void loop()
{
  // bluetooth OFF

  Serial.println("\n----------------------------------------------------------------");
  Serial.print("wifi status :: ");
  Serial.println(status);
  //check if wifi connected or not.
  //if connected then execute if part otherwise it execute else part to connect with wifi and further process
  if (status == WL_CONNECTED)
  {
    wifi_mac = WiFi.macAddress();
    Serial.print("BRIDGE Device ID :: ");
    Serial.println(wifi_mac);
    delay(2000);
    //if connected then execute if part otherwise else part
    if (aws_status == 1)
    {
      JSON_PACKET();                // JSON_DATA_PACKET STORE AND APEND
      aws_data();                   //data ready to publish
    }
    else
    {
      aws();                        // AMAZON CLOUD connect
      JSON_PACKET();                // JSON_DATA_PACKET STORE AND APEND
      aws_data();                   //data ready to publish
    }


  }
  else                              // first of all connect to wifi and do  further process
  {
    wifi();                         // wifi reconnect

    if (aws_status == 1)            //if connected then execute if part otherwise else part
    {
      JSON_PACKET();                // JSON_DATA_PACKET STORE AND APEND

      aws_data();                   //data ready to publish
    }
    else                            // firstly create aws connection and then execute rest of like json packet and aws data
    {
      aws();                        // AMAZON CLOUD connect

      JSON_PACKET();                // JSON_DATA_PACKET STORE AND APEND

      aws_data();                   //data ready to publish
    }

  }
  delay(5000);
  Serial.println("\n########################################\n########################################");
  // sleep();  ////device sleep and wake up in next 10 sec
  setup();

}

//*****************************************************************************************************************************

void wifi()
{
  //************** wifi *********************************

  if (status == WL_CONNECTED)
  {
    Serial.print(" wifi is in connected STATE  :: ");
  }
  else
  {
    while (status != WL_CONNECTED)
    {
      Serial.print("Attempting to connect to SSID :: ");
      Serial.println(WIFI_SSID);
      WiFi.mode(WIFI_MODE_STA);
      status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      delay(1500);
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
    Serial.println( WiFi.RSSI());
    Serial.print("WiFi MAC address :: ");
    Serial.println(WiFi.macAddress());
    Serial.println("------------------------------------------------\n");
    //  Serial.print("WIFI_data:"); Serial.println(data_packet_str);
  }
}

//**%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//******************              AMAZON CLOUD CONNECTION                *********************************************************************************

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void aws()
{
  // Try to connect to AWS and count how many times we retried.
  int retries = 0;
  Serial.println("Checking connection for AWS IOT");
  delay(1000);// if return value = 0 successfully cloud connect
  while (myiot.connect(HOST_ADDRESS, CLIENT_ID) != 0 && retries < AWS_MAX_RECONNECT_TRIES) // Connect to AWS using Host Address and Cliend ID
  {
    Serial.println(" Connecting to AWS... ");
    delay(1500);
    retries++;
  }

  if (myiot.connect(HOST_ADDRESS, CLIENT_ID) == 0)
  {
    Serial.println(" AWS_cloud is  connected ");
    aws_status = 1;
    delay(1500);
    return;
  }
  delay(200);

}// aws(); end

//*************** BLE ***************************************
int k = 0;
void ble_init()
{
  if (k == 0) {
    BLEDevice::init("ESP32");
  }
  Serial.println("\n------------------------------------------------");
  Serial.println(" \t:::: Bluetooth ON :::: ");
  Serial.println("------------------------------------------------\n");
  delay(1000);
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.

  BLEScan* pBLEScan = BLEDevice::getScan();
  Serial.print("pBLEScan ::");
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(100, false);
  delay(1000);

  //Trying to connect
  if (connectToServer())
  {
    k = 1;
    Serial.println("We have received DATA from Watch (Server)");

  }
  //  else
  //  {
  //    Serial.println("We have failed to connect to the Watch server; there is nothin more we will do.");
  //    }

}


void ble_deinit()

{
  connected = false;
  delay(500);
  myDevice = NULL;
  delay(500);
  pClient->disconnect();
  delay(2000);
  pclient = NULL;
  delay(2000);
  myDevice = NULL;

  BLEDevice::deinit(ESP.getFreeHeap());
  Serial.println("------------------------------------------------");
  Serial.println(" \t:::: Bluetooth OFF :::: ");
  Serial.println("------------------------------------------------");
  delay(100);
}



//************* data packet*************************************


void JSON_PACKET()
{
  // JSON memory allocation
  DynamicJsonDocument doc(350); //data packet buffer
  tsp();

  doc["type"] = "monitor";
  doc["tsp"] = "1584696179";
  doc["deviceID"] = wifi_mac;                 //wifi_mac;
  doc["temperature"] = temp_str;
  doc["humidity"] =  hum_str;
  doc["heart_rate"] = heart_str;
  doc["spo2_level"] = spo2_str;
  doc["pos_x"] =  posx_str ;
  doc["pos_Y"] = posy_str;
  doc["pos_Z"] = posz_str;
  //doc ["battery"] = "full";



  Serial.println("JSON DATA PACKET   :");
  serializeJsonPretty(doc, Serial);
  // serializeJson(doc, Serial); //show on serial
  serializeJson(doc, payload);//all obj stored from doc to playload

  /*
    // JSON PAYLOAD for TEMP_ALERT doc_1  ******************************

    doc_1["type"] = type_str;
    doc_1["param"] = "temp";
    doc_1["tsp"] = tsp_str;
    doc_1["deviceID"] = wifi_mac;
    JsonArray data_1 = doc_1.createNestedArray("temperature");
    data_1.add(temp_str); data_1.add(temp_str);
    data_1.add(temp_str); data_1.add(temp_str);
    data_1.add(temp_str); data_1.add(temp_str);
    data_1.add(temp_str); data_1.add(temp_str);
    data_1.add(temp_str); data_1.add(temp_str);

    //Serial.println(" TEMP_ALERT DATA PACKET");
    //  Serial.println("JSON payload for TEMP_ALERT is ready to upload :");
    // serializeJsonPretty(doc_1, Serial);
    //serializeJson(doc_1, Serial); //show on serial
    serializeJson(doc_1, payload_1);//all obj stored from doc to playload
    delay(10);
  */

}

void aws_data()
{
  if (myiot.publish(TOPIC_NAME, payload) == 0) // Publish the message(Temp and humidity)
  {
    Serial.println("\n------------------------------------------------");
    Serial.print("\nPublish Message :: ");
    Serial.println(payload);
    Serial.print("\nSize of monitor data packet after write :: ");
    Serial.print(sizeof(payload));
    Serial.println(" bytes");
    Serial.println("\n------------------------------------------------");
    Serial.println("Published monitor data packet to AWS_CLOUD successfully");
    Serial.println("------------------------------------------------");
    delay(1000);
    memset(payload, 0, 500 *  sizeof(payload[0]));
    Serial.println("\n------------------------------------------------");
    Serial.print("Size of payload after free ::");
    Serial.print(sizeof(payload));
    Serial.println(" bytes");
    delay(1000);
    led_status();
    data_cnt++;
  }

}


void led_status()
{
  //digitalWrite(ledpin_4, HIGH);   // turn the LED on (HIGH is the voltage level)
  //  Serial.println("led on");
  //delay(500);

  //digitalWrite(ledpin_4, LOW);    // turn the LED off by making the voltage LOW
  //Serial.println("led off");
  //delay(100);
}

void tsp()
{
  delay(500);
  //time unix ********* start  ***********************
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  int cb = udp.parsePacket();

  if (!cb)
  {
    delay(1);
  }
  else
  {
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    Serial.print("UNX_TIME : ");
    Serial.println(epoch);
  }
  delay(2000);  // 10000

}//************ time end  ********************

unsigned long sendNTPpacket(IPAddress & address)
{
  Serial.println("\n");
  Serial.println("Sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void ble_reconnect()
{

  if (connected)
  {
    Serial.println("bluetooth is connected mode");
    delay(500);
  }
  else if (doScan)
  {
    connected = false; // added from 28/05 on 09/06
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }



}
void sleep()
{
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //   Serial.println("BLE_BRIDGE  sleep for every " + String(TIME_TO_SLEEP) +
  //              " Seconds");
  //Serial.println("BRIDGE is getting UPDATING now... ");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}