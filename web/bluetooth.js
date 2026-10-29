// bluetooth.js: the Bluetooth link from this page to the relay (the ESP32).
//
// It sends the same message as the iPhone app (see PROTOCOL.md): 8 numbers,
// 20 times a second, even when nothing changes. If the messages stop for
// half a second (the page closes, the Chromebook sleeps), the relay holds
// the throttle down and the drone comes down.
//
// Chrome only allows Bluetooth on a page from http://localhost or https,
// not one opened from a file: see README.md.

// From PROTOCOL.md (Chrome wants them in small letters).
const SERVICE_ID = "6e1d0001-3c5a-4b8e-9f21-7a4d2c8b5e10";
const CONTROLS_ID = "6e1d0002-3c5a-4b8e-9f21-7a4d2c8b5e10";
const MESSAGE_VERSION = 2;
const TAKE_OFF_OR_LAND = 1;
const PAIR = 8;
const LIGHTS_ON = 2; // the switches byte; slow speed, always
const SEND_EVERY = 50; // milliseconds: 20 messages a second, like the phone

let controls = null; // the relay's controls, while connected
let sticks = { forward: 0, right: 0, turn: 0, up: 0 };
let letGoAt = 0; // performance.now() when the sticks go back to the middle
let command = 0;
let commandCount = 0;
let writing = false;

function canUseBluetooth() {
  return "bluetooth" in navigator;
}

function relayConnected() {
  return controls !== null;
}

// Chrome shows a list of relays to pick from. onLost() runs if the link drops.
async function connectRelay(onLost) {
  const device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE_ID] }] });
  device.addEventListener("gattserverdisconnected", () => {
    controls = null;
    onLost();
  });
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE_ID);
  controls = await service.getCharacteristic(CONTROLS_ID);
}

async function sendControls() {
  if (!controls || writing) return;
  if (letGoAt && performance.now() >= letGoAt) {
    sticks = { forward: 0, right: 0, turn: 0, up: 0 };
    letGoAt = 0;
  }
  // Uint8Array stores -30 as 226, which is how PROTOCOL.md wants it.
  const message = new Uint8Array([MESSAGE_VERSION, sticks.forward, sticks.right, sticks.turn,
    sticks.up, LIGHTS_ON, command, commandCount]);
  writing = true;
  try {
    await controls.writeValueWithoutResponse(message);
  } catch (error) {
    // A lost message is fine: the next one comes 50 ms later.
  } finally {
    writing = false;
  }
}

setInterval(sendControls, SEND_EVERY);

// Push the sticks for `seconds`, then let go.
function pushSticks(newSticks, seconds) {
  sticks = { forward: 0, right: 0, turn: 0, up: 0, ...newSticks };
  letGoAt = performance.now() + seconds * 1000;
}

// A one-off command: the relay does it once.
function giveCommand(newCommand) {
  command = newCommand;
  commandCount = (commandCount + 1) % 256;
}

function pairDrone() {
  giveCommand(PAIR);
}

// Make the real drone do one step of a flight from checker.js.
function flyStep(step) {
  if (!relayConnected()) return;
  const name = step.command.name;

  if (name === "take_off" || name === "land") {
    pushSticks({}, 0);
    giveCommand(TAKE_OFF_OR_LAND);
  } else if (name === "forward") {
    pushSticks({ forward: movePower }, step.seconds);
  } else if (name === "back") {
    pushSticks({ forward: -movePower }, step.seconds);
  } else if (name === "right") {
    pushSticks({ right: movePower }, step.seconds);
  } else if (name === "left") {
    pushSticks({ right: -movePower }, step.seconds);
  } else if (name === "up") {
    pushSticks({ up: upPower }, step.seconds);
  } else if (name === "down") {
    // Only as far as its hover: any more would bump the floor.
    pushSticks({ up: -downPower }, step.from.height - step.to.height);
  } else if (name === "rotate") {
    pushSticks({ turn: step.command.number < 0 ? -turnPower : turnPower }, step.seconds);
  } else {
    pushSticks({}, 0); // wait(): hover still
  }
}
