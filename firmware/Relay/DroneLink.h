// DroneLink: talks to the drone through the nRF24 radio, the way the
// drone's own remote does.
//
//   1. Pairing: send "pair with me" on channels 16 and 17, and listen in
//      between, until the drone answers (or 30 s pass).
//   2. Hop list: tell the drone which channels we will use.
//   3. Linked: send the remote's sticks and buttons every 7.57 ms, hopping
//      through the channels, for as long as the relay runs.
//
// What goes inside each packet is DronePackets.h's job.

#pragma once

#include <RF24.h>

#include "Remote.h"
#include "XN297.h"

class DroneLink {
 public:
  // Sets up the radio. Returns false if it isn't wired up.
  bool begin();

  // Start (or restart) pairing. Switch the drone on within 30 s.
  void startPairing();

  // What our pretend remote is doing; sent from the next packet on.
  void setRemote(const RemoteState &remote);

  // Sends the next packet when it is due. Call it as often as possible:
  // packets must leave on time.
  void update();

  bool isLinked() const { return step == Step::Linked; }
  bool isBusy() const { return step != Step::Idle; }

 private:
  enum class Step { Idle, Pairing, SendingHopList, Linked };

  void sendPacket(uint8_t channel, const uint8_t payload[XN297::PAYLOAD_SIZE]);
  void listenForAnswer();
  void readAnswers();
  void startStep(Step next);
  void reportNow();

  RF24 radio{4, 5};  // CE = GPIO 4, CSN = GPIO 5 (firmware/SETUP.md)
  bool radioFound = false;

  Step step = Step::Idle;
  unsigned long stepStartedAt = 0;  // millis()
  uint32_t lastPacketAt = 0;        // micros()
  int packetsThisStep = 0;
  int hopIndex = 0;

  uint8_t droneId[2] = {0, 0};
  RemoteState remote;

  unsigned long lastReportAt = 0;
  uint32_t packetsSinceReport = 0;
};
