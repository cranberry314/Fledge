// Fledge relay: receives phone controls over Bluetooth and sends them
// to the drone with the nRF24 radio.
//
// Temporary packet test: Pair sends a binding request on channels 16 and 17,
// and listens between requests for the drone's answer. When it answers, the
// relay sends the hop list, then 20 s of controls at rest (sticks centred,
// no command), then stops. The phone's sticks and commands are NOT sent.

#include <BLEDevice.h>
#include <BLEServer.h>
#include <RF24.h>

#include "RadioProtocol.h"
#if __has_include("LocalRadioId.h")
#include "LocalRadioId.h"
#define FLEDGE_LOCAL_RADIO_ID
#endif

// From PROTOCOL.md.
#define SERVICE_ID  "6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10"
#define CONTROLS_ID "6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10"

const int LED_PIN = 2;                 // the blue LED on the board
const unsigned long SILENCE_MS = 500;  // no message for this long: all 0
const int MESSAGE_SIZE = 8;            // version 2 of PROTOCOL.md
const uint32_t PAIR_TEST_PERIOD_US = 10000;
const unsigned long PAIR_TEST_WINDOW_MS = 30000;
const uint8_t PAIR_TEST_CHANNELS[] = {16, 17};

// After the drone answers, copy the real remote: tell the drone the hop
// channels on 16/17, then send controls at rest, one packet per slot,
// on 63, 65, 65, 60 and round again (measured from the remote, 10-11).
const uint8_t HOP_CHANNELS[4] = {63, 65, 65, 60};
const int HOP_LIST_PACKETS = 16;           // about 160 ms, like the remote
const uint32_t REST_PERIOD_US = 7570;      // one hop slot
const unsigned long REST_TEST_MS = 20000;  // then stop

// What the radio is doing now.
enum RadioStep { IDLE, PAIRING, SENDING_HOP_LIST, SENDING_REST };
const char *stepNames[] = {"IDLE", "PAIRING", "HOP_LIST", "REST"};
RadioStep radioStep = IDLE;

RF24 radio(4, 5);  // CE = GPIO 4, CSN = GPIO 5
uint8_t pairTestPacket[DroneRadio::PACKET_SIZE];
uint8_t hopListPacket[DroneRadio::PACKET_SIZE];
uint8_t droneId[2];  // from the drone's answer, bytes 4 and 5
bool radioReady = false;
uint32_t lastRadioAt = 0;
unsigned long stepStartedAt = 0;
unsigned long lastPairTestReportAt = 0;
int packetsThisStep = 0;
int hopIndex = 0;

// Listening between requests. XN297 chips start every packet with
// 71 0F 55 on air; the radio's address register wants it last byte first.
const uint8_t XN297_START[3] = {0x55, 0x0F, 0x71};
uint32_t droneAnswers = 0;   // 10 BB packets heard in this report
uint32_t otherPackets = 0;   // any other good packet heard

// The latest message, filled in by Bluetooth, read by loop().
// The lock stops loop() reading it while it is half written.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
uint8_t message[MESSAGE_SIZE];
unsigned long lastMessageAt = 0;
bool connected = false;

// What the drone should do: -100 to 100 each.
int forward = 0, right = 0, turn = 0, up = 0;

// Switches: speed and lights. The app starts slow with lights on.
int switches = 2;

// The command count: a command happens when it changes. -1 = not heard
// since the phone connected.
int commandCount = -1;

