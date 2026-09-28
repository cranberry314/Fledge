# Fledge 🐣

Fly a toy drone (FunPX SQN-053) from an iPhone:

    iPhone app --Bluetooth--> ESP32 --SPI--> nRF24L01+ --2.4 GHz--> drone

The drone itself is not modified.

## Folders

- `ios/` — the iPhone app (Swift, SwiftUI)
- `firmware/` — the ESP32 code: bench tools for now (radio check, sniffer,
  session coach); the relay isn't written yet

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
