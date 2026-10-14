// Controls: what the drone should do right now.
//
// They arrive in an 8-byte message, from the phone over Bluetooth or from
// the Mac over the USB cable. PROTOCOL.md (at the top of the repo) says
// what each byte means. This file is the relay's one copy of that table:
// change PROTOCOL.md first, then this file, the app's Buttons.swift and
// the Mac's fly/drone.py to match.

#pragma once

#include <Arduino.h>

const int MESSAGE_SIZE = 8;
const uint8_t MESSAGE_VERSION = 2;

// One-off commands, numbered as in PROTOCOL.md.
enum class Command : uint8_t {
  None = 0,
  TakeOffOrLand = 1,
  MotorsOff = 2,
  Calibrate = 3,
  FlipForward = 4,
  FlipBack = 5,
  FlipLeft = 6,
  FlipRight = 7,
  Pair = 8,
};

inline const char *commandName(Command command) {
  switch (command) {
    case Command::None: return "none";
    case Command::TakeOffOrLand: return "take off or land";
    case Command::MotorsOff: return "motors off";
    case Command::Calibrate: return "calibrate";
    case Command::FlipForward: return "flip forward";
    case Command::FlipBack: return "flip back";
    case Command::FlipLeft: return "flip left";
    case Command::FlipRight: return "flip right";
    case Command::Pair: return "pair";
  }
  return "unknown";
}

struct Controls {
  int forward = 0;  // -100 (full back) to 100 (full forward)
  int right = 0;    // -100 (full left) to 100 (full right)
  int turn = 0;     // -100 (spin left) to 100 (spin right)
  int up = 0;       // -100 (down) to 100 (up), 0 = hold height
  bool fast = false;
  bool lightsOn = true;
  Command command = Command::None;  // the last command given
  uint8_t commandCount = 0;         // how many commands so far
};

// Reads one message into `controls`. Returns false, and leaves `controls`
// alone, if the message isn't one this relay understands.
inline bool readMessage(const uint8_t message[MESSAGE_SIZE], Controls &controls) {
  if (message[0] != MESSAGE_VERSION) return false;
  if (message[6] > (uint8_t)Command::Pair) return false;

  controls.forward = constrain((int8_t)message[1], -100, 100);
  controls.right = constrain((int8_t)message[2], -100, 100);
  controls.turn = constrain((int8_t)message[3], -100, 100);
  controls.up = constrain((int8_t)message[4], -100, 100);
  controls.fast = message[5] & 0x01;
  controls.lightsOn = message[5] & 0x02;
  controls.command = (Command)message[6];
  controls.commandCount = message[7];
  return true;
}

// A command happens once, when the count changes. Each sender (phone or
// Mac) keeps its own count, so each gets its own CommandCounter.
class CommandCounter {
 public:
  // True if `controls` carries a command we haven't done yet. The first
  // message after a sender appears only sets the count: nothing happens.
  bool isNew(const Controls &controls) {
    bool first = lastCount < 0;
    bool changed = controls.commandCount != lastCount;
    lastCount = controls.commandCount;
    return changed && !first;
  }

  // Forget the count, e.g. when a sender disconnects or goes quiet.
  void reset() { lastCount = -1; }

 private:
  int lastCount = -1;
};
