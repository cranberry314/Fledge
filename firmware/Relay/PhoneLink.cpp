#include "PhoneLink.h"

#include <BLEDevice.h>
#include <BLEServer.h>

// From PROTOCOL.md. The app uses the same ones.
#define SERVICE_ID "6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10"
#define CONTROLS_ID "6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10"

// Bluetooth calls us from its own task, while loop() runs on another, so
// everything the two share is only touched while holding this lock.
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static bool connected = false;
static uint8_t newestMessage[MESSAGE_SIZE];
static unsigned long newestMessageAt = 0;  // 0 = none since connecting

class ConnectionEvents : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    portENTER_CRITICAL(&lock);
    connected = true;
    newestMessageAt = 0;
    portEXIT_CRITICAL(&lock);
  }

  void onDisconnect(BLEServer *server) override {
    portENTER_CRITICAL(&lock);
    connected = false;
    newestMessageAt = 0;
    portEXIT_CRITICAL(&lock);
    BLEDevice::startAdvertising();  // so the phone can find us again
  }
};

class MessageEvents : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    if (characteristic->getLength() != MESSAGE_SIZE) return;
    portENTER_CRITICAL(&lock);
    memcpy(newestMessage, characteristic->getData(), MESSAGE_SIZE);
    newestMessageAt = millis();
    portEXIT_CRITICAL(&lock);
  }
};

void PhoneLink::begin() {
  BLEDevice::init("Fledge Relay");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ConnectionEvents());

  BLEService *service = server->createService(SERVICE_ID);
  BLECharacteristic *controls = service->createCharacteristic(
      CONTROLS_ID, BLECharacteristic::PROPERTY_WRITE_NR);
  controls->setCallbacks(new MessageEvents());
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_ID);
  advertising->setScanResponse(true);  // the name goes in the scan response
  BLEDevice::startAdvertising();
}

bool PhoneLink::isConnected() const {
  portENTER_CRITICAL(&lock);
  bool now = connected;
  portEXIT_CRITICAL(&lock);
  return now;
}

unsigned long PhoneLink::latest(Controls &controls) const {
  uint8_t message[MESSAGE_SIZE];
  portENTER_CRITICAL(&lock);
  memcpy(message, newestMessage, MESSAGE_SIZE);
  unsigned long at = newestMessageAt;
  portEXIT_CRITICAL(&lock);

  if (at == 0 || !readMessage(message, controls)) return 0;
  return at;
}
