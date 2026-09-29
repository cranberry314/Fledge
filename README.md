# Fledge 🐣

Fly a toy drone (FunPX SQN-053) from an iPhone:

    iPhone app --Bluetooth--> ESP32 --SPI--> nRF24L01+ --2.4 GHz--> drone

The drone itself is not modified.

## Parts

- **Drone:** FunPX SQN-053, a 2.4 GHz toy drone, with its own remote.
  Its radio is an XN297 (or a copy) at 1 Mbps, which an nRF24L01+ can
  read; see `firmware/SNIFFING.md`.
- **Relay:** an ESP32 board and an nRF24L01+ radio on a breadboard.
  Wiring: `firmware/SETUP.md` step 3.
- **Phone:** any iPhone with iOS 26 (built on an iPhone 13). It uses the
  motion sensors, the barometer and Bluetooth LE.

## Buying the parts

These are the exact listings used (search for the names). One of each
pack is needed; the spares are handy if a part gets damaged.

| Part | Listing | Used for |
|------|---------|----------|
| ESP32 boards | **ELEGOO 3PCS ESP-32 Dev Boards, ESP-WROOM-32, USB-C, WiFi Bluetooth 4.2** | The relay: talks to the phone over Bluetooth and to the radio over SPI |
| Radios | **HiLetgo 4pcs NRF24L01+ Wireless Transceiver Module, 2.4G** | Talks to the drone (the small PCB-antenna version, with a 2x4 pin header) |
| Kit | **ELEGOO Electronic Fun Kit Bundle with Breadboard, 235 Items for Arduino** | The breadboard, female-to-male jumper wires and the 10 µF capacitor |

**Why this ESP32 works with an iPhone.** An iPhone app can only talk to
a homemade gadget over **Bluetooth Low Energy (BLE)**; classic Bluetooth
serial needs Apple's MFi certification. The ESP-WROOM-32 chip on these
boards has Bluetooth 4.2 with BLE, so the app can connect to it directly.
Any board built on the original ESP32, ESP32-S3 or ESP32-C3 would work
too, but **not the ESP32-S2, which has no Bluetooth**.

**Why these boards suit a Mac.** Their USB-serial chip is a CP2102, which
macOS supports out of the box: no driver to install. They have USB-C, and
reset themselves for uploading, so the BOOT button is never needed. Use a
USB cable that carries data; a charge-only cable powers the board, but
the Mac never sees it.

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
