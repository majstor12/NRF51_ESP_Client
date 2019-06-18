/* Based od nkolban BLE client example
 * Copyright 
 * Can be used or modified without permission.
 * Viktor Čaplinskij, Faculty of Electrical Engineering and Computing, University of Zagreb.
 * 
 * This software is made for ESP32 based boards.
 * Created BLE Client connects to multisensor board whih can be found on Ebay for around $10 https://forum.mysensors.org/topic/6951/nrf5-multi-sensor-board-12-14/2
 * Factory default firmware on sensor board is used
 * Must use "esp32 V1.0.0" board from espressif when defining board (in Boards manager). Greater version may show connecting issues.
 * Problem connecting to server (Sensor board) when there are many scanned devices.
 * If stuck on connecting, go to enviroment with less BLE devices or turn of Sensor, reset ESP32 board and after 5s turn on Sensor again.
 * ESP32 sends over Serial pot firstily "A" or "G" then 3 values of each sensor in new line.
 * Receiving data in MATLAB with fgetl(s).
 * Complete D2M project documentation can be found at: http://pametne-kuce.zesoi.fer.hr/doku.php?id=2019:projekt_d2m 
 */
#include "BLEDevice.h"

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID accUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");    //Accelerometer Characteristic
static BLEUUID gyroUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");   //Gyroscope Characteristic

static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pGyroCharachteristic; 
static BLERemoteCharacteristic* pAccelCharachteristic;

static void notifyCallbackGyro(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {  
  Serial.println("G");
  Serial.println((short int)((pData[0]<<8)|pData[1]));  
  Serial.println((short int)((pData[2]<<8)|pData[3]));  
  Serial.println((short int)((pData[4]<<8)|pData[5]));
}

static void notifyCallbackAccel(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.println("A");
  Serial.println((short int)((pData[0]<<8)|pData[1]));  
  Serial.println((short int)((pData[2]<<8)|pData[3]));  
  Serial.println((short int)((pData[4]<<8)|pData[5])); 
}

bool connectToServer(BLEAddress pAddress) {
  Serial.print("Establishing a connection to device address: ");
  Serial.println(pAddress.toString().c_str());
  
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
      Serial.println(" - Connected to server ");
      
  // Obtain a reference to the service UUID  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Remote BLE service reference established");


  // Obtain a reference to the Gyroscope characteristic
  pGyroCharachteristic = pRemoteService->getCharacteristic(gyroUUID);
  if (pGyroCharachteristic == nullptr) {
    Serial.print("Failed to find Gyroscope characteristic UUID: ");
    Serial.println(gyroUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Remote BLE Gyroscope characteristic reference established");

  // Read the value of the Gyroscope characteristic.
  std::string value = pGyroCharachteristic->readValue();
  Serial.print("The characteristic value is currently: ");
  Serial.println(value.c_str());

  pGyroCharachteristic->registerForNotify(notifyCallbackGyro);

  // Obtain a reference to the Accelerometer characteristic
  pAccelCharachteristic = pRemoteService->getCharacteristic(accUUID);
  if (pAccelCharachteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(accUUID.toString().c_str());
   return false;
  }
  Serial.println(" - Remote BLE Accelerometer characteristic reference established");
  pAccelCharachteristic->registerForNotify(notifyCallbackAccel);

    // Read the value of the Accelerometer characteristic
  std::string value2 = pAccelCharachteristic->readValue();
  Serial.print("The characteristic value of Accelerometer is currently: ");
  Serial.println(value2.c_str());
}


/**
   Scan for BLE servers and find the first one that advertises the Nordic UART service.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found - ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, check to see if it contains the Nordic UART service.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {

        Serial.println("Found a device with the desired ServiceUUID!");
        advertisedDevice.getScan()->stop();

        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
        doConnect = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(9600);
  Serial.println("Starting Arduino BLE Central Mode (Client)");

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device. Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
} // End of setup.

// Values used to enable or disable notifications
const uint8_t notificationOff[] = {0x0, 0x0};
const uint8_t notificationOn[] = {0x1, 0x0};
bool onoff = true;

void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  //If connected & onoff=true -> enable notifications
  if (connected && onoff==true) {
      pGyroCharachteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      delay(500);
      pAccelCharachteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      onoff==false;
  } 
  
 delay(5000); // Delay five seconds between loops.  
} // End of loop
