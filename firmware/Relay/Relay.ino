// Fledge relay, part 1: the Bluetooth side.
//
// Receives the phone's controls messages (see PROTOCOL.md at the top of
// the repo) and prints them on the Serial Monitor at 115200 baud. Nothing
// is sent to the drone yet.
//
// The blue LED is on while the phone is connected.

#include <BLEDevice.h>
#include <BLEServer.h>

// From PROTOCOL.md.
#define SERVICE_ID  "6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10"
#define CONTROLS_ID "6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10"

const int LED_PIN = 2;                 // the blue LED on the board
const unsigned long SILENCE_MS = 500;  // no message for this long: all 0
const int MESSAGE_SIZE = 8;            // version 2 of PROTOCOL.md

// The latest message, filled in by Bluetooth, read by loop().
// The lock stops loop() reading it while it is half written.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
uint8_t message[MESSAGE_SIZE];
unsigned long lastMessageAt = 0;
bool connected = false;

// What the drone should do: -100 to 100 each.
int forward = 0, right = 0, turn = 0, up = 0;

// Switches: speed and lights. -1 = not heard yet.
int switches = -1;

// The command count: a command happens when it changes. -1 = not heard
// since the phone connected.
int commandCount = -1;

const char *commandNames[] = {
  "none", "take off or land", "MOTORS OFF", "level calibration",
  "flip forward", "flip back", "flip left", "flip right",
};

class Phone : public BLEServerCallbacks {
  void onConnect(BLEServer *server) {
    connected = true;
  }
  void onDisconnect(BLEServer *server) {
    connected = false;
    BLEDevice::startAdvertising();  // so the phone can find us again
  }
};

class Controls : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    if (characteristic->getLength() != MESSAGE_SIZE) return;
    portENTER_CRITICAL(&lock);
    memcpy(message, characteristic->getData(), MESSAGE_SIZE);
    lastMessageAt = millis();
    portEXIT_CRITICAL(&lock);
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  BLEDevice::init("Fledge Relay");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new Phone());

  BLEService *service = server->createService(SERVICE_ID);
  BLECharacteristic *controls = service->createCharacteristic(
      CONTROLS_ID, BLECharacteristic::PROPERTY_WRITE_NR);
  controls->setCallbacks(new Controls());
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_ID);
  advertising->setScanResponse(true);  // the name goes in the scan response
  BLEDevice::startAdvertising();

  Serial.println("Fledge relay: waiting for the phone");
}

void loop() {
  static bool wasConnected = false;
  static bool silent = true;

  if (connected != wasConnected) {
    wasConnected = connected;
    digitalWrite(LED_PIN, connected ? HIGH : LOW);
    Serial.println(connected ? "Phone connected" : "Phone disconnected");
    commandCount = -1;  // the next message only sets the count
  }

  // Copy the latest message out, under the lock.
  uint8_t latest[MESSAGE_SIZE];
  unsigned long at;
  portENTER_CRITICAL(&lock);
  memcpy(latest, message, MESSAGE_SIZE);
  at = lastMessageAt;
  portEXIT_CRITICAL(&lock);

  // No phone, or no message for a while: everything goes back to 0.
  bool nowSilent = !connected || (at == 0) || (millis() - at > SILENCE_MS);
  int newForward = 0, newRight = 0, newTurn = 0, newUp = 0;
  bool understood = !nowSilent && latest[0] == 2;  // version 2
  if (understood) {
    newForward = (int8_t)latest[1];
    newRight = (int8_t)latest[2];
    newTurn = (int8_t)latest[3];
    newUp = (int8_t)latest[4];
  }

  // Say when the messages stop, and how long they were gone.
  static unsigned long silentSince = 0;
  static unsigned long lastSeen = 0;
  if (nowSilent && !silent) {
    silentSince = lastSeen;
    Serial.println(connected ? "No message for 500 ms: everything back to 0"
                             : "No phone: everything back to 0");
  }
  if (!nowSilent && silent && silentSince != 0) {
    Serial.printf("Messages back after %lu ms\n", at - silentSince);
  }
  silent = nowSilent;
  if (at != 0) {
    lastSeen = at;
  }

  if (newForward != forward || newRight != right || newTurn != turn || newUp != up) {
    forward = newForward;
    right = newRight;
    turn = newTurn;
    up = newUp;
    Serial.printf("forward %4d   right %4d   turn %4d   up %4d\n", forward, right, turn, up);
  }

  if (understood && latest[5] != switches) {
    switches = latest[5];
    Serial.printf("Speed %s, lights %s\n", (switches & 1) ? "fast" : "slow",
                  (switches & 2) ? "on" : "off");
  }

  if (understood && latest[7] != commandCount) {
    if (commandCount != -1) {  // not the first message: a new command
      int command = latest[6];
      Serial.printf("Command: %s\n", command < 8 ? commandNames[command] : "unknown");
    }
    commandCount = latest[7];
  }

  delay(10);
}
