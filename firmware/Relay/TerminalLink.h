// TerminalLink: controls from the Mac, over the USB cable.
//
// The Mac sends the same 8 numbers as the phone's Bluetooth message
// (PROTOCOL.md), as one line of text, e.g.
//
//     controls 2 0 0 100 0 2 1 5
//
// (version, forward, right, turn, up, switches, command, count). A line
// saying just "p" or "pair" starts pairing, and "hello" makes the relay
// answer "Relay ready: linked" or "Relay ready: not paired".

#pragma once

#include "Controls.h"

class TerminalLink {
 public:
  // Reads whatever the Mac has sent. Call it every loop.
  void update();

  // Copies the newest controls line into `controls` and returns when it
  // arrived (millis()), or 0 if none has arrived.
  unsigned long latest(Controls &controls) const;

  // True once after the Mac asked to pair.
  bool pairWasAsked();

  // True once after the Mac said hello.
  bool helloWasSaid();

 private:
  void handleLine(const char *line);

  char line[64];
  int lineLength = 0;
  Controls newestControls;
  unsigned long newestControlsAt = 0;
  bool pairAsked = false;
  bool helloSaid = false;
};
