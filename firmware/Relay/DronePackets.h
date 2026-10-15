// DronePackets: what each of the 16 bytes means to the drone.
//
// Worked out with the Sniffer and coach sessions (SNIFFING.md); the same
// table, written out with every action, is DroneActions.md. Every number
// here was measured from the real remote (a FunPX SQN-053). Where
// something is a guess, the comment says so. For a different drone, this
// is the file to rewrite.

#pragma once

#include <Arduino.h>

#include "Remote.h"
#include "XN297.h"

namespace DronePackets {

// Our pretend remote's ID: the ID of the real remote we sniffed. The drone
// pairs with this one. Whether it would pair with any other 5 bytes
// hasn't been tried.
const uint8_t REMOTE_ID[5] = {0xAA, 0xE2, 0xBD, 0x5A, 0x40};

// ---- The pairing request (the remote sends it on 16/17) ----
//   byte 0      10 (a pairing packet)
//   bytes 1-5   the remote's ID
const uint8_t PAIRING = 0x10;

// ---- The drone's answer ----
//   byte 0      10, byte 1  BB
//   bytes 2-3   the remote's ID, bytes 1-2 (who it is answering)
//   bytes 4-5   the drone's own ID
const uint8_t ANSWER = 0xBB;

// ---- Control packets (and the hop list, which has the same layout) ----
const int DRONE_ID_A = 0;       // the drone's ID, first byte
const int NEXT_CHANNEL = 1;     // the channel of the NEXT packet
const int DRONE_ID_B = 2;       // the drone's ID, second byte
const int CHECK_HIGH = 3;       // the checksum's top bits (see addCheckBytes)
const int LEFT_STICK_UP = 4;    // 01 full down, 80 middle, FF full up
const int LEFT_STICK_RIGHT = 5; // spin: 80 middle
const int RIGHT_STICK_UP = 6;   // forward: 80 middle
const int RIGHT_STICK_RIGHT = 7;// sideways: 80 middle
const int TRIM_A = 8;           // never changed in any recording
const int TRIM_B = 9;           // never changed in any recording
const int BUTTONS_A = 10;       // 40 = lights on, 80 = calibrating
const int ALWAYS_3F = 11;       // 3F in every packet
const int BUTTONS_B = 12;       // 01 = fast, 0C = oval pressed
const int CHECK_LOW = 13;       // the checksum's low byte
const int REMOTE_ID_A = 14;     // the remote's ID, byte 1
const int REMOTE_ID_B = 15;     // the remote's ID, byte 2

const uint8_t LIGHTS_ON = 0x40;
const uint8_t CALIBRATING = 0x80;
const uint8_t FAST = 0x01;
const uint8_t OVAL_PRESSED = 0x0C;

// In the hop list packet, bytes 4-7 hold the four hop channels instead of
// the sticks, NEXT_CHANNEL is 10, and BUTTONS_B is A5.
const uint8_t HOP_LIST = 0x10;
const uint8_t HOP_LIST_MARK = 0xA5;

// How far the sticks go, as an amount from the middle: 65 (41) in slow
// mode, 96 (60) in fast mode, the same both ways. The throttle always goes
// the whole way, 127.
const int SLOW_RANGE = 65;
const int FAST_RANGE = 96;
const int THROTTLE_RANGE = 127;

// A flip is the right stick pushed past its end (amount 127): FF for
// forward and right, 7F for back and left, for about 0.2 s. So 7F must
// never be sent for an ordinary push.
const uint8_t FLIP_FORWARD_OR_RIGHT = 0xFF;
const uint8_t FLIP_BACK_OR_LEFT = 0x7F;

// The throttle (left stick up/down): 80 in the middle, plus or minus the
// amount. Recorded sweep down: 80 64 42 14 01.
inline uint8_t throttleByte(int percent) {
  percent = constrain(percent, -100, 100);
  return 0x80 + (percent * THROTTLE_RANGE) / 100;
}

// The other sticks (spin, forward, sideways): forward / right is 80 plus
// the amount; back / left is the amount alone, counting up from 00.
// Recorded sweeps: back 80 0E 17 1D 39 4A 5A 60, left 80 03 08 15 30 45 60.
inline uint8_t stickByte(int percent, int range) {
  percent = constrain(percent, -100, 100);
  uint8_t amount = (abs(percent) * range) / 100;
  if (amount == 0) return 0x80;  // the middle
  if (percent < 0) return amount;
  return 0x80 + amount;
}

// The two check bytes. Add up byte 1 and bytes 4-12, plus 2: CHECK_LOW is
// the low 8 bits of the total, CHECK_HIGH holds the bits above that (as
// 06, 0A or 0E). Fits all 1181 different recorded control packets.
inline void addCheckBytes(uint8_t payload[XN297::PAYLOAD_SIZE]) {
  uint16_t total = payload[NEXT_CHANNEL] + 2;
  for (int i = LEFT_STICK_UP; i <= BUTTONS_B; i++) {
    total += payload[i];
  }
  payload[CHECK_LOW] = total & 0xFF;
  payload[CHECK_HIGH] = ((total >> 8) << 2) | 0x02;
}

// ---- Building packets ----

inline void pairingRequest(uint8_t payload[XN297::PAYLOAD_SIZE]) {
  memset(payload, 0, XN297::PAYLOAD_SIZE);
  payload[0] = PAIRING;
  memcpy(&payload[1], REMOTE_ID, 5);
}

// True if `payload` is the drone answering our pairing request; then
// `droneId` gets the drone's ID.
inline bool readAnswer(const uint8_t payload[XN297::PAYLOAD_SIZE], uint8_t droneId[2]) {
  bool isAnswer = payload[0] == PAIRING && payload[1] == ANSWER &&
                  payload[2] == REMOTE_ID[1] && payload[3] == REMOTE_ID[2];
  if (isAnswer) {
    droneId[0] = payload[4];
    droneId[1] = payload[5];
  }
  return isAnswer;
}

inline void startPacket(const uint8_t droneId[2], uint8_t payload[XN297::PAYLOAD_SIZE]) {
  memset(payload, 0, XN297::PAYLOAD_SIZE);
  payload[DRONE_ID_A] = droneId[0];
  payload[DRONE_ID_B] = droneId[1];
  payload[ALWAYS_3F] = 0x3F;
  payload[REMOTE_ID_A] = REMOTE_ID[1];
  payload[REMOTE_ID_B] = REMOTE_ID[2];
}

inline void hopList(const uint8_t droneId[2], const uint8_t hops[4],
                    uint8_t payload[XN297::PAYLOAD_SIZE]) {
  startPacket(droneId, payload);
  payload[NEXT_CHANNEL] = HOP_LIST;
  memcpy(&payload[LEFT_STICK_UP], hops, 4);
  payload[BUTTONS_A] = LIGHTS_ON;
  payload[BUTTONS_B] = HOP_LIST_MARK;
  addCheckBytes(payload);
}

inline void controls(const uint8_t droneId[2], uint8_t nextChannel,
                     const RemoteState &remote, uint8_t payload[XN297::PAYLOAD_SIZE]) {
  startPacket(droneId, payload);
  payload[NEXT_CHANNEL] = nextChannel;

  int range = remote.fast ? FAST_RANGE : SLOW_RANGE;
  payload[LEFT_STICK_UP] = throttleByte(remote.leftStickUp);
  payload[LEFT_STICK_RIGHT] = stickByte(remote.leftStickRight, range);
  payload[RIGHT_STICK_UP] = stickByte(remote.rightStickUp, range);
  payload[RIGHT_STICK_RIGHT] = stickByte(remote.rightStickRight, range);

  if (remote.flip == Flip::Forward) payload[RIGHT_STICK_UP] = FLIP_FORWARD_OR_RIGHT;
  if (remote.flip == Flip::Back) payload[RIGHT_STICK_UP] = FLIP_BACK_OR_LEFT;
  if (remote.flip == Flip::Right) payload[RIGHT_STICK_RIGHT] = FLIP_FORWARD_OR_RIGHT;
  if (remote.flip == Flip::Left) payload[RIGHT_STICK_RIGHT] = FLIP_BACK_OR_LEFT;

  if (remote.lightsOn) payload[BUTTONS_A] |= LIGHTS_ON;
  if (remote.calibrating) payload[BUTTONS_A] |= CALIBRATING;
  if (remote.fast) payload[BUTTONS_B] |= FAST;
  if (remote.ovalPressed) payload[BUTTONS_B] |= OVAL_PRESSED;

  addCheckBytes(payload);
}

}  // namespace DronePackets
