#include "DroneLink.h"

#if __has_include("DronePackets.h") && __has_include("LocalRadioId.h")
#include "DronePackets.h"
#define CAN_TALK_TO_DRONE
#endif

// Pairing, as the remote does it: "pair with me" on 16 and 17 in turn.
const uint8_t PAIRING_CHANNELS[2] = {16, 17};
const uint32_t PAIRING_EVERY_US = 10000;
const unsigned long PAIRING_GIVES_UP_MS = 30000;

// After the drone answers, the remote sends the hop list for ~160 ms.
const int HOP_LIST_PACKETS = 16;

// Then one packet per slot, on these channels in turn. Measured from the
// remote (2026-10-11): slots 7.57 ms apart, in this order. Whether another
// remote would use other channels is not known.
const uint8_t HOP_CHANNELS[4] = {63, 65, 65, 60};
const uint32_t SLOT_US = 7570;

const unsigned long REPORT_EVERY_MS = 5000;

bool DroneLink::begin() {
  radioFound = radio.begin() && radio.isChipConnected();
  if (!radioFound) {
    Serial.println("Radio not found: check the wiring with RadioCheck.");
    return false;
  }
  radio.setAutoAck(false);  // the XN297 way: no answers from the radio chip
  radio.disableCRC();       // XN297 checksums are added by XN297::encode
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(XN297::PACKET_SIZE);
  radio.stopListening();

#ifdef CAN_TALK_TO_DRONE
  Serial.println("Radio ready.");
#else
  Serial.println("Radio ready, but DronePackets.h or LocalRadioId.h is "
                 "missing: the relay can't talk to the drone.");
#endif
  return true;
}

void DroneLink::startPairing() {
#ifdef CAN_TALK_TO_DRONE
  if (!radioFound) {
    Serial.println("Can't pair: no radio.");
    return;
  }
  Serial.println("Pairing: switch the drone on now (30 seconds).");
  startStep(Step::Pairing);
#else
  Serial.println("Can't pair: DronePackets.h or LocalRadioId.h is missing.");
#endif
}

void DroneLink::setRemote(const RemoteState &newRemote) {
  remote = newRemote;
}

void DroneLink::startStep(Step next) {
  step = next;
  stepStartedAt = millis();
  packetsThisStep = 0;
  hopIndex = 0;
  uint32_t period = (next == Step::Linked) ? SLOT_US : PAIRING_EVERY_US;
  lastPacketAt = micros() - period;  // the first packet goes at once
  lastReportAt = millis();
  packetsSinceReport = 0;
}

void DroneLink::update() {
#ifdef CAN_TALK_TO_DRONE
  if (!radioFound || step == Step::Idle) return;

  if (step == Step::Pairing) {
    readAnswers();
    if (step == Step::Pairing && millis() - stepStartedAt >= PAIRING_GIVES_UP_MS) {
      radio.stopListening();
      step = Step::Idle;
      Serial.println("Pairing stopped: no answer from the drone in 30 seconds.");
      return;
    }
  }

  // Keep a steady beat: each packet is due one period after the last one
  // was due, even if loop() was a little late.
  uint32_t period = (step == Step::Linked) ? SLOT_US : PAIRING_EVERY_US;
  uint32_t now = micros();
  if (now - lastPacketAt < period) return;
  lastPacketAt += period;
  if (now - lastPacketAt > 4 * period) lastPacketAt = now;  // far behind: restart the beat

  uint8_t payload[XN297::PAYLOAD_SIZE];
  if (step == Step::Pairing) {
    DronePackets::pairingRequest(payload);
    sendPacket(PAIRING_CHANNELS[packetsThisStep % 2], payload);
    packetsThisStep++;
    listenForAnswer();
  } else if (step == Step::SendingHopList) {
    DronePackets::hopList(droneId, HOP_CHANNELS, payload);
    sendPacket(PAIRING_CHANNELS[packetsThisStep % 2], payload);
    packetsThisStep++;
    if (packetsThisStep == HOP_LIST_PACKETS) {
      startStep(Step::Linked);
      Serial.println("Linked: the drone is listening to the relay.");
    }
  } else if (step == Step::Linked) {
    uint8_t nextChannel = HOP_CHANNELS[(hopIndex + 1) % 4];
    DronePackets::controls(droneId, nextChannel, remote, payload);
    sendPacket(HOP_CHANNELS[hopIndex], payload);
    hopIndex = (hopIndex + 1) % 4;
  }

  if (millis() - lastReportAt >= REPORT_EVERY_MS) reportNow();
#endif
}

void DroneLink::sendPacket(uint8_t channel, const uint8_t payload[XN297::PAYLOAD_SIZE]) {
  uint8_t packet[XN297::PACKET_SIZE];
  XN297::encode(payload, packet);

  radio.stopListening();
  radio.setAddressWidth(5);
  radio.openWritingPipe(XN297::SEND_ADDRESS);
  radio.setChannel(channel);
  // With auto-ack off, write() can't know whether anything heard us: this
  // only counts packets handed to the radio.
  if (radio.write(packet, XN297::PACKET_SIZE)) packetsSinceReport++;
}

// Listen on the channel we just sent on, until the next packet is due.
void DroneLink::listenForAnswer() {
  radio.setAddressWidth(3);
  radio.openReadingPipe(0, XN297::LISTEN_ADDRESS);
  radio.startListening();
}

void DroneLink::readAnswers() {
#ifdef CAN_TALK_TO_DRONE
  while (radio.available()) {
    uint8_t packet[XN297::PACKET_SIZE];
    uint8_t payload[XN297::PAYLOAD_SIZE];
    radio.read(packet, XN297::PACKET_SIZE);
    if (!XN297::decode(packet, payload)) continue;  // noise

    if (DronePackets::readAnswer(payload, droneId)) {
      radio.stopListening();
      Serial.println("The drone answered. Sending the hop list.");
      startStep(Step::SendingHopList);
      return;
    }

    Serial.print("Heard another packet:");
    for (int i = 0; i < XN297::PAYLOAD_SIZE; i++) Serial.printf(" %02X", payload[i]);
    Serial.println();
  }
#endif
}

void DroneLink::reportNow() {
  if (step == Step::Pairing) {
    Serial.printf("Pairing: %lu requests sent, no answer yet.\n",
                  (unsigned long)packetsSinceReport);
  } else if (step == Step::Linked) {
    Serial.printf("Linked: %lu packets in the last 5 seconds.\n",
                  (unsigned long)packetsSinceReport);
  }
  packetsSinceReport = 0;
  lastReportAt = millis();
}
