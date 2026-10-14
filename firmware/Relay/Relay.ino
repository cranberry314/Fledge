// Fledge relay: takes controls from the phone (Bluetooth) or the Mac (USB
// cable) and flies the drone with them, pretending to be its remote.
//
//   phone or Mac --controls--> this relay --nRF24 radio--> drone
//
// Where things are:
//   Controls.h       the controls message (PROTOCOL.md)
//   PhoneLink        Bluetooth: controls from the phone
//   TerminalLink     USB cable: controls from the Mac (firmware/fly/)
//   Remote.h         our pretend remote's sticks and buttons
//   DroneLink        the radio: pairing, then sending to the drone
//   XN297            how the drone's radio chip wraps packets
//   DronePackets.h   what each byte means (local only, never pushed)
//
// To pair: type p in the Serial Monitor (115200 baud), tap Pair in the
// app, or run firmware/fly/; then switch the drone on.

#include "Controls.h"
#include "DroneLink.h"
#include "PhoneLink.h"
#include "Remote.h"
#include "TerminalLink.h"

const int LED_PIN = 2;  // the blue LED on the board: on while the phone is connected

// No controls for this long and the relay stops trusting them (PROTOCOL.md).
const unsigned long CONTROLS_TIMEOUT_MS = 500;

// How long the relay holds a button down for each command. The real
// remote holds the oval about 0.36 s and the calibrate button sends for
// about 0.6 s; a flip pushes the stick past its end for about 0.2 s.
const unsigned long OVAL_PRESS_MS = 400;
const unsigned long CALIBRATE_PRESS_MS = 600;
const unsigned long FLIP_PUSH_MS = 250;

// The drone's own "motors off" packet isn't known yet, so MOTORS OFF
// holds the throttle fully down for this long instead.
const unsigned long MOTORS_OFF_DOWN_MS = 3000;

PhoneLink phone;
TerminalLink terminal;
DroneLink drone;
CommandCounter phoneCommands;
CommandCounter macCommands;

// Buttons the relay is holding down, and until when (millis()).
unsigned long ovalPressedUntil = 0;
unsigned long calibratingUntil = 0;
unsigned long flipUntil = 0;
Flip flipWay = Flip::None;
unsigned long throttleDownUntil = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  drone.begin();
  phone.begin();
  Serial.println("Relay ready");
}

void loop() {
  terminal.update();
  if (terminal.pairWasAsked()) drone.startPairing();
  if (terminal.helloWasSaid()) {
    Serial.println(drone.isLinked() ? "Relay ready: linked" : "Relay ready: not paired");
  }
  showPhoneConnection();

  Controls controls;
  bool haveControls = newestControls(controls);
  showControlsChanges(haveControls, controls);

  drone.setRemote(remoteFor(haveControls, controls));
  drone.update();

  // While the radio is busy, no pause: every packet must leave on time.
  if (!drone.isBusy()) delay(1);
}

// The newest controls from the phone or the Mac, and do any new command
// either of them gave. Returns false if neither has sent controls lately.
bool newestControls(Controls &controls) {
  Controls fromPhone;
  Controls fromMac;
  unsigned long phoneAt = phone.latest(fromPhone);
  unsigned long macAt = terminal.latest(fromMac);

  unsigned long now = millis();
  bool phoneFresh = phoneAt != 0 && now - phoneAt <= CONTROLS_TIMEOUT_MS;
  bool macFresh = macAt != 0 && now - macAt <= CONTROLS_TIMEOUT_MS;

  // A sender that went quiet may come back as a new program counting from
  // 0, so forget its count: its next message only sets the count again.
  if (!phoneFresh) phoneCommands.reset();
  if (!macFresh) macCommands.reset();

  if (phoneFresh && phoneCommands.isNew(fromPhone)) doCommand(fromPhone.command);
  if (macFresh && macCommands.isNew(fromMac)) doCommand(fromMac.command);
  if (phoneFresh && (!macFresh || phoneAt >= macAt)) {
    controls = fromPhone;
    return true;
  }
  if (macFresh) {
    controls = fromMac;
    return true;
  }
  return false;
}

