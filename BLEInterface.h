// Pump <-> ESP interface

// ensure this library description is only included once
#ifndef BLEInterface_h
#define BLEInterface_h

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2904.h>

#define SERVICE_UUID           "27652cbb-76f0-45eb-bc37-826ca7315457" // UART service UUID
#define CHARACTERISTIC_UUID_RX "8983c612-5b25-43cd-85f0-391f8dd3cb67"
#define CHARACTERISTIC_UUID_TX "848909c1-a6f0-4fa4-ac2a-06b9a9d4eb60"

class BLEInterface {
    public:
        bool _deviceConnected = false; // Device connectino status
        bool _debug = false; // Print debug to Serial prompt

        void (*_callback)(String);

        const static char pingAPS = 'P';
        const static char bolusAPS = 'B';
        const static char tempAPS = 'T';
        const static char sleepAPS = 'S';
        const static char comm_variable = '&';
        const static char endAPS = '!';

        std::string batteryESP = "e";
        std::string bolusESP = "b";
        std::string tempESP = "t";
        std::string sleepESP = "s";
        std::string binder = "&";
        std::string equals = "=";

        BLEInterface(String *name, void (*callback)(String));

        void begin();
        void begin(bool debug);
        void sendBattery(uint8_t battery);
        void sendBolus(float bolus);
        void sendTemp(float basalRate, uint8_t duration);
        void sendSleep(uint8_t sleepTime);
        void end();

    private:
        BLEServer *_pServer; // Service pointer
        BLECharacteristic *_pInputCharacteristic; // Read charac pointer
        BLECharacteristic *_pOutputCharacteristic; // Write charac pointer

        String *_name;

        bool sendMessage(std::string message);

        void setupBLE();
        void stopBLE();
};
// Device connection callback
class MyServerCallbacks: public BLEServerCallbacks {
    friend class BLEInterface;

    bool *__deviceConnected;
    bool *__debug;

    MyServerCallbacks(bool *deviceConnected, bool *debug) {
        __deviceConnected = deviceConnected;
        __debug = debug;
    }

    void onConnect(BLEServer* pServer) {
        if (*__debug) {
            Serial.println("Device connected!");
        }
        *__deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        if (*__debug) {
            Serial.println("Device disconnected!");
        }
        *__deviceConnected = false;
    }
};
// Message recieved callback
class MyCallbacks: public BLECharacteristicCallbacks {
    friend class BLEInterface;

    bool *__debug;
    void (*__callback)(String);
    
    MyCallbacks(bool *debug, void (*callback)(String)) {
        __debug = debug;
        __callback = callback;
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            String message = rxValue.c_str();
            if (*__debug) {
                Serial.print("Received message: ");
                Serial.println(message);
            }
            __callback(message);
        }
    }
};
#endif
