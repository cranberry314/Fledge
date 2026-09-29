# Fledge 🐣

Fly a toy drone (FunPX SQN-053) from an iPhone:

    iPhone app --Bluetooth--> ESP32 --SPI--> nRF24L01+ --2.4 GHz--> drone

The drone itself is not modified.

## Parts

- **Drone:** FunPX SQN-053, a 2.4 GHz toy drone, with its own remote.
  Its radio is an XN297 (or a copy) at 1 Mbps, which an nRF24L01+ can
  read; see `firmware/SNIFFING.md`.
- **Relay:** an ELEGOO ESP-WROOM-32 dev board (30 pins, USB-C, CP2102
  USB-serial chip) and a HiLetgo nRF24L01+ module (PCB antenna, 2x4 pin
  header), on the breadboard from the ELEGOO 235-piece kit, with a 10 µF
  capacitor and female-to-male jumper wires. Wiring:
  `firmware/SETUP.md` step 3.
- **Phone:** any iPhone with iOS 26 (built on an iPhone 13). It uses the
  motion sensors, the barometer and Bluetooth LE.

## Folders

- `ios/` — the iPhone app (Swift, SwiftUI)
- `firmware/` — the ESP32 code: the relay (so far it receives the phone's
  controls over Bluetooth; it doesn't talk to the drone yet), and bench
  tools (radio check, sniffer, session coach)
- `PROTOCOL.md` — the messages the app sends the relay

## Changing how Fledge flies

Open `ios/Fledge/Tuning.swift`, change a number, colour or emoji, then build
and run again.

## Building the iPhone app

The Xcode project is generated from `ios/project.yml`, so it isn't kept in git.
To create it:

    cd ios
    xcodegen

Then open `ios/Fledge.xcodeproj` in Xcode, or build from VS Code with SweetPad.
New files added to `ios/Fledge/` become part of the app automatically. Run
`xcodegen` again only when `project.yml` changes.
