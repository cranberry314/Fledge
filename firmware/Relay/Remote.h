// Remote: what our pretend remote is doing right now.
//
// The relay pretends to be the drone's own remote. This is that remote's
// sticks and buttons, by name. Relay.ino sets them from the phone's or the
// Mac's controls; DronePackets.h turns them into the bytes the drone hears.

#pragma once

// The top-right button, then a push on the right stick.
enum class Flip { None, Forward, Back, Left, Right };

struct RemoteState {
  // Sticks: -100 to 100, 0 = in the middle. They spring back to the
  // middle, like the real ones.
  int leftStickUp = 0;      // climb (+) or sink (-); 0 = hold height
  int leftStickRight = 0;   // spin right (+) or left (-)
  int rightStickUp = 0;     // fly forward (+) or back (-)
  int rightStickRight = 0;  // fly right (+) or left (-)

  bool fast = false;         // top-left button: fast or slow
  bool lightsOn = true;      // left small round button, held
  bool ovalPressed = false;  // the oval: take off, or land if flying
  bool calibrating = false;  // right small round button, held 3 s
  Flip flip = Flip::None;
};
