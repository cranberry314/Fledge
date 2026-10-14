// PhoneLink: the phone's controls, over Bluetooth.
//
// The relay is a Bluetooth peripheral named "Fledge Relay" that the app
// finds and writes 8-byte messages to (PROTOCOL.md). Nothing else needs to
// open this file or PhoneLink.cpp.

#pragma once

#include "Controls.h"

class PhoneLink {
 public:
  // Start advertising, so the app can find the relay.
  void begin();

  bool isConnected() const;

  // Copies the newest message from the phone into `controls` and returns
  // when it arrived (millis()), or 0 if nothing usable has arrived since
  // the phone connected.
  unsigned long latest(Controls &controls) const;
};