void doCommand(Command command) {
  Serial.printf("Command: %s\n", commandName(command));
  unsigned long now = millis();

  switch (command) {
    case Command::None:
      break;
    case Command::Pair:
      drone.startPairing();
      break;
    case Command::TakeOffOrLand:
      ovalPressedUntil = now + OVAL_PRESS_MS;
      break;
    case Command::Calibrate:
      calibratingUntil = now + CALIBRATE_PRESS_MS;
      break;
    case Command::MotorsOff:
      Serial.println("The drone's motors-off signal isn't known yet: "
                     "holding the throttle down instead.");
      throttleDownUntil = now + MOTORS_OFF_DOWN_MS;
      break;
    case Command::FlipForward:
    case Command::FlipBack:
    case Command::FlipLeft:
    case Command::FlipRight:
      flipWay = flipFor(command);
      flipUntil = now + FLIP_PUSH_MS;
      break;
  }
}

Flip flipFor(Command command) {
  switch (command) {
    case Command::FlipForward: return Flip::Forward;
    case Command::FlipBack: return Flip::Back;
    case Command::FlipLeft: return Flip::Left;
    case Command::FlipRight: return Flip::Right;
    default: return Flip::None;
  }
}

// What our pretend remote should be doing now.
RemoteState remoteFor(bool haveControls, const Controls &controls) {
  RemoteState remote;
  unsigned long now = millis();

  if (!haveControls || now < throttleDownUntil) {
    // No controls (phone gone, Mac script stopped, cable out), or MOTORS
    // OFF: sticks in the middle and the throttle fully down, so the drone
    // comes down. This never presses the oval: that would take off a
    // drone that is already on the ground.
    // NOT YET TESTED in flight: check it lands the drone.
    remote.leftStickUp = -100;
    return remote;
  }

  remote.leftStickUp = controls.up;
  remote.leftStickRight = controls.turn;
  remote.rightStickUp = controls.forward;
  remote.rightStickRight = controls.right;
  remote.fast = controls.fast;
  remote.lightsOn = controls.lightsOn;
  remote.ovalPressed = now < ovalPressedUntil;
  remote.calibrating = now < calibratingUntil;
  remote.flip = (now < flipUntil) ? flipWay : Flip::None;
  return remote;
}

// The LED and a line in the Serial Monitor when the phone comes and goes.
void showPhoneConnection() {
  static bool wasConnected = false;
  bool connected = phone.isConnected();
  if (connected == wasConnected) return;

  wasConnected = connected;
  digitalWrite(LED_PIN, connected ? HIGH : LOW);
  Serial.println(connected ? "Phone connected" : "Phone disconnected");
  phoneCommands.reset();  // its next message only sets the count
}

// A line in the Serial Monitor when the controls change or stop.
void showControlsChanges(bool haveControls, const Controls &controls) {
  static bool hadControls = false;
  static Controls shown;

  if (haveControls != hadControls) {
    hadControls = haveControls;
    Serial.println(haveControls ? "Controls arriving"
                                : "No controls for 0.5 s: sticks to the middle, throttle down");
  }
  if (!haveControls) return;

  bool changed = controls.forward != shown.forward || controls.right != shown.right ||
                 controls.turn != shown.turn || controls.up != shown.up ||
                 controls.fast != shown.fast || controls.lightsOn != shown.lightsOn;
  if (!changed) return;

  shown = controls;
  Serial.printf("forward %4d   right %4d   turn %4d   up %4d   %s   lights %s\n",
                controls.forward, controls.right, controls.turn, controls.up,
                controls.fast ? "fast" : "slow", controls.lightsOn ? "on" : "off");
}