const char *commandNames[] = {
  "none", "take off or land", "MOTORS OFF", "level calibration",
  "flip forward", "flip back", "flip left", "flip right", "pair",
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

void setupRadio() {
  if (!radio.begin() || !radio.isChipConnected()) {
    Serial.println("Radio not found: check the wiring with RadioCheck.");
    return;
  }

  radio.setAutoAck(false);
  radio.disableCRC();
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAddressWidth(5);
  radio.setPayloadSize(DroneRadio::PACKET_SIZE);
  radio.openWritingPipe(DroneRadio::TX_ADDRESS);
  radio.stopListening();
  radioReady = true;
  Serial.println("nRF24 packet-test transmitter ready: 1 Mbps");
}

// Send one packet. With autoAck off the radio can't know whether anything
// heard it, so this only counts what was handed to the radio.
uint32_t transmittedPackets = 0;
void sendPacket(uint8_t channel, const uint8_t packet[DroneRadio::PACKET_SIZE]) {
  radio.stopListening();
  radio.setAddressWidth(5);
  radio.openWritingPipe(DroneRadio::TX_ADDRESS);
  radio.setChannel(channel);
  if (radio.write(packet, DroneRadio::PACKET_SIZE)) {
    transmittedPackets++;
  }
}

// Listen on the channel of the last packet sent.
void listenAfterSending() {
  radio.setAddressWidth(3);
  radio.openReadingPipe(0, XN297_START);
  radio.startListening();
}

void transmitNextPacket() {
#ifndef FLEDGE_LOCAL_RADIO_ID
  return;
#else
  if (!radioReady || radioStep == IDLE) return;
  uint32_t period = (radioStep == SENDING_REST) ? REST_PERIOD_US : PAIR_TEST_PERIOD_US;
  if ((uint32_t)(micros() - lastRadioAt) < period) return;
  lastRadioAt += period;  // keeps a steady beat, even if loop() is late

  if (radioStep == PAIRING) {
    // 1. The remote's pairing request, then listen for the answer.
    sendPacket(PAIR_TEST_CHANNELS[packetsThisStep % 2], pairTestPacket);
    packetsThisStep++;
    listenAfterSending();
  } else if (radioStep == SENDING_HOP_LIST) {
    // 2. Tell the drone the hop channels.
    sendPacket(PAIR_TEST_CHANNELS[packetsThisStep % 2], hopListPacket);
    packetsThisStep++;
    if (packetsThisStep == HOP_LIST_PACKETS) {
      radioStep = SENDING_REST;
      stepStartedAt = millis();
      lastRadioAt = micros();
      hopIndex = 0;
      Serial.println("Hop list sent. Sending controls at rest on 63/65/65/60 for 20 seconds.");
    }
  } else if (radioStep == SENDING_REST) {
    // 3. Sticks centred, no command, lights on: what the remote sends at rest.
    uint8_t nextChannel = HOP_CHANNELS[(hopIndex + 1) % 4];
    uint8_t packet[DroneRadio::PACKET_SIZE];
    DroneRadio::makeControlPacket(LOCAL_RADIO_ID, droneId, nextChannel,
                                  0, 0, 0, 0, 2, 0, packet);
    sendPacket(HOP_CHANNELS[hopIndex], packet);
    hopIndex = (hopIndex + 1) % 4;
    if (millis() - stepStartedAt >= REST_TEST_MS) {
      radioStep = IDLE;
      Serial.println("Rest test ended; radio is idle. Did the drone's light stop flashing?");
    }
  }

  unsigned long reportAt = millis();
  if (reportAt - lastPairTestReportAt >= 5000) {
    Serial.printf("%s packets=%lu drone_answers=%lu other=%lu\n",
                  stepNames[radioStep], (unsigned long)transmittedPackets,
                  (unsigned long)droneAnswers, (unsigned long)otherPackets);
    transmittedPackets = 0;
    droneAnswers = 0;
    otherPackets = 0;
    lastPairTestReportAt = reportAt;
  }
#endif
}

// While pairing, between requests: read anything heard. The drone's answer
// starts 10 BB, then our own ID bytes; its own ID is in bytes 4 and 5.
void listenForDrone() {
#ifdef FLEDGE_LOCAL_RADIO_ID
  if (!radioReady || radioStep != PAIRING) return;
  while (radio.available()) {
    uint8_t packet[DroneRadio::PACKET_SIZE];
    uint8_t data[DroneRadio::DATA_SIZE];
    radio.read(packet, sizeof(packet));
    if (!DroneRadio::decodePacket(packet, data)) continue;  // noise

    bool answer = data[0] == 0x10 && data[1] == 0xBB &&
                  data[2] == LOCAL_RADIO_ID[1] && data[3] == LOCAL_RADIO_ID[2];
    Serial.printf("%s at %lu ms:", answer ? "DRONE ANSWERED" : "HEARD", millis());
    for (uint8_t i = 0; i < DroneRadio::DATA_SIZE; i++) {
      Serial.printf(" %02X", data[i]);
    }
    Serial.println();
    if (!answer) {
      otherPackets++;
      continue;
    }

    droneAnswers++;
    droneId[0] = data[4];
    droneId[1] = data[5];
    DroneRadio::makeHopListPacket(LOCAL_RADIO_ID, droneId, HOP_CHANNELS, hopListPacket);
    radioStep = SENDING_HOP_LIST;
    packetsThisStep = 0;
    radio.stopListening();
    Serial.println("Sending the hop list on channels 16 and 17.");
    return;
  }
#endif
}

void startPairTest() {
#ifndef FLEDGE_LOCAL_RADIO_ID
  Serial.println("Pair test unavailable: ignored LocalRadioId.h is missing.");
  return;
#else
  if (!radioReady) {
    Serial.println("Pair test unavailable: radio is not ready.");
    return;
  }
  if (radioStep != IDLE) {
    Serial.println("Pair test already running.");
    return;
  }

  DroneRadio::makeBindingPacket(LOCAL_RADIO_ID, pairTestPacket);
  radioStep = PAIRING;
  packetsThisStep = 0;
  lastRadioAt = micros() - PAIR_TEST_PERIOD_US;
  stepStartedAt = millis();
  lastPairTestReportAt = millis();
  Serial.println("Pair test: sending remote binding request on channels 16 and 17 for 30 seconds.");
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  setupRadio();

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

#ifndef FLEDGE_LOCAL_RADIO_ID
  Serial.println("Missing ignored LocalRadioId.h; packet test is disabled.");
#endif
  Serial.println(radioReady
      ? "Packet test ready: type p in Serial Monitor or tap Pair in Fledge to transmit."
      : "Radio unavailable; Bluetooth only.");
}

void loop() {
  static bool wasConnected = false;
  static bool silent = true;

  while (Serial.available() > 0) {
    int input = Serial.read();
    if (input == 'p' || input == 'P') {
      startPairTest();
    }
  }

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

  if (radioStep == PAIRING &&
      millis() - stepStartedAt >= PAIR_TEST_WINDOW_MS) {
    radioStep = IDLE;
    radio.stopListening();
    Serial.println("Pair test ended with no answer from the drone; radio is idle.");
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
      Serial.printf("Command: %s\n", command < 9 ? commandNames[command] : "unknown");
      if (command == 8) {
        startPairTest();
      } else {
        Serial.println("Flight commands are disabled in this packet-test build.");
      }
    }
    commandCount = latest[7];
  }

  transmitNextPacket();
  listenForDrone();
  if (radioStep == IDLE) {
    delay(1);  // while sending, no pause: each packet must go out on time
  }
}
