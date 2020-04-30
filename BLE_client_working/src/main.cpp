#include <Arduino.h>

#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
//#include "BLEScan.h"

// Den service vi gerne vil have forbindelse til, fra den trådløse server.
BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// Karateristiken af serveren vi er intereseret i. I dette tilfælde er det modtager og sender UUID.
BLEUUID    r_RX_charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
BLEUUID    r_TX_charUUID("12ee6f51-021d-438f-8094-bf5c5b36eab9");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic_RX;
static BLERemoteCharacteristic* pRemoteCharacteristic_TX;
static BLEAdvertisedDevice* myDevice;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.println("Callback triggered");
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};
// laver en connectiong til server
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // former en trådløs BLE forbindelse til server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Får en reference til den service vi er efter til den trådløse BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) { // Dette if statment benyttes hvis den ikke får forbindelse med servicen
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");




    // Får en reference til modtager og sender service vi er efter i den trådløse BLE server.
    pRemoteCharacteristic_RX = pRemoteService->getCharacteristic(r_RX_charUUID);
    if (pRemoteCharacteristic_RX == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(r_TX_charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    Serial.println(" - Found our r_RX_characteristic");
    // samme som ovenover..
    pRemoteCharacteristic_TX = pRemoteService->getCharacteristic(r_TX_charUUID);
    if (pRemoteCharacteristic_TX == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(r_TX_charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our r_TX_characteristic");

 
    if(pRemoteCharacteristic_TX->canNotify()) {
      Serial.println("Characteristic has notify property");
      pRemoteCharacteristic_TX->registerForNotify(notifyCallback);
    }
    

    connected = true;
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
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

MPU6050 imu; // Initialize MPU6050 class.
int16_t ax, ay, az; // Accelerometer data variables
int16_t gx, gy, gz; // Gyroscope data variables
uint8_t SDA_pin = 21;
uint8_t SCL_pin = 22;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  Wire.begin(SDA_pin, SCL_pin); // Where SDA is the pin number connected to SDA
  imu.initialize(); // Initialize the MPU6050
  imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Get accelerometer and gyroscope data
  

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    String newValue = "imu values: " + String(ax) + " " + String(ay) + " " + String(az) + " " + String(gx) + " " + String(gy) + " " + String(gz);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic_TX->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  //delay(5000); // Delay a 5 second between loops.
} // End of loop