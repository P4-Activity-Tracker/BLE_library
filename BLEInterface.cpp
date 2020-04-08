#include <Arduino.h>
#include "BLEInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
BLEInterface::BLEInterface(String *name, void (*callback)(String)) {
    _name = name; // Name of ble instance - The name that other devices will see.
    _callback = callback; // Callback function for handeling new messages.
}
// Start BLE
void BLEInterface::begin() {
    setupBLE(); // Start the ble service.
}
// Start BLE with debug
void BLEInterface::begin(bool debug) {
    _debug = debug;
    setupBLE();
}
// Send battery status to Android
void BLEInterface::sendBattery(uint8_t battery) {
    std::string message = batteryESP;
    message += equals;
    char charBattery[4];
    itoa(battery,charBattery,10);
    message += charBattery;
    while(!sendMessage(message)) {
        #ifdef doDebug
            Serial.println("Failed to send message, retrying...");
        #endif
    }
}
// Send bolus to Android
void BLEInterface::sendBolus(float bolus) {
    std::string message = bolusESP;
    message += equals;
    char charBolus[5];
    dtostrf(bolus, 4, 2, charBolus);  
    message += charBolus;
    while(!sendMessage(message)) {
        #ifdef doDebug
            Serial.println("Failed to send message, retrying...");
        #endif
    }
 
}
// Send temp to Android
void BLEInterface::sendTemp(float basalRate, uint8_t duration) {
    std::string message = tempESP;
    message += equals;
    char charBasal[6];
    dtostrf(basalRate, 4, 2, charBasal);
    message += charBasal;  
    message += binder;
    message += equals;
    char charDuration[4];
    itoa(duration,charDuration,10);
    message += charDuration;
    while(!sendMessage(message)) {
        #ifdef doDebug
            Serial.println("Failed to send message, retrying...");
        #endif
    }
}
// Send sleep to Android
void BLEInterface::sendSleep(uint8_t sleepTime) {
    std::string message = sleepESP;
    message += equals;
    char charSleep[4];
    itoa(sleepTime,charSleep,10);
    message += charSleep;
    while(!sendMessage(message)) {
        #ifdef doDebug
            Serial.println("Failed to send message, retrying...");
        #endif
    }
}
// Stop BLE
void BLEInterface::end() {
    stopBLE();
}
//************************************************************************************
// Private functions
//************************************************************************************
// Setup BLE server and service 
void BLEInterface::setupBLE() {
    // Create the BLE Device
    uint8_t size = _name->length()+1;
    char charName[size];
    _name->toCharArray(charName, size);
    charName[size] = '\0';
    BLEDevice::init(charName); // Give it a name
    // Create the BLE Server
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(new MyServerCallbacks(&this->_deviceConnected, &this->_debug));
    // Create the BLE Service
    BLEService *_pService = _pServer->createService(SERVICE_UUID);
    // Create input BLE Characteristic
    _pInputCharacteristic = _pService->
        createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    // Create output BLE Characteristic
    _pOutputCharacteristic = _pService->
        createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY 
                                                   | BLECharacteristic::PROPERTY_READ);
    // Add descriptors
    _pInputCharacteristic->setCallbacks(new MyCallbacks(&this->_debug, this->_callback));
    _pOutputCharacteristic->addDescriptor(new BLE2904());
    // Add service UUID to advertising 
    _pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    // Start the service
    _pService->start();
    // Start advertising
    _pServer->getAdvertising()->start();
    if (_debug) {
        Serial.println("BLE started");
    }
}
// Stop bluetooth
void BLEInterface::stopBLE() {
    _pOutputCharacteristic->notify();
    long timerStart = millis();
    if (_debug) {
        Serial.println("Stopping BLE");
    }
    _pServer->getAdvertising()->stop();
}
// Send bluetooth message
bool BLEInterface::sendMessage(std::string message) {
    if (_deviceConnected) {
        _pOutputCharacteristic->setValue(message);
        delay(10);
        _pOutputCharacteristic->notify(); // Send the value to the app!
        delay(10);
        if (_debug) {
            Serial.print("*** Sent string: ");
            Serial.println(message.c_str());
        }
        return true;
    }
    return false;
}
